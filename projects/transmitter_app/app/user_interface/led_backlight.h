/***************************************************************************//**
 * @file led_backlight.h
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
#ifndef LED_BACKLIGHT_H
#define LED_BACKLIGHT_H
#ifdef __cplusplus
extern "C" {
#endif

// Macros ----------------------------------------------------------------------
// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
// Private variables -----------------------------------------------------------

// Function definitions --------------------------------------------------------

/***************************************************************************//**
 * @brief Initialize the LED backlight component.
 *
 * This function initializes the LED backlight component and sets up the LED
 * effect for backlight control.
 ******************************************************************************/
void sl_led_backlight_init(void);

/***************************************************************************//**
 * @brief Cyclic function for LED backlight.
 * This function should be called periodically to update the LED backlight state.
 ******************************************************************************/
void sl_led_backlight_cyclic(void);

/***************************************************************************//**
 * @brief Turn on the LED backlight.
 *
 * This function turns on the LED backlight with full brightness.
 ******************************************************************************/
void sl_led_backlight_on(void);

/***************************************************************************//**
 * @brief Turn off the LED backlight.
 *
 * This function turns off the LED backlight.
 ******************************************************************************/
void sl_led_backlight_off(void);

/***************************************************************************//**
 * @brief Check if LED backlight is currently on.
 *
 * @return true if LED backlight is on, false otherwise.
 ******************************************************************************/
bool sl_led_backlight_is_on(void);

/***************************************************************************//**
 * @brief Test function for LED backlight.
 *
 * This function is used for testing purposes to control the LED backlight
 * state during tests.
 *
 * @note This function is only available if SYS_CNF_TEST_EN_LED is set (by default in DEBUG builds).
 *
 * @param test_enable If true, enables the test mode for LED backlight.
 * @param led_on If true, turns on the LED backlight, otherwise turns it off.
 ******************************************************************************/
void sl_led_backlight_test(bool test_enable, bool led_on);

/***************************************************************************//**
 * @brief Check if the LED backlight test mode is enabled.
 * @note This function is only available if SYS_CNF_TEST_EN_LED is set (by default in DEBUG builds).
 *
 * @return true if the test mode is enabled, false otherwise.
 ******************************************************************************/
bool sl_led_backlight_test_is_enabled(void);

#ifdef __cplusplus
}
#endif
#endif /* LED_BACKLIGHT_H */
