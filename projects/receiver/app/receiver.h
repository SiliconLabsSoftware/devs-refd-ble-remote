/***************************************************************************//**
 * @file receiver.h
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
#ifndef RECEIVER_H
#define RECEIVER_H
#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------
// Macros ----------------------------------------------------------------------
// Type definitions ------------------------------------------------------------
// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------
/**
 * @brief Initialize the receiver application.
 *
 * This function sets up the necessary components for the receiver application
 * to function correctly.
 */
void sl_receiver_app_init(void);

/**
 * @brief Perform cyclic tasks for the receiver application.
 *
 * This function is called periodically to handle cyclic operations
 * required by the receiver application.
 */
void sl_receiver_app_cyclic(void);

/**
 * @brief Callback function invoked when the remote device is connected.
 *
 * This function is called when a connection to a remote device is established.
 * It can be used to perform any necessary setup or initialization related to
 * the remote connection.
 */
void sl_receiver_on_remote_connected(void);

#ifdef __cplusplus
}
#endif
#endif /* RECEIVER_H */
