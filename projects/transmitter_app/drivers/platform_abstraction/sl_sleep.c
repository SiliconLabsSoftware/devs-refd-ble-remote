/***************************************************************************//**
 * @file sl_sleep.c
 * @brief General sleep/timer component
 * @version 1.0.0
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include "assert.h"
#include "em_emu.h"
#include "em_gpio.h"
#include "sl_sleep.h"
#include "sl_sleeptimer.h"
#include "sl_power_manager.h"
#include "sl_power_manager_config.h"
#include "sl_system_config.h"

// Macros ----------------------------------------------------------------------
#if SYS_CNF_DEEP_SLEEP_IGNORE_PERIOD_PERCENT > 100 || SYS_CNF_DEEP_SLEEP_IGNORE_PERIOD_PERCENT < 0
  #error "SYS_CNF_DEEP_SLEEP_IGNORE_PERIOD_PERCENT must be between 0 and 100"
#endif
#if SL_POWER_MANAGER_INIT_EMU_EM4_PIN_RETENTION_MODE != EMU_EM4CTRL_EM4IORETMODE_DISABLE
  #error "Default EM4 pin retention mode must be set to 'Disable' in sl_power_manager_config.h"
#endif
#ifndef SYS_CNF_DEEP_SLEEP_PIN_LATCHING_EN
  #define SYS_CNF_DEEP_SLEEP_PIN_LATCHING_EN 0
#endif
#ifndef SYS_CNF_DEEP_SLEEP_SELECTIVE_PIN_LATCHING_EN
  #define SYS_CNF_DEEP_SLEEP_SELECTIVE_PIN_LATCHING_EN 0
#endif
#ifndef SYS_CNF_DEEP_SLEEP_GPIO_LATCHED_PINS
  #define SYS_CNF_DEEP_SLEEP_GPIO_LATCHED_PINS {}
#endif

// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
#if SYS_CNF_DEEP_SLEEP_TIMEOUT_MS
static void deep_sleep_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data);
static void deep_sleep_disable_pins(void);
static inline bool deep_sleep_set_pin_to_disabled(uint32_t port, uint32_t pin);
#endif

// Private variables -----------------------------------------------------------
static int sleep_disable_counter;
#if SYS_CNF_DEEP_SLEEP_TIMEOUT_MS
static uint32_t deep_sleep_timer_reset_ignore_interval_tick;
static sl_sleeptimer_timer_handle_t deep_sleep_timer_handle;
static const uint32_t deep_sleep_latched_pins[] = SYS_CNF_DEEP_SLEEP_GPIO_LATCHED_PINS;
#endif

// Function definitions --------------------------------------------------------
void sl_sleep_init(void)
{
#if SYS_CNF_DEEP_SLEEP_TIMEOUT_MS
  if (SYS_CNF_DEEP_SLEEP_PIN_LATCHING_EN) {
    sl_power_manager_em4_unlatch_pin_retention();
    EMU_EM4Init_TypeDef em4_init = EMU_EM4INIT_DEFAULT;
    em4_init.pinRetentionMode = emuPinRetentionLatch;
    EMU_EM4Init(&em4_init); //re-init power manager EM4 settings
  }
  sl_sleeptimer_ms32_to_tick((SYS_CNF_DEEP_SLEEP_IGNORE_PERIOD_PERCENT * SYS_CNF_DEEP_SLEEP_TIMEOUT_MS) / 100, &deep_sleep_timer_reset_ignore_interval_tick);
  sl_sleeptimer_start_timer_ms(&deep_sleep_timer_handle, SYS_CNF_DEEP_SLEEP_TIMEOUT_MS, deep_sleep_timer_callback, NULL, 0, SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
#endif
}

void sl_sleep_light_enable(bool enable)
{
  CORE_ATOMIC_SECTION(
    sleep_disable_counter = enable ? (sleep_disable_counter - 1) : (sleep_disable_counter + 1);
    )
  SL_ASSERT(sleep_disable_counter >= 0, "Sleep enabled counter is negative!");
}

bool app_is_ok_to_sleep(void)
{
  return sleep_disable_counter == 0;
}

void sl_sleep_deep_enable(bool enable)
{
  if (enable) {
    sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
  } else {
    sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
  }
}

void sl_sleep_reset_standby_timer(void)
{
#if SYS_CNF_DEEP_SLEEP_TIMEOUT_MS
  static uint32_t last_timestamp = UINT32_MAX / 2;
  uint32_t tick_count = sl_sleeptimer_get_tick_count();

  if (tick_count - last_timestamp >= deep_sleep_timer_reset_ignore_interval_tick) {
    last_timestamp = tick_count;
    sl_sleeptimer_restart_timer_ms(&deep_sleep_timer_handle, SYS_CNF_DEEP_SLEEP_TIMEOUT_MS, deep_sleep_timer_callback, NULL, 0, SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
  }
#endif
}

SL_WEAK void sl_sleep_standby_hook(void)
{
}

#if SYS_CNF_DEEP_SLEEP_TIMEOUT_MS
static void deep_sleep_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void)handle;
  (void)data;
  sl_sleep_standby_hook();
  // Disable all GPIOs except for latched pins
  // This is done to ensure minimal power consumption in deep sleep
  if (SYS_CNF_DEEP_SLEEP_PIN_LATCHING_EN
      && SYS_CNF_DEEP_SLEEP_SELECTIVE_PIN_LATCHING_EN
      && (sizeof(deep_sleep_latched_pins) / sizeof(deep_sleep_latched_pins[0]))) {
    deep_sleep_disable_pins();
  }
  sl_power_manager_enter_em4();
}

static void deep_sleep_disable_pins(void)
{
  for (uint32_t port = 0; port <= GPIO_PORT_MAX; port++) {
    for (int pin = 0; pin <= GPIO_PIN_MAX; pin++) {
      if (deep_sleep_set_pin_to_disabled(port, pin)) {
        GPIO_PinModeSet(port, pin, gpioModeDisabled, 0);
      }
    }
  }
}

static inline bool deep_sleep_set_pin_to_disabled(uint32_t port, uint32_t pin)
{
  bool keep = !GPIO_PORT_PIN_VALID(port, pin);

  // casted to int to avoid warnings about when the array is empty
  for (int i = 0; i < (int)(sizeof(deep_sleep_latched_pins) / sizeof(deep_sleep_latched_pins[0])) && !keep; ++i) {
    keep = (SYS_CNF_PORT_PIN_GET_PORT(deep_sleep_latched_pins[i]) == port && SYS_CNF_PORT_PIN_GET_PIN(deep_sleep_latched_pins[i]) == pin);
  }
  return !keep;
}
#endif
