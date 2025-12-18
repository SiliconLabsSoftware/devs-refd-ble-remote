/***************************************************************************//**
 * @file sl_ble.h
 * @brief Project specific Bluetooth Low Energy (BLE) interface.
 *
 * This file contains the function necessary to fulfill the Requirements of a
 * Remote Control Device.
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
#ifndef SL_BLE_H
#define SL_BLE_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <stddef.h>
#include "sl_ble_ids.h"

// Includes --------------------------------------------------------------------
// Macros ----------------------------------------------------------------------
// Type definitions ------------------------------------------------------------
// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------

/**
 * @brief Starts and restarts BLE advertising.
 *
 * This function initiates the Bluetooth Low Energy (BLE) advertising process,
 * making the device discoverable to other BLE devices.
 *
 * The advertising parameters are coming from the system configuration.
 * After a configured period of time the advertising will change to slow,
 * then it will stop.
 */
void sl_ble_advertising_start(void);

/**
 * @brief Reads the value of a BLE characteristic.
 *
 * This function retrieves the value of a specified BLE characteristic.
 *
 * @param characteristic The characteristic to read, defined in the enum from sl_ble_config.h.
 * @param data Pointer to the buffer where the characteristic value will be stored.
 * @param size Pointer to the size of the buffer. On input, it specifies the buffer size.
 *             On output, it contains the actual size of the data read.
 * @return 0 on success, or an error code on failure.
 */
int sl_ble_characteristic_read(sl_ble_characteristic_id_t characteristic, void *data, size_t *size);

/**
 * @brief Writes a value to a BLE characteristic.
 *
 * This function updates the value of a specified BLE characteristic.
 *
 * @param characteristic The characteristic to write, defined in the enum from sl_ble_config.h.
 * @param data Pointer to the buffer containing the data to write.
 * @param size The size of the data to write.
 * @return 0 on success, or an error code on failure.
 */
int sl_ble_characteristic_write(sl_ble_characteristic_id_t characteristic, const void *data, size_t size);

/**
 * @brief Sends a notification or indication for a BLE characteristic (depending on what configure by the central device).
 *
 * This function sends a notification containing the specified data for a given
 * BLE characteristic to connected devices that have enabled notifications.
 *
 * @param characteristic The characteristic to notify, defined in the enum from sl_ble_config.h.
 * @param data Pointer to the buffer containing the data to send in the notification.
 * @param size The size of the data to send.
 * @return 0 on success, or an error code on failure.
 */
int sl_ble_characteristic_notify(sl_ble_characteristic_id_t characteristic, const void *data, size_t size);

/**
 * @brief Callback function for handling characteristic read events.
 *
 * This function is invoked when a central device performs a read operation
 * on a BLE characteristic. It allows the application to provide the value
 * of the characteristic dynamically.
 *
 * @param characteristic The characteristic being read, defined in the enum from sl_ble_config.h.
 * @param buffer Pointer to the buffer where the characteristic value should be written.
 * @param size As an input the size of the buffer provided. The application should ensure that the value written does not exceed this size.
 *             As an output, the actual size of the data written to the buffer.
 * @return 0 on success, or an error code on failure.
 */
int sl_ble_characteristic_on_read_event(sl_ble_characteristic_id_t characteristic, void *buffer, size_t *size);

/**
 * @brief Callback function for handling characteristic write events.
 *
 * This function is invoked when a central device performs a write operation
 * on a BLE characteristic. It allows the application to process the data
 * written to the characteristic.
 *
 * @param characteristic The characteristic being written to, defined in the enum from sl_ble_config.h.
 * @param buffer Pointer to the buffer containing the data written to the characteristic.
 * @param size The size of the data written to the characteristic.
 * @return 0 on success, or an error code on failure.
 */
int sl_ble_characteristic_on_write_event(sl_ble_characteristic_id_t characteristic, const void *buffer, size_t size);

/**
 * @brief Checks if a BLE connection is currently active.
 *
 * This function returns true if the device is currently connected to a BLE central device,
 * otherwise returns false.
 *
 * @return true if connected, false otherwise.
 */
bool sl_ble_is_connected(void);

#ifdef __cplusplus
}
#endif
#endif /* SL_BLE_H */
