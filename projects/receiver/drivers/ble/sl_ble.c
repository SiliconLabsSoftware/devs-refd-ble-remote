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
#include "assert.h"
#include "sl_ble.h"
#include "sl_log.h"
#include "sl_assert.h"
#include "sl_activity_led.h"
#include "sl_system_config.h"

// Macros ----------------------------------------------------------------------
#ifndef ARRAY_LEN
  #define ARRAY_LEN(x) (sizeof(x) / sizeof((x)[0]))
#endif

#define BLE_UUID_SIZE 16
#define BLE_MAX_EXPECTED_CHAR_ID 128

#define ble_log_info_mac_addr(header_str, mac)                \
  sl_log_info("%s: %02X:%02X:%02X:%02X:%02X:%02X" SL_LOG_EOL, \
              header_str, mac.addr[5], mac.addr[4], mac.addr[3], mac.addr[2], mac.addr[1], mac.addr[0])

#define ble_scan_start() sl_bt_scanner_start(sl_bt_scanner_scan_phy_1m_and_coded, sl_bt_scanner_discover_generic)
#define ble_scan_stop()  sl_bt_scanner_stop()

// Private type definitions ----------------------------------------------------
typedef enum {
  BLE_GATT_OP_TYPE_READ,
  BLE_GATT_OP_TYPE_WRITE,
  BLE_GATT_OP_TYPE_NOTIFY,
  //
  BLE_GATT_OP_TYPE_MAX_COUNT
} ble_gatt_op_type_t;

// Private function prototypes -------------------------------------------------
static void ble_init(void);
static bool ble_filter_address(const bd_addr *addr);
static bool ble_filter_name(const uint8_t *adv_data, size_t adv_data_len, const char *name, size_t name_len);
static void ble_connect(const bd_addr *addr);
static void ble_handle_gatt_discovery(sl_bt_msg_t *evt);
static uint32_t ble_handle_gatt_discovery_get_new_service_map_idx(uint32_t current_service_id);
static int ble_gatt_queue_add(ble_gatt_op_type_t type, sl_ble_characteristic_id_t cid, const void *data, size_t size, void *callback);
static void ble_gatt_queue_pop(void);

// Private variables -----------------------------------------------------------
static bool ble_gatt_discovered;
static uint8_t ble_current_connection = SL_BT_INVALID_CONNECTION_HANDLE;
static uint32_t ble_cid_serv_id_map[SL_BLE_CID_MAX_COUNT];
static uint16_t ble_cid_char_id_map[SL_BLE_CID_MAX_COUNT];
static sl_ble_characteristic_id_t ble_char_id_cid_map[BLE_MAX_EXPECTED_CHAR_ID];
static sl_ble_characteristic_notify_callback_t ble_cid_notify_callback[SL_BLE_CID_MAX_COUNT];
static size_t ble_remote_addr_filter_len;
static bd_addr ble_remote_addr_filter;
static struct {
  volatile bool in_progress;
  sl_ble_characteristic_read_callback_t rd_callback;
  sl_ble_characteristic_write_callback_t wr_callback;
  sl_ble_characteristic_id_t wr_characteristic;
} ble_gatt_op;
static struct {
  uint32_t rd_idx;
  uint32_t wr_idx;
  struct {
    ble_gatt_op_type_t type;
    sl_ble_characteristic_id_t cid;
    uint8_t *data;
    size_t size;
    void *callback;
  } queue[SYS_CNF_BLE_GATT_QUEUE_SIZE];
} ble_gatt_queue;

