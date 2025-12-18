/***************************************************************************//**
 * @file sl_dwt_handler.h
 * @brief DWT timer handler header file
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
#ifndef SL_DWT_HANDLER_H_
#define SL_DWT_HANDLER_H_

#include <stdint.h>
#include <stdbool.h>

/***************************************************************************//**
 * @brief Initialize the DWT cycle counter for timing operations.
 ******************************************************************************/
void sl_dwt_handler_init(void);

/***************************************************************************//**
 * @brief Deinitialize the DWT cycle counter and related debug features.
 ******************************************************************************/
void sl_dwt_handler_deinit(void);

/***************************************************************************//**
 * @brief Get the current value of the DWT cycle counter.
 *
 * @return Current DWT cycle count (32-bit value).
 ******************************************************************************/
uint32_t sl_dwt_handler_get_cycle_count(void);

/***************************************************************************//**
 * @brief Check if the specified number of milliseconds has elapsed since start_cycle.
 *
 * @param start_cycle The DWT cycle count at the start of the interval.
 * @param timeout_ms  Timeout interval in milliseconds.
 * @return true if the interval has elapsed, false otherwise.
 ******************************************************************************/
bool sl_dwt_handler_is_elapsed_ms(uint32_t start_cycle, uint32_t timeout_ms);

/***************************************************************************//**
 * @brief Block execution for a specified number of microseconds using the DWT cycle counter.
 *
 * @param timeout_us Timeout interval in microseconds.
 ******************************************************************************/
void sl_dwt_handler_blocking_delay_us(uint32_t timeout_us);

#endif // SL_DWT_HANDLER_H_
