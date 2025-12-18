/***************************************************************************//**
 * @file
 * @brief
 * @version 1.0.0
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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
#ifndef SL_SYSTEM_CONFIG_H
#define SL_SYSTEM_CONFIG_H
#ifdef __cplusplus
extern "C" {
#endif
#include "sl_transmitter_hw_config.h"

// Utility macros -------------------------------------------------------------
// This macro is used to set/get a port and pin number into a single 16-bit value.
#define SYS_CNF_PORT_PIN_SET(port, pin)        HW_CNF_PORT_PIN_SET(port, pin)
#define SYS_CNF_PORT_PIN_GET_PORT(port_pin)    HW_CNF_PORT_PIN_GET_PORT(port_pin)
#define SYS_CNF_PORT_PIN_GET_PIN(port_pin)     HW_CNF_PORT_PIN_GET_PIN(port_pin)
#define SYS_CNF_ARR_LEN(x)                     (sizeof(x) / sizeof(x[0]))

// Key matrix driver ----------------------------------------------------------
#define SYS_CNF_KEY_MATRIX_TIMER_TIME_OUT_CHANGE_SETTLE_US  HW_CNF_KEY_MATRIX_TIMER_TIME_OUT_CHANGE_SETTLE_US
#define SYS_CNF_KEY_MATRIX_ROWS                HW_CNF_KEY_MATRIX_ROWS
#define SYS_CNF_KEY_MATRIX_COLS                HW_CNF_KEY_MATRIX_COLS

// Recovery handler driver ----------------------------------------------------
#define SYS_CNF_RECOVERY_ENTER_TIMEOUT_MS      10000

// Calculating from the number of the switches is faster than using the GPIO pin numbers directly.
// The switch positions are defined in the reference design, and the matrix is defined as follows:
// COL0 COL2 ... COL7
// SW01 SW02 ... SW08 ROW0
// SW10 SW12 ... SW18 ROW1
// .......................
// SW41 SW42 ... SW48 ROW5
#if HW_CNF_HW_VERSION_IS(HW_CNF_HW_VERSION_THUNDERBOARD)
  #define SYS_CNF_RECOVERY_KEY_SW_COMBINATION  { 1, 4 }
#else
  #define SYS_CNF_RECOVERY_KEY_SW_COMBINATION  { 28, 48 }
#endif

// LED handler driver -----------------------------------------------------
#define SYS_CNF_LED_ACTIVITY_CONFIG {                                            \
    HW_CNF_PORT_PIN_SET(HW_CNF_LED_ACTIVITY_IO_PORT, HW_CNF_LED_ACTIVITY_IO_PIN) \
}

// Apploader configuration -----------------------------------------------------
#define SYS_CNF_INCATIVITY_TIMEOUT_MS           30000
#define SYS_CNF_LED_BLINK_PERIOD_CONNECTION_MS  500
#define SYS_CNF_LED_BLINK_PERIOD_OTA_MS         100

#ifdef __cplusplus
}
#endif
#endif // SL_SYSTEM_CONFIG_H
