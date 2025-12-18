/***************************************************************************//**
 * @file led_test.h
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
#ifndef LED_TEST_H
#define LED_TEST_H
#ifdef __cplusplus
extern "C" {
#endif
#include "sl_led.h"
#include "sl_sleep.h"
#include "../sl_system_config.h"

// Macros ----------------------------------------------------------------------
// Type definitions ------------------------------------------------------------
// Private variables -----------------------------------------------------------
#if SYS_CNF_TEST_EN_LED
static struct {
  bool change;
  bool enabled;
  bool led_state;
  bool sleep_disabled;
} led_test_data;
#endif

// Private functions -----------------------------------------------------------
static inline void led_test_enable(bool test_enable, bool led_state)
{
#if SYS_CNF_TEST_EN_LED
  led_test_data.change = true;
  led_test_data.enabled = test_enable;
  led_test_data.led_state = led_state;
#else
  (void)test_enable;
  (void)led_state;
#endif
}

static inline bool led_test_is_enabled(void)
{
#if SYS_CNF_TEST_EN_LED
  return led_test_data.enabled;
#else
  return false;
#endif
}

static inline void led_test_process(const sl_led_t *led)
{
#if SYS_CNF_TEST_EN_LED
  if (led_test_data.change) {
    led_test_data.change = false;

    bool led_on = led_test_data.enabled && led_test_data.led_state;
    if (led_test_data.sleep_disabled != led_on) {
      led_test_data.sleep_disabled = led_on;
      sl_sleep_deep_enable(!led_on);
    }

    if (led_on) {
      sl_led_turn_on(led);
    } else {
      sl_led_turn_off(led);
    }
  }
#else
  (void)led;
#endif
}

#ifdef __cplusplus
}
#endif
#endif /* LED_TEST_H */
