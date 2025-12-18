/***************************************************************************//**
 * @file sl_ble_ids.h
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
#ifndef SL_BLE_IDS_H
#define SL_BLE_IDS_H
#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------
// Macros ----------------------------------------------------------------------
// Type definitions ------------------------------------------------------------
typedef enum {
  SL_BLE_CID_INVALID,
  //
  SL_BLE_CID_VOICE_AUDIO_DATA,
  SL_BLE_CID_VOICE_SAMPLE_RATE,
  SL_BLE_CID_VOICE_FILTER_ENABLE,
  SL_BLE_CID_VOICE_ENCODING_ENABLE,
  SL_BLE_CID_VOICE_TRANSFER_STATUS,
  SL_BLE_CID_VOICE_AUDIO_CHANNELS,
  SL_BLE_CID_KEY_STATUS,
  SL_BLE_CID_KEY_CONFIG,
  SL_BLE_CID_TEST_BATTERY,
  SL_BLE_CID_TEST_BATTERY_MEAS_TIME,
  SL_BLE_CID_TEST_VOICE_RECORD,
  SL_BLE_CID_TEST_MIC_USE_LEFT,
  SL_BLE_CID_TEST_MIC_PDM_DELAY,
  SL_BLE_CID_TEST_LED_TX,
  SL_BLE_CID_TEST_LED_BACKLIGHT,
  SL_BLE_CID_TEST_KEY_FORCE,
  SL_BLE_CID_TEST_ACCELEROMETER,
  SL_BLE_CID_TEST_ACCELEROMETER_THRESHOLD,
  //
  SL_BLE_CID_MAX_COUNT,
} sl_ble_characteristic_id_t;

// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------

#ifdef __cplusplus
}
#endif
#endif /* SL_BLE_IDS_H */