static const uint8_t ble_cid_uuid_char_map[][BLE_UUID_SIZE] = SYS_CNF_BLE_CID_TO_CHAR_UUID_MAP;
static const uint8_t ble_cid_uuid_serv_map[][BLE_UUID_SIZE] = SYS_CNF_BLE_CID_TO_SERV_UUID_MAP;
SL_STATIC_ASSERT(sizeof(ble_cid_uuid_char_map) / BLE_UUID_SIZE == SL_BLE_CID_MAX_COUNT, "ble_cid_uuid_char_map length must be SL_BLE_CID_MAX_COUNT");
SL_STATIC_ASSERT(sizeof(ble_cid_uuid_serv_map) / BLE_UUID_SIZE == SL_BLE_CID_MAX_COUNT, "ble_cid_uuid_serv_map length must be SL_BLE_CID_MAX_COUNT");

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
      ble_current_connection = evt->data.evt_connection_opened.connection;
      sl_bt_sm_increase_security(ble_current_connection);
      if (!ble_gatt_discovered) {
        ble_gatt_op.in_progress = true;
        SL_ASSERT(sl_bt_gatt_discover_primary_services(ble_current_connection) == SL_STATUS_OK);
      } else {
        sl_ble_on_remote_connected(evt->data.evt_connection_opened.address.addr, sizeof(evt->data.evt_connection_opened.address.addr));
      }
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
      ble_current_connection = SL_BT_INVALID_CONNECTION_HANDLE;
      memset(ble_cid_notify_callback, 0, sizeof(ble_cid_notify_callback));
      ble_scan_start();
      break;

    case sl_bt_evt_scanner_legacy_advertisement_report_id:
      if (ble_filter_address(&evt->data.evt_scanner_legacy_advertisement_report.address)
          && ble_filter_name(evt->data.evt_scanner_legacy_advertisement_report.data.data,
                             evt->data.evt_scanner_legacy_advertisement_report.data.len,
                             SYS_CNF_BLE_REMOTE_NAME,
                             sizeof(SYS_CNF_BLE_REMOTE_NAME) - 1)) {
        ble_scan_stop();
        ble_log_info_mac_addr("Found device: " SYS_CNF_BLE_REMOTE_NAME, evt->data.evt_scanner_legacy_advertisement_report.address);
        ble_connect(&evt->data.evt_scanner_legacy_advertisement_report.address);
      }
      break;

    case sl_bt_evt_sm_bonded_id:
      sl_log_info("Bonded with %d!" SL_LOG_EOL, evt->data.evt_sm_bonded.bonding);
      break;

    case sl_bt_evt_gatt_service_id:
    case sl_bt_evt_gatt_characteristic_id:
      ble_handle_gatt_discovery(evt);
      break;

    case sl_bt_evt_gatt_procedure_completed_id:
      ble_handle_gatt_discovery(evt);
      if (ble_gatt_discovered) {
        sl_log_debug("GATT procedure completed! Result: 0x%x" SL_LOG_EOL, evt->data.evt_gatt_procedure_completed.result);
        if (ble_gatt_op.wr_callback) {
          ble_gatt_op.wr_callback(ble_gatt_op.wr_characteristic, evt->data.evt_gatt_procedure_completed.result);
          ble_gatt_op.wr_callback = NULL;
        }
        ble_gatt_op.rd_callback = NULL;
        ble_gatt_op.in_progress = false;
        ble_gatt_queue_pop();
      }
      break;

    case sl_bt_evt_gatt_characteristic_value_id:
      bool cid_range_ok = (evt->data.evt_gatt_characteristic_value.characteristic < ARRAY_LEN(ble_char_id_cid_map));
      SL_ASSERT(cid_range_ok, "Characteristic ID out of range: %d" SL_LOG_EOL, evt->data.evt_gatt_characteristic_value.characteristic);
      sl_ble_characteristic_id_t cid = cid_range_ok ? ble_char_id_cid_map[evt->data.evt_gatt_characteristic_value.characteristic] : SL_BLE_CID_INVALID;

      if (sl_bt_gatt_handle_value_notification == evt->data.evt_gatt_characteristic_value.att_opcode) {
        if (ble_cid_notify_callback[cid]) {
          ble_cid_notify_callback[cid](cid, evt->data.evt_gatt_characteristic_value.value.data, evt->data.evt_gatt_characteristic_value.value.len);
          sl_activity_led_set(SL_ACTIVITY_LED_INST_BLE);
        }
      } else if (ble_gatt_op.rd_callback) {
        ble_gatt_op.rd_callback(cid, evt->data.evt_gatt_characteristic_value.value.data, evt->data.evt_gatt_characteristic_value.value.len);
      }
      break;

    //indifferent events which not to be logged (at the moment)
    case sl_bt_evt_gatt_server_characteristic_status_id:
    case sl_bt_evt_connection_remote_used_features_id:
    case sl_bt_evt_connection_phy_status_id:
    case sl_bt_evt_connection_data_length_id:

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
  uint16_t max_mtu_out = SYS_CNF_BLE_MAX_MTU;

  // Set maximal MTU for GATT Server.
  sl_status_t sc = sl_bt_gatt_server_set_max_mtu(max_mtu_out, &max_mtu_out);
  SL_ASSERT(sc == SL_STATUS_OK && max_mtu_out == SYS_CNF_BLE_MAX_MTU);

  // Start scanning
  sc = ble_scan_start();
  SL_ASSERT(sc == SL_STATUS_OK);
}

