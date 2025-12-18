/***************************************************************************//**
 * @file sl_activity_led.c
 * @brief
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
#include "sl_activity_led.h"
#include "assert.h"
#include "em_gpio.h"
#include "sl_sleeptimer.h"
#include "sl_system_config.h"

// Macros ----------------------------------------------------------------------
// Private type definitions ----------------------------------------------------
typedef struct {
  GPIO_Port_TypeDef port;
  unsigned int pin;
  sl_sleeptimer_timer_handle_t *timer_handle;
} led_config_t;

// Private function prototypes -------------------------------------------------
static inline void activity_led_set_state(const led_config_t *led, bool state);
static void activity_led_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data);

// Private variables -----------------------------------------------------------
static sl_sleeptimer_timer_handle_t led_timer_handles[SL_ACTIVITY_LED_INST_MAX];
static const led_config_t led_config[] = {
  [SL_ACTIVITY_LED_INST_BLE] = { SYS_CNF_ACTIVITY_LED_IO_PORT_BLE, SYS_CNF_ACTIVITY_LED_IO_PIN_BLE, &led_timer_handles[SL_ACTIVITY_LED_INST_BLE] },
  [SL_ACTIVITY_LED_INST_IR] = { SYS_CNF_ACTIVITY_LED_IO_PORT_IR, SYS_CNF_ACTIVITY_LED_IO_PIN_IR, &led_timer_handles[SL_ACTIVITY_LED_INST_IR] },
};
SL_STATIC_ASSERT(sizeof(led_config) / sizeof(led_config[0]) == SL_ACTIVITY_LED_INST_MAX, "LED configuration array size mismatch");

// Function definitions --------------------------------------------------------
void sl_activity_led_init(void)
{
  for (size_t i = 0; i < SL_ACTIVITY_LED_INST_MAX; ++i) {
    GPIO_PinModeSet(led_config[i].port, led_config[i].pin, gpioModePushPull, !SYS_CNF_ACTIVITY_LED_ACTIVE_STATE);
  }
}

void sl_activity_led_set(sl_activity_led_inst_t led_idx)
{
  if (led_idx < SL_ACTIVITY_LED_INST_MAX) {
    activity_led_set_state(&led_config[led_idx], true);
    sl_sleeptimer_restart_timer(led_config[led_idx].timer_handle, SYS_CNF_ACTIVITY_LED_TIMEOUT_MS,
                                activity_led_timer_callback, (void *)&led_config[led_idx],
                                0xFF, SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
  }
}

static void activity_led_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void)handle;
  activity_led_set_state(data, false);
}

static inline void activity_led_set_state(const led_config_t *led, bool state)
{
  if (state == SYS_CNF_ACTIVITY_LED_ACTIVE_STATE) {
    GPIO_PinOutSet(led->port, led->pin);
  } else {
    GPIO_PinOutClear(led->port, led->pin);
  }
}
