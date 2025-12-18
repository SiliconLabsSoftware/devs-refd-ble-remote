/***************************************************************************//**
 * @file sl_activity_led.h
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
#ifndef SL_ACTIVITY_LED_H
#define SL_ACTIVITY_LED_H
#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------
// Macros ----------------------------------------------------------------------
// Type definitions ------------------------------------------------------------
typedef enum {
  SL_ACTIVITY_LED_INST_BLE,
  SL_ACTIVITY_LED_INST_IR,
  SL_ACTIVITY_LED_INST_MAX
} sl_activity_led_inst_t;

// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------
/**
 * @brief Initializes the activity LED.
 * This function initializes the activity LED and sets its initial state.
 */

void sl_activity_led_init(void);
/**
 * @brief Sets the state of the activity LED.
 * This function sets the state of the specified activity LED instance.
 *
 * @param led The LED instance to set.
 */
void sl_activity_led_set(sl_activity_led_inst_t led);

#ifdef __cplusplus
}
#endif
#endif /* SL_ACTIVITY_LED_H */
