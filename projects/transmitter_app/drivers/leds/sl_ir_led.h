/***************************************************************************//**
 * @file sl_ir_led.h
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
#ifndef SL_IR_LED_H
#define SL_IR_LED_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

// Includes --------------------------------------------------------------------
// Macros ----------------------------------------------------------------------
#define SL_IR_LED_REPEAT_LIMIT_INFINITE    UINT32_MAX

// Type definitions ------------------------------------------------------------
// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------

/***************************************************************************//**
 * @brief Initialize the IR LED driver.
 ******************************************************************************/
void sl_ir_led_init(void);

/***************************************************************************//**
 * @brief Sends a standard NEC IR frame. Address and command are 8-bit values.
 * The negated address and command are also sent.
 *
 * Needs to be called when the key is pressed.
 * A repeat code will be sent automatically until @ref sl_ir_led_stop is called.
 *
 * @param[in] raw_data     The 32-bit raw data to be sent.
 * @param[in] repeat_limit  The maximum number of times to send the repeat code.
 *                          Use @ref SL_IR_LED_REPEAT_LIMIT_INFINITE for infinite repeat.
 *
 * @return 0 on success, negative value on error.
 ******************************************************************************/
int sl_ir_led_send(uint8_t address, uint8_t command, uint32_t repeat_limit);

/***************************************************************************//**
 * @brief Sends an extended NEC IR frame.
 * Address is 16bit and command is 8-bit (a negated command is sent).
 *
 * Needs to be called when the key is pressed.
 * A repeat code will be sent automatically until @ref sl_ir_led_stop is called.
 *
 * @param[in] address      The 16-bit address to be sent.
 * @param[in] command      The 16-bit command to be sent.
 * @param[in] repeat_limit  The maximum number of times to send the repeat code.
 *                          Use @ref SL_IR_LED_REPEAT_LIMIT_INFINITE for infinite repeat.
 *
 * @return 0 on success, negative value on error.
 ******************************************************************************/
int sl_ir_led_send_extended(uint16_t address, uint8_t command, uint32_t repeat_limit);

/***************************************************************************//**
 * @brief Sends a custom 32 bit NEC IR frame.
 *
 * Needs to be called when the key is pressed.
 * A repeat code will be sent automatically until @ref sl_ir_led_stop is called.
 *
 * @param[in] raw_data     The 32-bit raw data to be sent.
 * @param[in] repeat_limit  The maximum number of times to send the repeat code.
 *                          Use @ref SL_IR_LED_REPEAT_LIMIT_INFINITE for infinite repeat.
 *
 * @return 0 on success, negative value on error.
 ******************************************************************************/
int sl_ir_led_send_raw(uint32_t raw_data, uint32_t repeat_limit);

/***************************************************************************//**
 * @brief Stop sending the IR signal.
 * This function will stop sending the repeat code.
 * Needs to be called when the key is released.
 ******************************************************************************/
void sl_ir_led_stop(void);

#ifdef __cplusplus
}
#endif
#endif /* SL_IR_LED_H */
