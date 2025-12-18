/***************************************************************************//**
 * @file remote.h
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
#ifndef REMOTE_H
#define REMOTE_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

// Macros ----------------------------------------------------------------------
// Type definitions ------------------------------------------------------------
// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------

/**
 * @brief Initialize the remote application.
 *
 * This function sets up the necessary components for the remote application
 * to function correctly.
 *
 * @param cold_start If true, indicates a cold start of the application,
 *                   which may require additional initialization steps.
 */
void sl_remote_app_init(bool cold_start);

/**
 * @brief Perform cyclic tasks for the remote application.
 *
 * This function is called periodically to handle cyclic operations
 * required by the remote application.
 */
void sl_remote_app_cyclic(void);

#ifdef __cplusplus
}
#endif
#endif /* REMOTE_H */
