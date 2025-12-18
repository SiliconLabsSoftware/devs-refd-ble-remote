/***************************************************************************//**
 * @file sl_ble.h
 * @brief Project specific Bluetooth Low Energy (BLE) interface (central device).
 *
 * This file contains the function necessary to fulfill the Requirements of a
 * Remote Control Receiver device.
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
#include <stdint.h>
#include <stddef.h>
#include "sl_ble_ids.h"

// Includes --------------------------------------------------------------------
// Macros ----------------------------------------------------------------------
// Type definitions ------------------------------------------------------------
typedef void (*sl_ble_characteristic_notify_callback_t)(sl_ble_characteristic_id_t characteristic, const void *data, size_t size);
typedef void (*sl_ble_characteristic_read_callback_t)(sl_ble_characteristic_id_t characteristic, const void *data, size_t size);
typedef void (*sl_ble_characteristic_write_callback_t)(sl_ble_characteristic_id_t characteristic, int result);

// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------

/**
 * @brief Sets the address filter for BLE scanning.
 *
 * This function configures the BLE stack to only accept connections from
 * a specific remote device, identified by its address.
 *
 * @param addr Pointer to the address of the remote device to filter for.
 * @param addr_len Length of the address pointed to by @p addr.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int sl_ble_set_address_filter(const uint8_t *addr, size_t addr_len);

/**
 * @brief A weakly defined callback function invoked when a remote BLE device is connected.
 *
 * This function is called when a remote Bluetooth Low Energy (BLE) device establishes a connection.
 *
 * @param addr     Pointer to the address of the connected remote device.
 * @param addr_len Length of the address pointed to by @p addr.
 */
void sl_ble_on_remote_connected(const uint8_t *addr, size_t addr_len);

/**
 * @brief Registers a read callback for a BLE characteristic.
 * This function initiates a read operation for the specified BLE characteristic.
 * When the read completes, the provided callback is invoked with the result.
 *
 * @param characteristic The characteristic to read, defined in the enum from sl_ble_config.h.
 * @param callback The callback function to be invoked when the read completes. Must not be NULL.
 * @return 0 on success, or an error code on failure.
 */
int sl_ble_characteristic_read(sl_ble_characteristic_id_t characteristic, sl_ble_characteristic_read_callback_t callback);

/**
 * @brief Writes a value to a BLE characteristic.
 * This function updates the value of a specified BLE characteristic.
 *
 * @param characteristic The characteristic to write, defined in the enum from sl_ble_config.h.
 * @param data Pointer to the buffer containing the data to write.
 * @param size The size of the data to write.
 * @param callback The callback function to be invoked when the write completes. Can be NULL if no callback is needed.
 * @return 0 on success, or an error code on failure.
 */
int sl_ble_characteristic_write(sl_ble_characteristic_id_t characteristic, const void *data, size_t size, sl_ble_characteristic_write_callback_t callback);

/**
 * @brief Registers a notification callback for a specified BLE characteristic.
 * This function enables notifications for the given BLE characteristic and sets
 * the provided callback function to be called whenever a notification is received.
 *
 * @param characteristic The identifier of the BLE characteristic to enable notifications for.
 * @param callback The callback function to be invoked when a notification is received. Use NULL to disable notifications.
 * @return 0 on success, or a negative error code on failure.
 */
int sl_ble_characteristic_notify(sl_ble_characteristic_id_t characteristic, sl_ble_characteristic_notify_callback_t callback);

#ifdef __cplusplus
}
#endif
#endif /* SL_BLE_H */
