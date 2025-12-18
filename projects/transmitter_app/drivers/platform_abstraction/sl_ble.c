/***************************************************************************//**
 * @file sl_ble.c
 * @brief
 * @version 1.0.0
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
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
#define SL_LOG_MODULE_NAME "BT"
#include "sl_bluetooth_config.h"
#include "sl_bt_api.h"
#include "gatt_db.h"
#include "sl_common.h"
#include "assert.h"
#include "sl_ble.h"
#include "sl_log.h"
#include "sl_assert.h"
#include "sl_system_config.h"

// Macros ----------------------------------------------------------------------
#define ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))

#if DEBUG
  #define BLE_MAX_CONNECTIONS SL_BT_CONFIG_MAX_CONNECTIONS
#else
  #define BLE_MAX_CONNECTIONS 1
#endif
#define BLE_ATT_ERROR_NONE              0x00 //Success
#define BLE_ATT_ERROR_READ              0x02 //The permissions prohibit reading the attribute’s value.
#define BLE_ATT_ERROR_WRITE             0x03 //The permissions prohibit writing the attribute’s value.
#define BLE_ATT_ERROR_REQ_NOT_SUPPORTED 0x06 //The attribute server doesn’t support the request received from the client.
#define BLE_ATT_ERROR_UNLIKELY_EVENT    0x0E //The ATT request encountered an unlikely error and wasn’t completed.

// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
static void ble_init(void);
static void ble_advertising_on_timeout(uint8_t handle);
static void ble_advertising_config(uint8_t *handle, uint32_t interval_ms, uint32_t timeout_ms);
static void ble_connection_change_parameters(uint8_t connection);
static uint8_t ble_connection_get_handle_index(uint8_t connection);

// Private variables -----------------------------------------------------------
static int8_t ble_current_connection_count;
static uint8_t ble_current_connections[BLE_MAX_CONNECTIONS];
static const uint8_t *ble_advertising_curr_handle;
static uint8_t ble_advertising_fast_handle = SL_BT_INVALID_ADVERTISING_SET_HANDLE;
static uint8_t ble_advertising_slow_handle = SL_BT_INVALID_ADVERTISING_SET_HANDLE;
static const uint8_t ble_cid_gatt_map[] = SYS_CNF_BLE_CHARACTERISTIC_MAP;
static const uint8_t ble_gatt_cid_map[] = SYS_CNF_BLE_CHARACTERISTIC_MAP_INVERSE;
SL_STATIC_ASSERT(SL_BLE_CID_MAX_COUNT == (sizeof(ble_cid_gatt_map) / sizeof(ble_cid_gatt_map[0])), "Invalid size of characteristic map!");

// Function definitions --------------------------------------------------------
void sl_bt_on_event(sl_bt_msg_t *evt)
{
  switch (SL_BT_MSG_ID(evt->header)) {
    case sl_bt_evt_system_boot_id:
      // This event indicates the device has started and the radio is ready.
      // Do not call any stack command before receiving this boot event!
      sl_log_info("System boot! Stack version: %d.%d.%d (build %d)" SL_LOG_EOL,
                  evt->data.evt_system_boot.major,
                  evt->data.evt_system_boot.minor,
                  evt->data.evt_system_boot.patch,
                  evt->data.evt_system_boot.build);
      ble_init();
      break;
    case sl_bt_evt_connection_opened_id:
      sl_log_info("Connection %d opened!" SL_LOG_EOL, evt->data.evt_connection_opened.connection);
      ble_connection_change_parameters(evt->data.evt_connection_opened.connection);
      ble_advertising_curr_handle = NULL; //Advertising is stopped
      ble_current_connections[ble_connection_get_handle_index(SL_BT_INVALID_CONNECTION_HANDLE)] = evt->data.evt_connection_opened.connection;
      ble_current_connection_count++;
      SL_ASSERT(ble_current_connection_count <= BLE_MAX_CONNECTIONS, "Invalid connection count!");
      sl_ble_advertising_start();
      break;
    case sl_bt_evt_connection_parameters_id:
      sl_log_debug("Connection parameters!" SL_LOG_EOL
                   " interval: %d ms" SL_LOG_EOL
                   " latency: %d" SL_LOG_EOL
                   " timeout: %d ms" SL_LOG_EOL
                   " security mode: %u" SL_LOG_EOL,
                   (evt->data.evt_connection_parameters.interval * 5 / 4),
                   evt->data.evt_connection_parameters.latency,
                   (evt->data.evt_connection_parameters.timeout * 10),
                   evt->data.evt_connection_parameters.security_mode);
      break;
    case sl_bt_evt_gatt_mtu_exchanged_id:
      sl_log_debug("MTU: %d" SL_LOG_EOL, evt->data.evt_gatt_mtu_exchanged.mtu);
      break;
    case sl_bt_evt_connection_closed_id:
      sl_log_info("Connection %d closed!" SL_LOG_EOL, evt->data.evt_connection_closed.connection);
      ble_current_connections[ble_connection_get_handle_index(evt->data.evt_connection_closed.connection)] = SL_BT_INVALID_CONNECTION_HANDLE;
      ble_current_connection_count--;
      SL_ASSERT(ble_current_connection_count >= 0, "Invalid connection count!");
      if ((BLE_MAX_CONNECTIONS - 1) == ble_current_connection_count) {
        sl_ble_advertising_start();
      }
      break;
    case sl_bt_evt_advertiser_timeout_id:
      ble_advertising_on_timeout(evt->data.evt_advertiser_timeout.handle);
      break;
    case sl_bt_evt_connection_set_parameters_failed_id:
      sl_log_error("Connection %d set parameters failed!" SL_LOG_EOL, evt->data.evt_connection_set_parameters_failed.connection);
      break;

    case sl_bt_evt_gatt_server_user_write_request_id: {
      sl_log_debug("GATT write request: %d!" SL_LOG_EOL, evt->data.evt_gatt_server_user_write_request.characteristic);
      uint8_t att_err = BLE_ATT_ERROR_REQ_NOT_SUPPORTED;
      const sl_bt_evt_gatt_server_user_write_request_t *wr_req = &evt->data.evt_gatt_server_user_write_request;
      if (ARRAY_LENGTH(ble_gatt_cid_map) > wr_req->characteristic && wr_req->offset == 0) {
        int sc = sl_ble_characteristic_on_write_event(ble_gatt_cid_map[wr_req->characteristic], wr_req->value.data, wr_req->value.len);
        att_err = sc ? BLE_ATT_ERROR_WRITE : BLE_ATT_ERROR_NONE;
      }
      sl_bt_gatt_server_send_user_write_response(wr_req->connection, wr_req->characteristic, att_err);
      break;
    }
    case sl_bt_evt_gatt_server_user_read_request_id: {
      sl_log_debug("GATT read request: %d!" SL_LOG_EOL, evt->data.evt_gatt_server_user_read_request.characteristic);
      static uint8_t rd_buf[SYS_CNF_BLE_MAX_READ_SIZE];
      size_t rd_buf_size = sizeof(rd_buf);
      uint8_t att_err = BLE_ATT_ERROR_REQ_NOT_SUPPORTED;
      const sl_bt_evt_gatt_server_user_read_request_t *rd_req = &evt->data.evt_gatt_server_user_read_request;

      if (ARRAY_LENGTH(ble_gatt_cid_map) > rd_req->characteristic && rd_req->offset == 0) {
        int sc = sl_ble_characteristic_on_read_event(ble_gatt_cid_map[rd_req->characteristic], rd_buf, &rd_buf_size);
        att_err = sc ? BLE_ATT_ERROR_READ : BLE_ATT_ERROR_NONE;
      }
      sl_bt_gatt_server_send_user_read_response(rd_req->connection, rd_req->characteristic, att_err, rd_buf_size, rd_buf, NULL);
      break;
    }

    //indifferent events which not to be logged (at the moment)
    case sl_bt_evt_gatt_server_characteristic_status_id:
    case sl_bt_evt_connection_remote_used_features_id:
    case sl_bt_evt_connection_phy_status_id:
    case sl_bt_evt_connection_data_length_id:
    case sl_bt_evt_sm_bonded_id:
      break;

    case sl_bt_evt_system_resource_exhausted_id:
      sl_log_error("System resource exhausted!" SL_LOG_EOL);
      break;
    case sl_bt_evt_system_error_id:
      sl_log_error("System error! Reason: 0x%X" SL_LOG_EOL, evt->data.evt_system_error.reason);
      break;

    default:
      sl_log_debug("Unhandled event: 0x%04lX" SL_LOG_EOL, SL_BT_MSG_ID(evt->header));
      break;
  }
}

static void ble_init(void)
{
  uint16_t max_mtu_out = 250;

  sl_log_info("Performing initial setup..." SL_LOG_EOL);
  sl_status_t sc = sl_bt_gatt_server_set_max_mtu(max_mtu_out, &max_mtu_out); // Set maximal MTU for GATT Server.
  SL_ASSERT(sc == SL_STATUS_OK && max_mtu_out == 250);

  for (int i = 0; i < BLE_MAX_CONNECTIONS; i++) {
    ble_current_connections[i] = SL_BT_INVALID_CONNECTION_HANDLE;
  }
  ble_advertising_config(&ble_advertising_fast_handle, SYS_CNF_BLE_ADV_FAST_INTERVAL_MS, SYS_CNF_BLE_ADV_FAST_TIMEOUT_MS);
  ble_advertising_config(&ble_advertising_slow_handle, SYS_CNF_BLE_ADV_SLOW_INTERVAL_MS, SYS_CNF_BLE_ADV_SLOW_TIMEOUT_MS);
  sl_ble_advertising_start();
}

void sl_ble_advertising_start(void)
{
  if (ble_current_connection_count < BLE_MAX_CONNECTIONS && ble_advertising_curr_handle != &ble_advertising_fast_handle) {
    if (ble_advertising_curr_handle) {
      sl_bt_advertiser_stop(*ble_advertising_curr_handle);
    }
    sl_status_t sc = sl_bt_legacy_advertiser_start(ble_advertising_fast_handle, sl_bt_legacy_advertiser_connectable);
    ble_advertising_curr_handle = sc ? NULL : &ble_advertising_fast_handle;
    sl_log_status_info(!sc, "Starting advertising..." SL_LOG_EOL);
  }
}

static void ble_advertising_on_timeout(uint8_t handle)
{
  if (handle == ble_advertising_fast_handle) {
    sl_log_debug("Switching to slow advertising..." SL_LOG_EOL);
    ble_advertising_curr_handle = &ble_advertising_slow_handle;
    sl_bt_legacy_advertiser_start(*ble_advertising_curr_handle, sl_bt_legacy_advertiser_connectable);
  } else {
    sl_log_debug("Stopping advertising..." SL_LOG_EOL);
    ble_advertising_curr_handle = NULL;
  }
}

static void ble_advertising_config(uint8_t *handle, uint32_t interval_ms, uint32_t timeout_ms)
{
  uint32_t interval = (interval_ms * 1000) / 625;

  sl_status_t sc = sl_bt_advertiser_create_set(handle);
  sc |= sl_bt_legacy_advertiser_generate_data(*handle, sl_bt_advertiser_general_discoverable);
  sc |= sl_bt_advertiser_set_timing(*handle, interval, interval, timeout_ms / 10, 0);
  SL_ASSERT(sc == SL_STATUS_OK);
}

static void ble_connection_change_parameters(uint8_t connection)
{
  sl_status_t sc = sl_bt_connection_set_parameters(connection,
                                                   (SYS_CNF_BLE_CONN_INTERVAL_MIN_MS * 4) / 5,
                                                   (SYS_CNF_BLE_CONN_INTERVAL_MAX_MS * 4) / 5,
                                                   SYS_CNF_BLE_CONN_LATENCY,
                                                   ((1 + SYS_CNF_BLE_CONN_LATENCY) * SYS_CNF_BLE_CONN_INTERVAL_MAX_MS) * 3 / 10,
                                                   0,
                                                   0xFFFF);
  SL_ASSERT(sc == SL_STATUS_OK, "Failed to set connection parameters! Error: 0x%X" SL_LOG_EOL, sc);
}

static uint8_t ble_connection_get_handle_index(uint8_t connection)
{
  int8_t idx = 0;
  for (; (ble_current_connections[idx] != connection) && (idx < (BLE_MAX_CONNECTIONS - 1)); idx++) {
    // Nothing to do here, just searching for the index with the given connection handle
  }
  return idx;
}

int sl_ble_characteristic_read(sl_ble_characteristic_id_t characteristic, void *data, size_t *size)
{
  sl_status_t sc = SL_STATUS_INVALID_PARAMETER;

  if (characteristic < SL_BLE_CID_MAX_COUNT && data && size) {
    uint16_t gatt_characteristic = ble_cid_gatt_map[characteristic];
    sc = sl_bt_gatt_server_read_attribute_value(gatt_characteristic, 0, *size, size, data);
  }
  return sc;
}

int sl_ble_characteristic_write(sl_ble_characteristic_id_t characteristic, const void *data, size_t size)
{
  sl_status_t sc = SL_STATUS_INVALID_PARAMETER;

  if (characteristic < SL_BLE_CID_MAX_COUNT && data && size) {
    uint16_t gatt_characteristic = ble_cid_gatt_map[characteristic];
    sc = sl_bt_gatt_server_write_attribute_value(gatt_characteristic, 0, size, data);
  }
  return sc;
}

int sl_ble_characteristic_notify(sl_ble_characteristic_id_t characteristic, const void *data, size_t size)
{
  sl_status_t sc = SL_STATUS_INVALID_PARAMETER;

  if (characteristic < SL_BLE_CID_MAX_COUNT && data && size && ble_current_connection_count) {
    uint16_t gatt_characteristic = ble_cid_gatt_map[characteristic];

    sc = 0;
    for (int i = 0; i < BLE_MAX_CONNECTIONS; i++) {
      if (ble_current_connections[i] != SL_BT_INVALID_CONNECTION_HANDLE) {
        sc |= sl_bt_gatt_server_send_notification(ble_current_connections[i], gatt_characteristic, size, data);
      }
    }
  }
  return sc;
}

bool sl_ble_is_connected(void)
{
  return (ble_current_connection_count > 0);
}

SL_WEAK int sl_ble_characteristic_on_read_event(sl_ble_characteristic_id_t characteristic, void *buffer, size_t *size)
{
  (void)characteristic;
  (void)buffer;
  (void)size;
  return -1;
}

SL_WEAK int sl_ble_characteristic_on_write_event(sl_ble_characteristic_id_t characteristic, const void *buffer, size_t size)
{
  (void)characteristic;
  (void)buffer;
  (void)size;
  return -1;
}
