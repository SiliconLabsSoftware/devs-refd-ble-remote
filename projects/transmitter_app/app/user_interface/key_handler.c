/***************************************************************************//**
 * @file key_handler.c
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
#define SL_LOG_MODULE_NAME "KeyHandler"
#include <errno.h>
#include <string.h>
#include "sl_key_matrix.h"
#include "key_handler.h"
#include "sl_ir_led.h"
#include "sl_ble.h"
#include "sl_atomic.h"
#include "sl_nvm.h"
#include "sl_log.h"
#include "assert.h"

// Macros ----------------------------------------------------------------------
#define KEY_HANDLER_KEY_ID_MAX           64
#define KEY_HANDLER_BLE_SEND_CYCLE_LIMIT (SYS_CNF_KEY_HANDLER_BLE_NOTIFICATION_PERIOD_MS / SYS_CNF_KEY_MATRIX_CYCLE_TIME_MS)
SL_STATIC_ASSERT(SYS_CNF_KEY_HANDLER_BLE_NOTIFICATION_PERIOD_MS % SYS_CNF_KEY_MATRIX_CYCLE_TIME_MS == 0,
                 "Key handler BLE notification period must be a multiple of key matrix cycle time (because that is used to simply determine time)!");

// Private type definitions ----------------------------------------------------
typedef struct {
  uint64_t allowed_key_bitfield_ble;
  uint64_t allowed_key_bitfield_ir;
  uint32_t ir_repeat_limit;
  uint16_t ir_address;
  uint8_t ir_key_to_command_table[KEY_HANDLER_MAX_SUPPORTED_KEYS];
} key_handler_config_t;

typedef struct {
  key_handler_config_id_t id;
  void * const data;
  size_t size;
} key_handler_config_table_t;

// Private function prototypes -------------------------------------------------
static key_handler_status_t key_handler_get_status(uint64_t press_curr, uint64_t press_prev);
static uint8_t key_handler_get_first_pressed_key(uint64_t press_curr);
static void key_handler_ir_handling(const key_handler_info_t *info);
static void key_handler_ble_handling(const key_handler_info_t *info);
static inline uint32_t key_handler_population_count(uint64_t bitfield);

// Private variables -----------------------------------------------------------
static volatile bool key_handler_event;
static uint64_t key_handler_keys_presses_curr;
static uint64_t key_handler_keys_presses_prev;
static key_handler_info_t key_handler_info;
static key_handler_config_t key_handler_config = {
  .allowed_key_bitfield_ble = SYS_CNF_KEY_HANDLER_DEF_ALLOWED_KEY_BITFIELD_BLE,
  .allowed_key_bitfield_ir = SYS_CNF_KEY_HANDLER_DEF_ALLOWED_KEY_BITFIELD_IR,
  .ir_address = SYS_CNF_KEY_HANDLER_DEF_IR_ADDRESS,
  .ir_repeat_limit = SYS_CNF_KEY_HANDLER_DEF_IR_REPEAT_LIMIT,
  .ir_key_to_command_table = SYS_CNF_KEY_HANDLER_DEF_IR_KEY_TO_COMMAND_TABLE
};

static const key_handler_config_table_t key_handler_config_table[] = {
  { KEY_HANDLER_CONFIG_ID_ALLOWED_KEY_BITFIELD_BLE, &key_handler_config.allowed_key_bitfield_ble, sizeof(key_handler_config.allowed_key_bitfield_ble) },
  { KEY_HANDLER_CONFIG_ID_ALLOWED_KEY_BITFIELD_IR, &key_handler_config.allowed_key_bitfield_ir, sizeof(key_handler_config.allowed_key_bitfield_ir) },
  { KEY_HANDLER_CONFIG_ID_IR_ADDRESS, &key_handler_config.ir_address, sizeof(key_handler_config.ir_address) },
  { KEY_HANDLER_CONFIG_ID_IR_REPEAT_LIMIT, &key_handler_config.ir_repeat_limit, sizeof(key_handler_config.ir_repeat_limit) },
  { KEY_HANDLER_CONFIG_ID_IR_KEY_TO_CMD_TABLE, &key_handler_config.ir_key_to_command_table, sizeof(key_handler_config.ir_key_to_command_table) }
};
SL_STATIC_ASSERT(KEY_HANDLER_CONFIG_ID_MAX_COUNT == (sizeof(key_handler_config_table) / sizeof(key_handler_config_table[0])),
                 "Key handler config table size mismatch!");
static const char *key_handler_status_strings[] = {
  "No key press",
  "New key press",
  "Key repeated",
  "Key released",
  "Two key press",
  "Multiple key press (ghosting)"
};
SL_STATIC_ASSERT(KEY_HANDLER_STATUS_MAX_COUNT == (sizeof(key_handler_status_strings) / sizeof(key_handler_status_strings[0])),
                 "Key handler status strings array size mismatch");

// Function definitions --------------------------------------------------------
void key_handler_init(void)
{
#if SYS_CNF_KEY_HANDLER_NON_VOLATILE_CONFIG_ENABLE
  for (size_t i = 0; i < KEY_HANDLER_CONFIG_ID_MAX_COUNT; i++) {
    int sc = sl_nvm_read(SYS_CNF_NVM_ID_OFFS_KEY_HANDLER + i, key_handler_config_table[i].data, key_handler_config_table[i].size);
    sl_log_status_debug(sc, "Failed to read NVM! ID=%u, ret=%d" SL_LOG_EOL, key_handler_config_table[i].id, sc);
  }
#endif

  sl_ir_led_init();
  sl_key_matrix_init();
}

void sl_key_matrix_on_event(uint64_t keys_pressed)
{
  key_handler_keys_presses_curr = keys_pressed;
  key_handler_event = true;
}

void key_handler_get_info(key_handler_info_t *info)
{
  if (info) {
    *info = key_handler_info;
  }
}

void key_handler_cyclic(void)
{
  uint64_t keys_presses_curr;

  if (key_handler_event) {
    SL_ATOMIC_SECTION({
      key_handler_event = false;
      keys_presses_curr = key_handler_keys_presses_curr;
    });

    key_handler_info.status = key_handler_get_status(keys_presses_curr, key_handler_keys_presses_prev);
    if (key_handler_keys_presses_prev != keys_presses_curr) {
      key_handler_info.key_id = key_handler_get_first_pressed_key(keys_presses_curr);
  #if SYS_CNF_KEY_HANDLER_BLE_SEND_BITFIELD
      memcpy(key_handler_info.keys_bitfield, &keys_presses_curr, sizeof(keys_presses_curr));
      SL_STATIC_ASSERT(sizeof(key_handler_info.keys_bitfield) == sizeof(keys_presses_curr), "Key handler info keys bitfield size mismatch!");
  #endif
      key_handler_ir_handling(&key_handler_info); //Needs start/stop underlying layer sends repeat code

      sl_log_debug("%s! First key: %u, bitfield: 0x%llX" SL_LOG_EOL,
                   key_handler_status_strings[key_handler_info.status],
                   key_handler_info.key_id,
                   keys_presses_curr);
    }
    key_handler_ble_handling(&key_handler_info); //needs to be called cyclically to send the current key state
    key_handler_keys_presses_prev = keys_presses_curr; //must be the LAST step
  }
}

static void key_handler_ir_handling(const key_handler_info_t *info)
{
  static uint8_t last_key_id = KEY_HANDLER_KEY_ID_INVALID;

  if ((KEY_HANDLER_KEY_ID_INVALID != last_key_id) && (last_key_id != info->key_id)) {
    sl_ir_led_stop();
    if (KEY_HANDLER_KEY_ID_INVALID == info->key_id) {
      last_key_id = KEY_HANDLER_KEY_ID_INVALID;  // Reset last key ID if no key is pressed
    }
  }

  if ((info->key_id < KEY_HANDLER_KEY_ID_MAX)
      && (last_key_id != info->key_id)
      && (key_handler_config.allowed_key_bitfield_ir & (1ULL << info->key_id))) {
    uint8_t command = key_handler_config.ir_key_to_command_table[info->key_id];
    int sc = key_handler_config.ir_address < 0xFF
             ? sl_ir_led_send(key_handler_config.ir_address, command, key_handler_config.ir_repeat_limit)
             : sl_ir_led_send_extended(key_handler_config.ir_address, command, key_handler_config.ir_repeat_limit);

    if (!sc) {
      last_key_id = info->key_id; // Retry next time in case of failure
    }
    sl_log_status_warning(sc, "Failed to send IR command for key %u: %d" SL_LOG_EOL, info->key_id, sc);
  }
}

static void key_handler_ble_handling(const key_handler_info_t *info)
{
  static uint8_t last_key_id = KEY_HANDLER_KEY_ID_INVALID;
  static uint32_t key_handler_ble_cycle_count = KEY_HANDLER_BLE_SEND_CYCLE_LIMIT;
  bool release_event = (KEY_HANDLER_KEY_ID_INVALID == info->key_id);
  uint8_t key_id = release_event ? last_key_id : info->key_id;

  if (sl_ble_is_connected()
      && (KEY_HANDLER_KEY_ID_INVALID != key_id)
      && (key_handler_config.allowed_key_bitfield_ble & (1ULL << key_id))) {
    if (KEY_HANDLER_BLE_SEND_CYCLE_LIMIT <= key_handler_ble_cycle_count++ || release_event) {
      key_handler_ble_cycle_count = 0; // Delay next send
      int sc = sl_ble_characteristic_notify(SL_BLE_CID_KEY_STATUS, info, sizeof(*info));
      sl_log_status_warning(sc, "Failed to send BLE notification for key %u: %d" SL_LOG_EOL, info->key_id, sc);
    }
  } else {
    key_handler_ble_cycle_count = KEY_HANDLER_BLE_SEND_CYCLE_LIMIT; // Send new press immediately
  }
  last_key_id = info->key_id;
}

static key_handler_status_t key_handler_get_status(uint64_t press_curr, uint64_t press_prev)
{
  if (press_curr == 0) {
    return KEY_HANDLER_STATUS_NO_KEY_PRESSED;
  } else {
    uint32_t count_curr = key_handler_population_count(press_curr);
    uint32_t count_prev = key_handler_population_count(press_prev);

    if (count_curr == 2) {
      return KEY_HANDLER_STATUS_MULTIPLE_KEY_PRESS;
    } else if (count_curr > 2) {
      return KEY_HANDLER_STATUS_MULTIPLE_KEY_PRESS_GHOSTING;
    } else if (press_curr == press_prev) {
      return KEY_HANDLER_STATUS_KEY_REPEATED;
    } else if (count_curr < count_prev) {
      return KEY_HANDLER_STATUS_KEY_RELEASED;
    } else {
      return KEY_HANDLER_STATUS_NEW_KEY_PRESSED;
    }
  }
}

static uint8_t key_handler_get_first_pressed_key(uint64_t press_curr)
{
  static uint8_t key_id = KEY_HANDLER_KEY_ID_INVALID;

  if (press_curr == 0) {
    key_id = KEY_HANDLER_KEY_ID_INVALID;
  } else if (   (KEY_HANDLER_KEY_ID_INVALID == key_id)
                || (0 == (press_curr & (1ULL << key_id))) ) {
    for (key_id = 0; 0 == (press_curr & (1ULL << key_id)); key_id++) {
    }
  }
  return key_id;
}

static inline uint32_t key_handler_population_count(uint64_t bitfield)
{
  bitfield = bitfield - ((bitfield >> 1) & 0x5555555555555555ULL);
  bitfield = (bitfield & 0x3333333333333333ULL) + ((bitfield >> 2) & 0x3333333333333333ULL);
  bitfield = (bitfield + (bitfield >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
  return (bitfield * 0x0101010101010101ULL) >> 56;
}

int key_handler_config_set(key_handler_config_id_t id, const void *data, size_t length)
{
  int sc = -EINVAL;

  if ((id < KEY_HANDLER_CONFIG_ID_MAX_COUNT)
      && (NULL != data)
      && (length == key_handler_config_table[id].size)) {
#if SYS_CNF_KEY_HANDLER_NON_VOLATILE_CONFIG_ENABLE
    sc = sl_nvm_write(SYS_CNF_NVM_ID_OFFS_KEY_HANDLER + id, data, length);
#else
    sc = 0; // No NVM write, just update the in-memory config
#endif
    if (!sc) {
      memcpy(key_handler_config_table[id].data, data, length);
    }
  }
  return sc;
}

int key_handler_config_get(key_handler_config_id_t id, void *data, size_t *length)
{
  int sc = -EINVAL;

  if ((id < KEY_HANDLER_CONFIG_ID_MAX_COUNT)
      && (NULL != data)
      && (length != NULL)
      && (*length >= key_handler_config_table[id].size)) {
    sc = 0;
    *length = key_handler_config_table[id].size;
    memcpy(data, key_handler_config_table[id].data, key_handler_config_table[id].size);
  }
  return sc;
}
