/***************************************************************************//**
 * @file sl_recovery_handler.h
 * @brief Recovery handler header file
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
#ifndef SL_RECOVERY_HANDLER_H_
#define SL_RECOVERY_HANDLER_H_

#include "stdbool.h"

/***************************************************************************//**
 * @brief Initialize the recovery handler module.
 *
 * This function initializes all resources required for the recovery handler,
 * including GPIO and any timers or variables needed for recovery key detection.
 ******************************************************************************/
void sl_recovery_init(void);

/***************************************************************************//**
 * @brief Deinitialize the recovery handler module.
 *
 * This function releases or disables all resources used by the recovery handler,
 * such as GPIOs and timers.
 ******************************************************************************/
void sl_recovery_deinit(void);

/***************************************************************************//**
 * @brief Verify if the recovery condition is met.
 *
 * This function checks if the required recovery key combination is pressed
 * for the specified timeout period. Returns true if the condition is met.
 *
 * @return true if recovery mode should be entered, false otherwise.
 ******************************************************************************/
bool sl_recovery_verify_condition(void);

#endif // SL_RECOVERY_HANDLER_H_