int sl_ble_set_address_filter(const uint8_t *addr, size_t addr_len)
{
  int sc = SL_STATUS_INVALID_PARAMETER;

  if (addr && addr_len <= sizeof(bd_addr)) {
    sc = SL_STATUS_OK;
    memcpy(ble_remote_addr_filter.addr, addr, addr_len);
    ble_remote_addr_filter_len = addr_len;
  }
  return sc;
}

SL_WEAK void sl_ble_on_remote_connected(const uint8_t *addr, size_t addr_len)
{
  (void)addr;
  (void)addr_len;
}

static bool ble_filter_address(const bd_addr *addr)
{
  bool found = true;

  if (ble_remote_addr_filter_len && addr) {
    found = (0 == memcmp(addr->addr, ble_remote_addr_filter.addr, ble_remote_addr_filter_len));
  }
  return found;
}

static bool ble_filter_name(const uint8_t *adv_data, size_t adv_data_len, const char *name, size_t name_len)
{
  bool found = false;

  if (adv_data && adv_data_len && name && name_len) {
    size_t i = 0;
    while (i < adv_data_len && (i + adv_data[i]) < adv_data_len && adv_data[i] != 0 && !found) {
      uint32_t length = adv_data[i];
      uint32_t type = adv_data[i + 1];
      if (type == 0x08 || type == 0x09) { // Shortened/Complete Local Name
        found = (length == (name_len + 1) && memcmp(&adv_data[i + 2], name, name_len) == 0);
      }
      i += (length + 1);
    }
  }
  return found;
}

static void ble_connect(const bd_addr *addr)
{
  SL_ASSERT(addr != NULL);
  sl_status_t sc = sl_bt_connection_open(*addr, sl_bt_gap_public_address, sl_bt_gap_1m_phy, &ble_current_connection);
  SL_ASSERT(sc == SL_STATUS_OK);
  SL_ASSERT(ble_current_connection != SL_BT_INVALID_CONNECTION_HANDLE);
}

