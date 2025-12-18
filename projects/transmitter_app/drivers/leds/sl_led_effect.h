/***************************************************************************//**
 * @file sl_led_effect.h
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
#ifndef SL_LED_EFFECT_H
#define SL_LED_EFFECT_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include "sl_led.h"

// Macros ----------------------------------------------------------------------
#define SL_LED_PATTERN_MIN_TIME_MS 20 ///< Minimum time for a single pattern value in milliseconds
//
#if 20 != SL_LED_PATTERN_MIN_TIME_MS
  #error "SL_LED_PATTERN_MIN_TIME_MS must be 20 ms for SL_LED_PATTERN_SAME_VALUE_FOR_20_MS"
#endif
#define SL_LED_PATTERN_SAME_VALUE_FOR_20_MS(value) \
  (value)
#define SL_LED_PATTERN_SAME_VALUE_FOR_100_MS(value) \
  SL_LED_PATTERN_SAME_VALUE_FOR_20_MS(value),       \
  SL_LED_PATTERN_SAME_VALUE_FOR_20_MS(value),       \
  SL_LED_PATTERN_SAME_VALUE_FOR_20_MS(value),       \
  SL_LED_PATTERN_SAME_VALUE_FOR_20_MS(value),       \
  SL_LED_PATTERN_SAME_VALUE_FOR_20_MS(value)
#define SL_LED_PATTERN_SAME_VALUE_FOR_500_MS(value) \
  SL_LED_PATTERN_SAME_VALUE_FOR_100_MS(value),      \
  SL_LED_PATTERN_SAME_VALUE_FOR_100_MS(value),      \
  SL_LED_PATTERN_SAME_VALUE_FOR_100_MS(value),      \
  SL_LED_PATTERN_SAME_VALUE_FOR_100_MS(value),      \
  SL_LED_PATTERN_SAME_VALUE_FOR_100_MS(value)
#define SL_LED_PATTERN_SAME_VALUE_FOR_1000_MS(value) \
  SL_LED_PATTERN_SAME_VALUE_FOR_500_MS(value),       \
  SL_LED_PATTERN_SAME_VALUE_FOR_500_MS(value)
//
#if 20 != SL_LED_PATTERN_MIN_TIME_MS
  #error "SL_LED_PATTERN_MIN_TIME_MS must be 20 ms for SL_LED_PATTERN_FADE_20_MS"
#endif
#define SL_LED_PATTERN_FADE_20_MS(start, to_add, offset) \
  SL_LED_PATTERN_SAME_VALUE_FOR_20_MS((start) + (to_add) + (offset))
#define SL_LED_PATTERN_FADE_100_MS(start, to_add, offset)                        \
  SL_LED_PATTERN_FADE_20_MS(start + offset, (1 * to_add) / 5, (0 * to_add) / 5), \
  SL_LED_PATTERN_FADE_20_MS(start + offset, (1 * to_add) / 5, (1 * to_add) / 5), \
  SL_LED_PATTERN_FADE_20_MS(start + offset, (1 * to_add) / 5, (2 * to_add) / 5), \
  SL_LED_PATTERN_FADE_20_MS(start + offset, (1 * to_add) / 5, (3 * to_add) / 5), \
  SL_LED_PATTERN_FADE_20_MS(start + offset, (1 * to_add) / 5, (4 * to_add) / 5)
#define SL_LED_PATTERN_FADE_500_MS(start, to_add, offset)                         \
  SL_LED_PATTERN_FADE_100_MS(start + offset, (1 * to_add) / 5, (0 * to_add) / 5), \
  SL_LED_PATTERN_FADE_100_MS(start + offset, (1 * to_add) / 5, (1 * to_add) / 5), \
  SL_LED_PATTERN_FADE_100_MS(start + offset, (1 * to_add) / 5, (2 * to_add) / 5), \
  SL_LED_PATTERN_FADE_100_MS(start + offset, (1 * to_add) / 5, (3 * to_add) / 5), \
  SL_LED_PATTERN_FADE_100_MS(start + offset, (1 * to_add) / 5, (4 * to_add) / 5)
#define SL_LED_PATTERN_FADE_1000_MS(start, to_add, offset)                        \
  SL_LED_PATTERN_FADE_500_MS(start + offset, (1 * to_add) / 2, (0 * to_add) / 2), \
  SL_LED_PATTERN_FADE_500_MS(start + offset, (1 * to_add) / 2, (1 * to_add) / 2)
//
#define SL_LED_PATTERN_CALCULATE_LENGTH(pattern_array) \
  (sizeof(pattern_array) / sizeof((pattern_array)[0]))

// Type definitions ------------------------------------------------------------
typedef struct {
  uint32_t context_buffer[16]; ///< Context buffer for the LED effect, used for DMA transfers
} sl_led_effect_t;

///Pattern type definition for the LED
typedef struct {
  const uint16_t *pwm_values; ///< Pointer to the pattern array containing PWM values
  const uint16_t length; ///< Length of the pattern array.
} sl_led_effect_pattern_t;

typedef void (*sl_led_effect_callback_t)(sl_led_effect_t *effect); ///< Callback type for effectdone or timeout completion

// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------

/***************************************************************************//**
 * @brief Initialize the LED effect.
 *
 * This function initializes the LED effect by setting up the DMA channel and
 * preparing the LED instance for use.
 *
 * @param[in] effect Pointer to the sl_led_effect_t structure to initialize.
 * @param[in] led Pointer to the sl_led_t structure representing the LED to control.
 *
 * @return 0 on success, or an error code on failure.
 ******************************************************************************/
