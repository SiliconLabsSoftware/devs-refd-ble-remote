/***************************************************************************//**
 * @file remote.c
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
#define SL_LOG_MODULE_NAME "RemoteApp"
#include "remote.h"
#include "voice.h"
#include "sl_acc.h"
#include "sl_ble.h"
#include "sl_battery_meas.h"
#include "sl_sleep.h"
#include "led_activity.h"
#include "led_backlight.h"
#include "key_handler.h"
#include "../sl_system_config.h"

// Macros ----------------------------------------------------------------------
#define REMOTE_ACC_CLEAR_INTERRUPT (!SYS_CNF_ACC_INT_AUTO_CLEAR_EN || SYS_CNF_ACC_INT_CLEAR_IS_MANDATORY)

// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
static void remote_cyclic(void);
static inline bool remote_acc_activity_check(void);

// Private variables -----------------------------------------------------------
static volatile bool remote_acc_activity;
static key_handler_info_t remote_key_info;

// Function definitions --------------------------------------------------------
void sl_remote_app_init(bool cold_start)
{
  sl_acc_init_io();
  sl_sleep_init(); //This will unlatch EM4 pins
  //----------------------------------------------------------------------------
  // GPIO pins are now active and can be used
  if (cold_start || !SYS_CNF_REMOTE_PICK_UP_DETECTION_EN_IN_STBY) { //re-initialize the HW on the board because power could have been lost
    sl_acc_init_device();
  }
  sl_battery_meas_init();
  sl_led_activity_init();
  sl_led_backlight_init();
  key_handler_init();
  voice_init();
}

void sl_remote_app_cyclic(void)
{
  voice_cyclic();
  key_handler_cyclic();
  sl_battery_meas_cyclic();
  sl_led_activity_cyclic();
  sl_led_backlight_cyclic();
  remote_cyclic();
}

static void remote_cyclic(void)
{
  bool voice_was_pressed = (SYS_CNF_REMOTE_VOICE_BUTTON_ID == remote_key_info.key_id);
  key_handler_get_info(&remote_key_info);
  if (!voice_was_pressed && (SYS_CNF_REMOTE_VOICE_BUTTON_ID == remote_key_info.key_id)) {
    voice_start();
  } else if (voice_was_pressed && (SYS_CNF_REMOTE_VOICE_BUTTON_ID != remote_key_info.key_id)) {
    voice_stop();
  }

  bool key_pressed = (KEY_HANDLER_STATUS_NO_KEY_PRESSED != remote_key_info.status);
  if (key_pressed) {
    sl_led_activity_start();
  }

  bool activity = remote_acc_activity_check();
  if (activity || key_pressed) {
    sl_sleep_reset_standby_timer();
    sl_led_backlight_on();
    sl_ble_advertising_start();
  }
}

void sl_acc_on_threshold_reached_event(void)
{
  remote_acc_activity = true;
}

static inline bool remote_acc_activity_check(void)
{
  bool activity = remote_acc_activity;

  if (remote_acc_activity) {
    remote_acc_activity = false;
    #if !SYS_CNF_ACC_INT_CLEAR_DELAY_MS && REMOTE_ACC_CLEAR_INTERRUPT
    sl_acc_interrupt_clear();
    #endif
  }

  #if SYS_CNF_ACC_INT_CLEAR_DELAY_MS && REMOTE_ACC_CLEAR_INTERRUPT
  static bool needs_irq_clear;
  if (!needs_irq_clear && activity) {
    needs_irq_clear = activity;
  }
  if (needs_irq_clear) {
    needs_irq_clear = !sl_acc_interrupt_clear_lazy(SYS_CNF_ACC_INT_CLEAR_DELAY_MS);
  }
  #endif
  return activity;
}

void sl_sleep_standby_hook(void)
{
#if !SYS_CNF_REMOTE_PICK_UP_DETECTION_EN_IN_STBY
  sl_acc_deinit();
#endif
}