static void ble_handle_gatt_discovery(sl_bt_msg_t *evt)
{
  static uint32_t current_service_id;

  switch (SL_BT_MSG_ID(evt->header)) {
    case sl_bt_evt_gatt_service_id:
      for (uint32_t i = 0; i < SL_BLE_CID_MAX_COUNT; i++) {
        if (memcmp(evt->data.evt_gatt_service.uuid.data, ble_cid_uuid_serv_map[i], BLE_UUID_SIZE) == 0) {
          ble_cid_serv_id_map[i] = evt->data.evt_gatt_service.service;
        }
      }
      break;

    case sl_bt_evt_gatt_characteristic_id:
      for (uint32_t i = 0; i < SL_BLE_CID_MAX_COUNT; i++) {
        if (   0 == memcmp(evt->data.evt_gatt_characteristic.uuid.data, ble_cid_uuid_char_map[i], BLE_UUID_SIZE)
               && current_service_id == ble_cid_serv_id_map[i]) {
          ble_cid_char_id_map[i] = evt->data.evt_gatt_characteristic.characteristic;
          if (evt->data.evt_gatt_characteristic.characteristic < ARRAY_LEN(ble_char_id_cid_map)) {
            ble_char_id_cid_map[evt->data.evt_gatt_characteristic.characteristic] = i;
          } else {
            SL_ASSERT(false, "Characteristic ID out of range: %d" SL_LOG_EOL, evt->data.evt_gatt_characteristic.characteristic);
          }
        }
      }
      break;

    case sl_bt_evt_gatt_procedure_completed_id:
      if (0 == current_service_id) {
        sl_log_info("GATT service discovery completed!" SL_LOG_EOL);
        current_service_id = ble_cid_serv_id_map[1];
        sl_status_t sc = sl_bt_gatt_discover_characteristics(ble_current_connection, current_service_id);
        SL_ASSERT(sc == SL_STATUS_OK);
      } else if (UINT32_MAX != current_service_id) {
        uint32_t new_idx = ble_handle_gatt_discovery_get_new_service_map_idx(current_service_id);

        if (new_idx != UINT32_MAX) {
          current_service_id = ble_cid_serv_id_map[new_idx];
          sl_status_t sc = sl_bt_gatt_discover_characteristics(ble_current_connection, current_service_id);
          SL_ASSERT(sc == SL_STATUS_OK);
        } else {
          ble_gatt_discovered = true;
          current_service_id = UINT32_MAX;
          sl_log_info("GATT service discovery completed! No more services to discover!" SL_LOG_EOL);

          bd_addr addr = { 0 };
          sl_bt_connection_get_remote_address(ble_current_connection, &addr, sl_bt_gap_public_address);
          sl_ble_on_remote_connected(addr.addr, sizeof(addr.addr));
        }
      }
      break;
  }
}

static uint32_t ble_handle_gatt_discovery_get_new_service_map_idx(uint32_t current_service_id)
{
  uint32_t new_idx = UINT32_MAX;

  for (uint32_t i = 0; i < SL_BLE_CID_MAX_COUNT && UINT32_MAX == new_idx; i++) {
    if (ble_cid_serv_id_map[i] != 0
        && ble_cid_serv_id_map[i] != current_service_id
        && ble_cid_char_id_map[i] == 0) {
      new_idx = i;
    }
  }
  return new_idx;
}

int sl_ble_characteristic_read(sl_ble_characteristic_id_t characteristic, sl_ble_characteristic_read_callback_t callback)
{
  sl_status_t sc = SL_STATUS_INVALID_PARAMETER;

  if (characteristic < SL_BLE_CID_MAX_COUNT && callback) {
    if (ble_gatt_op.in_progress) {
      sc = ble_gatt_queue_add(BLE_GATT_OP_TYPE_READ, characteristic, NULL, 0, callback);
    } else {
      sc = sl_bt_gatt_read_characteristic_value(ble_current_connection, ble_cid_char_id_map[characteristic]);
      ble_gatt_op.in_progress = sc ? false : true;
      ble_gatt_op.rd_callback = callback;
    }
  }
  sl_log_status_debug(sc, "sl_ble_characteristic_read: char=%u, result=%ld" SL_LOG_EOL, characteristic, sc);
  return sc;
}

int sl_ble_characteristic_write(sl_ble_characteristic_id_t characteristic, const void *data, size_t size, sl_ble_characteristic_write_callback_t callback)
{
  sl_status_t sc = SL_STATUS_INVALID_PARAMETER;

  if (characteristic < SL_BLE_CID_MAX_COUNT && data && size) {
    if (ble_gatt_op.in_progress) {
      sc = ble_gatt_queue_add(BLE_GATT_OP_TYPE_WRITE, characteristic, data, size, callback);
    } else {
      sc = sl_bt_gatt_write_characteristic_value(ble_current_connection, ble_cid_char_id_map[characteristic], size, data);
      ble_gatt_op.in_progress = sc ? false : true;
      ble_gatt_op.wr_callback = callback;
      ble_gatt_op.wr_characteristic = characteristic;
    }
  }
  sl_log_status_debug(sc, "sl_ble_characteristic_write: char=%u, size=%u, result=%ld" SL_LOG_EOL, characteristic, (unsigned)size, sc);
  return sc;
}

