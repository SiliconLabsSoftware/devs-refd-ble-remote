/***************************************************************************//**
 * @file led_activity.c
 * @brief LED Activity Component
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
#include <stddef.h>
#include <stdbool.h>
#include "assert.h"
#include "sl_sleep.h"
#include "led_activity.h"
#include "led_test.h"
#include "sl_led_effect.h"
#include "../sl_system_config.h"

#if SYS_CNF_LED_ACTIVITY_ENABLE
// Macros ----------------------------------------------------------------------
// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
static void led_activity_effect_done_callback(sl_led_effect_t *effect);
static void led_activity_timeout_callback(sl_led_effect_t *effect);

// Private variables -----------------------------------------------------------
static bool led_activity_running;
static volatile bool led_activity_stop_requested;
static volatile bool led_activity_stop_now;
static sl_led_effect_t led_activity_effect;

static const sl_led_t led_activity = SYS_CNF_LED_ACTIVITY_CONFIG;
static const uint16_t led_activity_blink_pattern_array[] = SYS_CNF_LED_ACTIVITY_PATTERN;
static const sl_led_effect_pattern_t led_activity_blink_pattern = {
  .pwm_values = led_activity_blink_pattern_array,
  .length = SL_LED_PATTERN_CALCULATE_LENGTH(led_activity_blink_pattern_array)
};
static const uint32_t led_activity_timeout_ms =  8 * SL_LED_PATTERN_MIN_TIME_MS * SL_LED_PATTERN_CALCULATE_LENGTH(led_activity_blink_pattern_array) / 10;

// Function definitions --------------------------------------------------------
void sl_led_activity_init(void)
{
  int sc = sl_led_effect_init(&led_activity_effect, &led_activity);
  SL_ASSERT(sc == 0, "LED activity effect initialization failed!");
}

void sl_led_activity_cyclic(void)
{
  if (led_activity_stop_now) {
    led_activity_stop_now = false;
    led_activity_running = false;
    sl_led_effect_stop(&led_activity_effect);
    sl_led_turn_off(&led_activity);
    sl_sleep_deep_enable(true);
  }
  if (!led_activity_running) {
    led_test_process(&led_activity);
  }
}

void sl_led_activity_start(void)
{
  if (!led_test_is_enabled()) {
    int sc = sl_led_effect_timer_start(&led_activity_effect, led_activity_timeout_ms, led_activity_timeout_callback);
    led_activity_stop_requested = false;

    if (!led_activity_running) {
      led_activity_running = true;
      sc |= sl_led_effect_start(&led_activity_effect, &led_activity_blink_pattern, 0, led_activity_effect_done_callback);
      sl_sleep_deep_enable(false);
    }
    SL_ASSERT(sc == 0, "LED activity effect start failed!");
  }
}

static void led_activity_effect_done_callback(sl_led_effect_t *effect)
{
  (void)effect;
  if (led_activity_stop_requested) {
    led_activity_stop_requested = false;
    led_activity_stop_now = true;
  }
}

static void led_activity_timeout_callback(sl_led_effect_t *effect)
{
  (void)effect;
  led_activity_stop_requested = true;
}

void sl_led_activity_stop(void)
{
  led_activity_stop_requested = true;
}

bool sl_led_activity_is_on(void)
{
  return led_activity_running
         || (led_test_is_enabled() && sl_led_get_state(&led_activity));
}

void sl_led_activity_test(bool test_enable, bool led_on)
{
  led_test_enable(test_enable, led_on);
}

bool sl_led_activity_test_is_enabled(void)
{
  return led_test_is_enabled();
}

#else  // SYS_CNF_LED_ACTIVITY_ENABLE
void sl_led_activity_init(void)
{
}

void sl_led_activity_cyclic(void)
{
}

void sl_led_activity_start(void)
{
}

void sl_led_activity_stop(void)
{
}

bool sl_led_activity_is_on(void)
{
  return false;
}

void sl_led_activity_test(bool test_enable, bool led_on)
{
  (void)test_enable;
  (void)led_on;
}

bool sl_led_activity_test_is_enabled(void)
{
  return false;
}
#endif // SYS_CNF_LED_ACTIVITY_ENABLE
