/***************************************************************************//**
 * @file key_handler.h
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
#ifndef KEY_HANDLER_H
#define KEY_HANDLER_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include "../sl_system_config.h"

// Macros ----------------------------------------------------------------------
#define KEY_HANDLER_KEY_ID_INVALID       0xFF
#define KEY_HANDLER_MAX_SUPPORTED_KEYS   (sizeof(uint64_t) * 8)

// Type definitions ------------------------------------------------------------
typedef enum {
  KEY_HANDLER_STATUS_NO_KEY_PRESSED,
  KEY_HANDLER_STATUS_NEW_KEY_PRESSED,
  KEY_HANDLER_STATUS_KEY_REPEATED,
  KEY_HANDLER_STATUS_KEY_RELEASED,
  KEY_HANDLER_STATUS_MULTIPLE_KEY_PRESS, //This we can safely detect
  // This status is used when multiple keys are pressed at the same time
  //--> Key matrix has no diodes thus ghosting is possible
  //--> Detected keys from now on are not reliable
  KEY_HANDLER_STATUS_MULTIPLE_KEY_PRESS_GHOSTING, //This we cannot detect
  //
  KEY_HANDLER_STATUS_MAX_COUNT
} key_handler_status_t;

typedef enum {
  KEY_HANDLER_CONFIG_ID_ALLOWED_KEY_BITFIELD_BLE,
  KEY_HANDLER_CONFIG_ID_ALLOWED_KEY_BITFIELD_IR,
  KEY_HANDLER_CONFIG_ID_IR_ADDRESS,
  KEY_HANDLER_CONFIG_ID_IR_REPEAT_LIMIT,
  KEY_HANDLER_CONFIG_ID_IR_KEY_TO_CMD_TABLE,
  //
  KEY_HANDLER_CONFIG_ID_MAX_COUNT
} key_handler_config_id_t;

typedef struct {
  uint8_t key_id; //< ID of the first pressed key (zero based index).
  uint8_t status; //< Status of the key events.
#if SYS_CNF_KEY_HANDLER_BLE_SEND_BITFIELD
  uint8_t keys_bitfield[KEY_HANDLER_MAX_SUPPORTED_KEYS / 8]; //< Bitfield representing all pressed keys. (shall be used as uint64_t, LSB)
#endif
} key_handler_info_t;

// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------

/** @brief Initialize the key handler component.
 *
 * This function initializes the key matrix and prepares the key handler
 * for operation.
 */
void key_handler_init(void);

/** @brief Perform cyclic processing for the key handler.
 *
 * This function should be called periodically to process key matrix
 * scanning and handle key events.
 */
void key_handler_cyclic(void);

/** @brief Get information about the current key events.
 *
 * This function fills the provided key_handler_info_t structure with
 * the current status, first pressed key ID, and bitfield of all pressed keys.
 *
 * @param info Pointer to a key_handler_info_t structure to be filled.
 */
void key_handler_get_info(key_handler_info_t *info);

/** @brief Configure the key handler.
 * This function allows configuring various parameters of the key handler.
 *
 * @param id    The configuration ID to set.
 * @param data  Pointer to the data to set.
 * @param length Length of the data to set in bytes.
 * @return 0 on success, negative error code on failure.
 */
int key_handler_config_set(key_handler_config_id_t id, const void *data, size_t length);

/** @brief Get the current configuration of the key handler.
 * This function retrieves the current configuration for a specified ID.
 *
 * @param id    The configuration ID to get.
 * @param data  Pointer to the buffer where the configuration data will be stored.
 * @param length As an input length of the buffer provided in bytes as an output the actual length of the data copied.
 * @return 0 on success, negative error code on failure.
 */
int key_handler_config_get(key_handler_config_id_t id, void *data, size_t *length);

#ifdef __cplusplus
}
#endif
#endif /* KEY_HANDLER_H */