int sl_ble_characteristic_notify(sl_ble_characteristic_id_t characteristic, sl_ble_characteristic_notify_callback_t callback)
{
  sl_status_t sc = SL_STATUS_INVALID_PARAMETER;

  if (characteristic < SL_BLE_CID_MAX_COUNT) {
    if (ble_gatt_op.in_progress) {
      sc = ble_gatt_queue_add(BLE_GATT_OP_TYPE_NOTIFY, characteristic, NULL, 0, callback);
    } else {
      ble_cid_notify_callback[characteristic] = callback;
      sc = sl_bt_gatt_set_characteristic_notification(ble_current_connection,
                                                      ble_cid_char_id_map[characteristic],
                                                      callback ? sl_bt_gatt_notification : sl_bt_gatt_disable);
      ble_gatt_op.in_progress = sc ? false : true;
    }
  }
  sl_log_status_debug(sc, "sl_ble_characteristic_notify: char=%u, result=%ld" SL_LOG_EOL, characteristic, sc);
  return sc;
}

static int ble_gatt_queue_add(ble_gatt_op_type_t type, sl_ble_characteristic_id_t cid, const void *data, size_t size, void *callback)
{
  int sc = SL_STATUS_ALLOCATION_FAILED;
  uint32_t next_wr_idx = (ble_gatt_queue.wr_idx + 1) % ARRAY_LEN(ble_gatt_queue.queue);

  if (next_wr_idx != ble_gatt_queue.rd_idx && ble_gatt_queue.queue[ble_gatt_queue.wr_idx].type < BLE_GATT_OP_TYPE_MAX_COUNT) {
    ble_gatt_queue.queue[ble_gatt_queue.wr_idx].type = type;
    ble_gatt_queue.queue[ble_gatt_queue.wr_idx].cid = cid;
    ble_gatt_queue.queue[ble_gatt_queue.wr_idx].data = (void *)data;
    ble_gatt_queue.queue[ble_gatt_queue.wr_idx].size = size;
    ble_gatt_queue.queue[ble_gatt_queue.wr_idx].callback = callback;
    ble_gatt_queue.wr_idx = next_wr_idx;
    sc = SL_STATUS_OK;
  }
  return sc;
}

static void ble_gatt_queue_pop(void)
{
  int sc = 1;

  while (sc && ble_gatt_queue.rd_idx != ble_gatt_queue.wr_idx) {
    ble_gatt_op_type_t type = ble_gatt_queue.queue[ble_gatt_queue.rd_idx].type;

    if (BLE_GATT_OP_TYPE_READ == type) {
      sc = sl_ble_characteristic_read(ble_gatt_queue.queue[ble_gatt_queue.rd_idx].cid, ble_gatt_queue.queue[ble_gatt_queue.rd_idx].callback);
    } else if (BLE_GATT_OP_TYPE_WRITE == type) {
      sc = sl_ble_characteristic_write(ble_gatt_queue.queue[ble_gatt_queue.rd_idx].cid,
                                       ble_gatt_queue.queue[ble_gatt_queue.rd_idx].data,
                                       ble_gatt_queue.queue[ble_gatt_queue.rd_idx].size,
                                       ble_gatt_queue.queue[ble_gatt_queue.rd_idx].callback);
    } else if (BLE_GATT_OP_TYPE_NOTIFY == type) {
      sc = sl_ble_characteristic_notify(ble_gatt_queue.queue[ble_gatt_queue.rd_idx].cid, ble_gatt_queue.queue[ble_gatt_queue.rd_idx].callback);
    } else {
      SL_ASSERT(false, "Invalid BLE operation type");
    }
    ble_gatt_queue.rd_idx = (ble_gatt_queue.rd_idx + 1) % ARRAY_LEN(ble_gatt_queue.queue);
  }
}

SL_STATIC_ASSERT(1 == SL_BT_CONFIG_MAX_CONNECTIONS, "Only one connection is supported!");
