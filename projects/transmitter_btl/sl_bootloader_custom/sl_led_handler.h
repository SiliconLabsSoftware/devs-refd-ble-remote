/***************************************************************************//**
 * @file sl_led_handler.h
 * @brief LED handler header file
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc.  Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.  This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#ifndef SL_LED_HANDLER_H_
#define SL_LED_HANDLER_H_

/***************************************************************************//**
 * @brief Initialize the LED handler.
 *
 * This function configures the GPIO used for the LED and prepares it for use.
 ******************************************************************************/
void sl_led_handler_init(void);

/***************************************************************************//**
 * @brief Deinitialize the LED handler and release resources.
 *
 * This function disables the GPIO used for the LED.
 ******************************************************************************/
void sl_led_handler_deinit(void);

/***************************************************************************//**
 * @brief Turn the LED on.
 *
 * Sets the GPIO pin to the active state to turn the LED on.
 ******************************************************************************/
void sl_led_on(void);

/***************************************************************************//**
 * @brief Turn the LED off.
 *
 * Sets the GPIO pin to the inactive state to turn the LED off.
 ******************************************************************************/
void sl_led_off(void);

/***************************************************************************//**
 * @brief Toggle the LED state.
 *
 * Changes the current state of the LED (on to off, or off to on).
 ******************************************************************************/
void sl_led_toggle(void);

#endif // SL_LED_HANDLER_H_
