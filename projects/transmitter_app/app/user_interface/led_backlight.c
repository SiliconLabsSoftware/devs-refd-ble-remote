/***************************************************************************//**
 * @file led_backlight.c
 * @brief LED Backlight Component
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
#include "led_backlight.h"
#include "led_test.h"
#include "sl_led_effect.h"
#include "../sl_system_config.h"

#if SYS_CNF_LED_BACKLIGHT_ENABLE
// Macros ----------------------------------------------------------------------
// Private type definitions ----------------------------------------------------
typedef enum {
  LED_BACKLIGHT_STATE_IDLE,
  LED_BACKLIGHT_STATE_FADE_IN,
  LED_BACKLIGHT_STATE_ON,
  LED_BACKLIGHT_STATE_FADE_OUT,
} led_backlight_state_t;

typedef enum {
  LED_BACKLIGHT_EVENT_START,
  LED_BACKLIGHT_EVENT_FADE_EFFECT_DONE,
  LED_BACKLIGHT_EVENT_STOP,
} led_backlight_event_t;

// Private function prototypes -------------------------------------------------
static void led_backlight_effect_done_callback(sl_led_effect_t *effect);
static void led_backlight_timeout_callback(sl_led_effect_t *effect);
static void led_backlight_update(led_backlight_event_t event);
static led_backlight_state_t led_backlight_state_machine(led_backlight_event_t event, led_backlight_state_t state);
static void led_backlight_start_effects(led_backlight_state_t state);

// Private variables -----------------------------------------------------------
static led_backlight_state_t led_backlight_state;
static sl_led_effect_t led_backlight_effect;
static volatile bool led_backlight_effect_done;
static volatile bool led_backlight_timeout;

static const sl_led_t led_backlight = SYS_CNF_LED_BACKLIGHT_CONFIG;
static const uint16_t led_backlight_pattern_array_fade_in[] = SYS_CNF_LED_BACKLIGHT_FADE_IN_PATTERN;
static const uint16_t led_backlight_pattern_array_fade_out[] = SYS_CNF_LED_BACKLIGHT_FADE_OUT_PATTERN;
static const sl_led_effect_pattern_t led_backlight_fade_in_pattern = {
  .pwm_values = led_backlight_pattern_array_fade_in,
  .length = SL_LED_PATTERN_CALCULATE_LENGTH(led_backlight_pattern_array_fade_in)
};
static const sl_led_effect_pattern_t led_backlight_fade_out_pattern = {
  .pwm_values = led_backlight_pattern_array_fade_out,
  .length = SL_LED_PATTERN_CALCULATE_LENGTH(led_backlight_pattern_array_fade_out)
};

// Function definitions --------------------------------------------------------
void sl_led_backlight_init(void)
{
  int sc = sl_led_effect_init(&led_backlight_effect, &led_backlight);
  SL_ASSERT(sc == 0, "LED backlight effect initialization failed!");
}

void sl_led_backlight_cyclic(void)
{
  if (led_backlight_effect_done) {
    led_backlight_effect_done = false;
    led_backlight_update(LED_BACKLIGHT_EVENT_FADE_EFFECT_DONE);
  }
  if (led_backlight_timeout) {
    led_backlight_timeout = false;
    led_backlight_update(LED_BACKLIGHT_EVENT_STOP);
  }
  if (LED_BACKLIGHT_STATE_IDLE == led_backlight_state) {
    led_test_process(&led_backlight);
  }
}

void sl_led_backlight_on(void)
{
  if (!led_test_is_enabled()) {
    led_backlight_update(LED_BACKLIGHT_EVENT_START);
  }
}

void sl_led_backlight_off(void)
{
  if (!led_test_is_enabled()) {
    led_backlight_update(LED_BACKLIGHT_EVENT_STOP);
  }
}

bool sl_led_backlight_is_on(void)
{
  return (LED_BACKLIGHT_STATE_IDLE != led_backlight_state)
         || (led_test_is_enabled() && sl_led_get_state(&led_backlight));
}

static void led_backlight_effect_done_callback(sl_led_effect_t *effect)
{
  (void)effect;
  led_backlight_effect_done = true;
}

static void led_backlight_timeout_callback(sl_led_effect_t *effect)
{
  (void)effect;
  led_backlight_timeout = true;
}

static void led_backlight_update(led_backlight_event_t event)
{
  static bool deep_sleep_disabled;
  led_backlight_state_t last_state = led_backlight_state;
  led_backlight_state = led_backlight_state_machine(event, led_backlight_state);

  if ((last_state != led_backlight_state) //React on state change
      || (LED_BACKLIGHT_EVENT_START == event && LED_BACKLIGHT_STATE_ON == led_backlight_state)) { //or restart the timeout timer
    led_backlight_start_effects(led_backlight_state);
  }

  if ((!deep_sleep_disabled && LED_BACKLIGHT_STATE_IDLE != led_backlight_state)
      || (deep_sleep_disabled && LED_BACKLIGHT_STATE_IDLE == led_backlight_state)) {
    deep_sleep_disabled = LED_BACKLIGHT_STATE_IDLE != led_backlight_state;
    sl_sleep_deep_enable(!deep_sleep_disabled);
  }
}

static led_backlight_state_t led_backlight_state_machine(led_backlight_event_t event, led_backlight_state_t state)
{
  static bool restart_req;

  switch (state) {
    case LED_BACKLIGHT_STATE_IDLE:
      if (event == LED_BACKLIGHT_EVENT_START) {
        state = LED_BACKLIGHT_STATE_FADE_IN;
      }
      break;
    case LED_BACKLIGHT_STATE_FADE_IN:
      if (LED_BACKLIGHT_EVENT_FADE_EFFECT_DONE == event) {
        state = LED_BACKLIGHT_STATE_ON;
      }
      break;
    case LED_BACKLIGHT_STATE_ON:
      if (LED_BACKLIGHT_EVENT_STOP == event) {
        state = LED_BACKLIGHT_STATE_FADE_OUT;
      }
      break;
    case LED_BACKLIGHT_STATE_FADE_OUT:
      if (LED_BACKLIGHT_EVENT_START == event) {
        restart_req = true;
      } else if (LED_BACKLIGHT_EVENT_FADE_EFFECT_DONE == event) {
        state = restart_req ? LED_BACKLIGHT_STATE_FADE_IN : LED_BACKLIGHT_STATE_IDLE;
        restart_req = false;
      }
      break;

    default:
      break;
  }
  return state;
}

static void led_backlight_start_effects(led_backlight_state_t state)
{
  int sc = 0;

  switch (state) {
    case LED_BACKLIGHT_STATE_FADE_IN:
      sc = sl_led_effect_start(&led_backlight_effect, &led_backlight_fade_in_pattern, 1, led_backlight_effect_done_callback);
      break;
    case LED_BACKLIGHT_STATE_ON:
      sc = sl_led_effect_timer_start(&led_backlight_effect, SYS_CNF_LED_BACKLIGHT_TIMEOUT_MS, led_backlight_timeout_callback);
      break;
    case LED_BACKLIGHT_STATE_FADE_OUT:
      sc = sl_led_effect_start(&led_backlight_effect, &led_backlight_fade_out_pattern, 1, led_backlight_effect_done_callback);
      break;
    case LED_BACKLIGHT_STATE_IDLE:
    default:
      break;
  }
  SL_ASSERT(sc == 0, "LED backlight effect handling failure in state: %d error: %d!", state, sc);
}

void sl_led_backlight_test(bool test_enable, bool led_on)
{
  led_test_enable(test_enable, led_on);
  if (led_test_is_enabled() && (LED_BACKLIGHT_STATE_IDLE != led_backlight_state)) {
    led_backlight_update(LED_BACKLIGHT_EVENT_STOP);
  }
}

bool sl_led_backlight_test_is_enabled(void)
{
  return led_test_is_enabled();
}

#else  // SYS_CNF_LED_BACKLIGHT_ENABLE
void sl_led_backlight_init(void)
{
}

void sl_led_backlight_cyclic(void)
{
}

void sl_led_backlight_on(void)
{
}

void sl_led_backlight_off(void)
{
}

bool sl_led_backlight_is_on(void)
{
  return false;
}

void sl_led_backlight_test(bool test_enable, bool led_on)
{
  (void)test_enable;
  (void)led_on;
}

bool sl_led_backlight_test_is_enabled(void)
{
  return false;
}

#endif // SYS_CNF_LED_BACKLIGHT_ENABLE
