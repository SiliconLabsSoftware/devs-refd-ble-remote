/***************************************************************************//**
 * @file receiver.c
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
#define SL_LOG_MODULE_NAME "Main"
#include <string.h>
#include "receiver.h"
#include "sl_ble.h"
#include "sl_ir_nec_decoder.h"
#include "sl_activity_led.h"
#include "voice.h"
#include "sl_log.h"

// Macros ----------------------------------------------------------------------
// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
static void receiver_on_key_status_change(sl_ble_characteristic_id_t characteristic, const void *data, size_t size);

// Private variables -----------------------------------------------------------
// Function definitions --------------------------------------------------------
void sl_receiver_app_init(void)
{
  sl_activity_led_init();
  sl_ir_nec_decoder_init();
  sl_voice_init();
}

void sl_receiver_app_cyclic(void)
{
  sl_ir_nec_decoder_cyclic();
}

void sl_receiver_on_remote_connected(void)
{
  int result = sl_ble_characteristic_notify(SL_BLE_CID_KEY_STATUS, receiver_on_key_status_change);
  sl_log_status_warning(result, "BLE KEY_STATUS notify failed! res=%d" SL_LOG_EOL, result);
}

static void receiver_on_key_status_change(sl_ble_characteristic_id_t characteristic, const void *data, size_t size)
{
  (void)characteristic;
  static uint8_t last_content[10];
  const uint8_t *key_status = data;

  bool changed = false;
  if (key_status
      && (size >= 2)
      && (size <= sizeof(last_content))
      && (0 != memcmp(last_content, key_status, size))) {
    changed = true;
    memcpy(last_content, key_status, size);
  }

  if (changed && size >= 10) {
    sl_log_debug("Key info: ID=%02X, Status=%02X, Bitfield=%02X%02X %02X%02X %02X%02X %02X%02X" SL_LOG_EOL,
                 key_status[0], key_status[1], key_status[9], key_status[8],
                 key_status[7], key_status[6], key_status[5], key_status[4],
                 key_status[3], key_status[2]);
  } else if (changed) {
    sl_log_debug("Key info: ID=%02X, Status=%02X" SL_LOG_EOL,
                 key_status[0], key_status[1]);
  }
}