int sl_led_effect_init(sl_led_effect_t *effect, const sl_led_t *led);

/***************************************************************************//**
 * Starts the user defined pattern.
 * This function starts the LED effect with the specified pattern and repeat count.
 *
 * @param[in] effect Pointer to the sl_led_effect_t structure.
 * @param[in] pattern Pointer to the sl_led_effect_pattern_t structure containing the pattern.
 * @param[in] repeat_count Number of times to repeat the pattern. Use 0 for infinite repetition.
 * @param[in] effect_done_callback Callback function to be called when the effect is done. Called from an ISR!
 *                                 Can be NULL if no callback is needed.
 *
 * @return 0 on success, or an error code on failure.
 ******************************************************************************/
int sl_led_effect_start(sl_led_effect_t *effect, const sl_led_effect_pattern_t *pattern, uint8_t repeat_count, sl_led_effect_callback_t effect_done_callback);

/***************************************************************************//**
 * Stops the LED effect.
 * This function stops the LED effect and releases the DMA channel.
 *
 * @param[in] effect Pointer to the sl_led_effect_t structure.
 *
 * @return 0 on success, or an error code on failure.
 ******************************************************************************/
int sl_led_effect_stop(sl_led_effect_t *effect);

/***************************************************************************//**
 * Checks if the LED effect is currently running.
 *
 * @param[in] effect Pointer to the sl_led_effect_t structure.
 *
 * @return true if the effect is running, false otherwise.
 ******************************************************************************/
bool sl_led_effect_is_running(sl_led_effect_t *effect);

/***************************************************************************//**
 * Starts or restarts a timer for the LED effect.
 * This function starts a timer that will call the specified callback function
 * after the given time in milliseconds.
 * If called while the timer is already running, it will reset the timer.
 *
 * @param[in] effect Pointer to the sl_led_effect_t structure.
 * @param[in] time_ms Time in milliseconds to wait before calling the callback.
 * @param[in] timeout_callback Pointer to the callback function to be called on timeout. Called from an ISR!
 *
 * @return 0 on success, or an error code on failure.
 ******************************************************************************/
int sl_led_effect_timer_start(sl_led_effect_t *effect, uint32_t time_ms, sl_led_effect_callback_t timeout_callback);

#ifdef __cplusplus
}
#endif
#endif /* SL_LED_EFFECT_H */
