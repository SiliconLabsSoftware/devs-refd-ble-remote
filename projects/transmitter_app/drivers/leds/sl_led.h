/***************************************************************************//**
 * @file sl_led.h
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
#ifndef SL_LED_H
#define SL_LED_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

// Macros ----------------------------------------------------------------------
#define SL_LED_MAX_LEVEL            UINT16_MAX
#define SL_LED_MIN_LEVEL            0U

// Type definitions ------------------------------------------------------------
typedef enum {
  SL_LED_POLARITY_ACTIVE_LOW,
  SL_LED_POLARITY_ACTIVE_HIGH
} sl_led_polarity_t;

typedef enum {
  SL_LED_TYPE_PWM,
  SL_LED_TYPE_GPIO
} sl_led_type_t;

typedef enum {
  SL_LED_STATE_OFF,
  SL_LED_STATE_ON
} sl_led_state_t;

typedef struct {
  uint8_t port; ///< LED's GPIO port
  uint8_t pin;  ///< LED's GPIO pin
  uint8_t tim_oc_nbr; ///< The number of the output compare channel used for the PWM, needed in case of a @ref SL_LED_TYPE_PWM
  uint16_t max_level; ///< Maximum brightness level of the LED, used for PWM
  sl_led_polarity_t polarity; ///< Polarity of the LED
  sl_led_type_t type; ///< Control type of the LED
} sl_led_t;

// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------

/***************************************************************************//**
 * Initialize the LED driver. Call this function before any other LED
 * function. Initializes the selected LED GPIO, mode, and polarity.
 *
 * @param[in] led_handle    Pointer to instance of sl_led_t to initialize
 *
 * @return 0 on success, or an error code on failure.
 ******************************************************************************/
int sl_led_init(const sl_led_t *led_handle);

/***************************************************************************//**
 * Turn on the LED with maximum brightness.
 *
 * @param[in] led_handle    Pointer to instance of sl_led_t to turn on
 ******************************************************************************/
void sl_led_turn_on(const sl_led_t *led_handle);

/***************************************************************************//**
 * Turn off the LED.
 *
 * @param[in] led_handle    Pointer to instance of sl_led_t to turn off
 ******************************************************************************/
void sl_led_turn_off(const sl_led_t *led_handle);

/***************************************************************************//**
 * Toggle the LED. Turn it on if it is off, and off if it is on.
 *
 * @param[in] led_handle    Pointer to instance of sl_led_t to toggle
 ******************************************************************************/
void sl_led_toggle(const sl_led_t *led_handle);

/***************************************************************************//**
 * Get the current state of the LED.
 *
 * @param[in] led_handle         Pointer to instance of sl_led_t to check
 *
 * @return    sl_led_state_t     Current state of LED. 1 for on, 0 for off
 ******************************************************************************/
sl_led_state_t sl_led_get_state(const sl_led_t *led_handle);

/***************************************************************************//**
 * Sets the brightness of the PWM LED.
 *
 * @param[in] led_handle    Pointer to instance of sl_led_t
 * @param[in] level         Brightness in PWM duty-cycle [0-65535]
 *
 ******************************************************************************/
void sl_led_set_level(const sl_led_t *led_handle, uint16_t level);

/***************************************************************************//**
 * Gets the level (brightness) of the PWM LED.
 *
 * @param[in] led_handle  Pointer to instance of sl_led_t
 * @return uint16_t       Current PWM duty cycle
 *
 ******************************************************************************/
uint16_t sl_led_get_level(const sl_led_t *led_handle);

/** @} (end group led) */
#ifdef __cplusplus
}
#endif
#endif // SL_LED_H
