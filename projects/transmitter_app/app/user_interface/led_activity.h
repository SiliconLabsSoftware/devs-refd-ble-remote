/***************************************************************************//**
 * @file led_activity.h
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
#ifndef LED_ACTIVITY_H
#define LED_ACTIVITY_H
#ifdef __cplusplus
extern "C" {
#endif

// Macros ----------------------------------------------------------------------
// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
// Private variables -----------------------------------------------------------

// Function definitions --------------------------------------------------------

/***************************************************************************//**
 * @brief Initialize the LED activity component.
 *
 * This function initializes the LED activity component and sets up the LED
 * effect for activity indication.
 ******************************************************************************/
void sl_led_activity_init(void);

/***************************************************************************//**
 * @brief Cyclic function for LED activity.
 * This function should be called periodically to update the LED activity state.
 ******************************************************************************/
void sl_led_activity_cyclic(void);

/***************************************************************************//**
 * @brief Start LED activity indication.
 *
 * This function starts the LED activity pattern to indicate system activity.
 ******************************************************************************/
void sl_led_activity_start(void);

/***************************************************************************//**
 * @brief Stop LED activity indication.
 *
 * This function stops the LED activity pattern.
 ******************************************************************************/
void sl_led_activity_stop(void);

/***************************************************************************//**
 * @brief Check if LED activity is currently on.
 *
 * @return true if LED activity is on, false otherwise.
 ******************************************************************************/
bool sl_led_activity_is_on(void);

/***************************************************************************//**
 * @brief Test function for LED activity.
 *
 * This function is used for testing purposes to control the LED activity
 * state during tests.
 *
 * @note This function is only available if SYS_CNF_TEST_EN_LED is set (by default in DEBUG builds).
 *
 * @param test_enable If true, enables the test mode for LED activity.
 * @param led_on If true, turns on the LED activity, otherwise turns it off.
 ******************************************************************************/
void sl_led_activity_test(bool test_enable, bool led_on);

/***************************************************************************//**
 * @brief Check if LED activity test mode is enabled.
 * @note This function is only available if SYS_CNF_TEST_EN_LED is set (by default in DEBUG builds).
 *
 * @return true if LED activity test mode is enabled, false otherwise.
 ******************************************************************************/
bool sl_led_activity_test_is_enabled(void);

#ifdef __cplusplus
}
#endif
#endif /* LED_ACTIVITY_H */
