/***************************************************************************//**
 * @file callbacks.c
 * @brief Callbacks for the application from the lower layers.
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
#define SL_LOG_MODULE_NAME "AppCallbacks"
#include <string.h>
#include <stddef.h>
#include <errno.h>
#include "sl_acc.h"
#include "sl_log.h"
#include "sl_ble.h"
#include "led_activity.h"
#include "led_backlight.h"
#include "voice.h"
#include "key_handler.h"
#include "sl_key_matrix.h"
#include "sl_battery_meas.h"

// Macros ----------------------------------------------------------------------
typedef enum {
  CALLBACK_BLE_LED_TEST_STATE_OFF,
  CALLBACK_BLE_LED_TEST_STATE_ON,
  CALLBACK_BLE_LED_TEST_STATE_AUTO,
  //
  CALLBACK_BLE_LED_TEST_STATE_MAX
} callback_ble_led_test_state_t;

// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
static int callback_ble_read_result_copy(const void *src_buffer, size_t src_size, void *dest_buffer, size_t *dest_size);
static int callback_on_voice_ble_write(sl_ble_characteristic_id_t characteristic, const uint8_t *value);
static int callback_on_battery_ble_read(sl_ble_characteristic_id_t characteristic, void *buffer, size_t *size);
static int callback_on_led_ble_write(void (*led_test_set)(bool test_enable, bool led_on), callback_ble_led_test_state_t state);
static int callback_on_led_ble_read(bool (*test_is_on)(void), bool (*led_is_on)(void), void *buffer, size_t *size);

// Private variables -----------------------------------------------------------
static key_handler_config_id_t callback_ble_last_key_config_id = KEY_HANDLER_CONFIG_ID_ALLOWED_KEY_BITFIELD_BLE;

// Function definitions --------------------------------------------------------
int sl_ble_characteristic_on_read_event(sl_ble_characteristic_id_t characteristic, void *buffer, size_t *size)
{
  int ret = -ENOSYS;

  switch (characteristic) {
    case SL_BLE_CID_TEST_BATTERY:
    case SL_BLE_CID_TEST_BATTERY_MEAS_TIME:
      ret = callback_on_battery_ble_read(characteristic, buffer, size);
      break;

    case SL_BLE_CID_TEST_ACCELEROMETER:
      if (buffer && size && *size >= (3 * sizeof(uint32_t))) {
        int32_t *acc_ug = buffer;
        *size = 3 * sizeof(uint32_t);
        ret = sl_acc_get_acceleration(&acc_ug[0], &acc_ug[1], &acc_ug[2]);
      } else {
        ret = -ENOMEM;
      }
      break;

    case SL_BLE_CID_KEY_STATUS:
      key_handler_info_t info = { 0 };
      key_handler_get_info(&info);
      ret = callback_ble_read_result_copy(&info, sizeof(info), buffer, size);
      break;
    case SL_BLE_CID_KEY_CONFIG:
      if (buffer && size && *size >= (KEY_HANDLER_MAX_SUPPORTED_KEYS + 1)) {
        ret = key_handler_config_get(callback_ble_last_key_config_id, buffer, size);
      }
      break;

    case SL_BLE_CID_TEST_LED_BACKLIGHT:
      ret = callback_on_led_ble_read(sl_led_backlight_test_is_enabled, sl_led_backlight_is_on, buffer, size);
      break;
    case SL_BLE_CID_TEST_LED_TX:
      ret = callback_on_led_ble_read(sl_led_activity_test_is_enabled, sl_led_activity_is_on, buffer, size);
      break;

    default:
      break; // Operation not supported
  }
  sl_log_status_error(ret, "BLE read error: %d" SL_LOG_EOL, ret);
  return ret;
}

int sl_ble_characteristic_on_write_event(sl_ble_characteristic_id_t characteristic, const void *buffer, size_t size)
{
  int ret = -ENOSYS;

  switch (characteristic) {
    case SL_BLE_CID_TEST_VOICE_RECORD:
    case SL_BLE_CID_VOICE_SAMPLE_RATE:
    case SL_BLE_CID_VOICE_AUDIO_CHANNELS:
    case SL_BLE_CID_VOICE_FILTER_ENABLE:
    case SL_BLE_CID_VOICE_ENCODING_ENABLE:
    case SL_BLE_CID_TEST_MIC_PDM_DELAY:
    case SL_BLE_CID_TEST_MIC_USE_LEFT:
      if (size == sizeof(uint8_t)) {
        ret = callback_on_voice_ble_write(characteristic, buffer);
      }
      break;
    case SL_BLE_CID_TEST_BATTERY_MEAS_TIME:
      if (size <= sizeof(uint32_t)) {
        ret = 0;
        uint32_t period_time_s = 0;
        memcpy(&period_time_s, buffer, size);
        sl_battery_meas_set_period_time_ms(period_time_s * 1000);
      }
      break;
    case SL_BLE_CID_TEST_ACCELEROMETER_THRESHOLD:
      if (size <= sizeof(uint32_t)) {
        uint32_t threshold_ug = 0;
        memcpy(&threshold_ug, buffer, size);
        ret = sl_acc_set_thresholds(threshold_ug, threshold_ug, threshold_ug);
      }
      break;

    case SL_BLE_CID_KEY_CONFIG:
      if (buffer && size) {
        const uint8_t *raw_data = buffer;
        callback_ble_last_key_config_id = raw_data[0];
        if (size > 1) {
          ret = key_handler_config_set(callback_ble_last_key_config_id, &raw_data[1], size - 1);
        } else {
          ret = 0; //only the ID was set, no data
        }
      }
      break;
    case SL_BLE_CID_TEST_KEY_FORCE:
      if (size >= 2) {
        const uint8_t *data = buffer;
        uint8_t key_id = data[0];
        uint32_t ir_repeat_count = data[1];
        ret = sl_key_matrix_test_inject_key_press(key_id, ir_repeat_count * 100);
      }
      break;

    case SL_BLE_CID_TEST_LED_BACKLIGHT:
      if (size) {
        ret = callback_on_led_ble_write(sl_led_backlight_test, *((uint8_t *)buffer));
      }
      break;
    case SL_BLE_CID_TEST_LED_TX:
      if (size) {
        ret = callback_on_led_ble_write(sl_led_activity_test, *((uint8_t *)buffer));
      }
      break;

    default:
      break; // Operation not supported
  }
  sl_log_status_error(ret, "BLE write error: %d" SL_LOG_EOL, ret);
  return ret;
}

static int callback_ble_read_result_copy(const void *src_buffer, size_t src_size, void *dest_buffer, size_t *dest_size)
{
  int ret = 0;

  if (src_buffer == NULL || dest_buffer == NULL || dest_size == NULL) {
    ret = -EINVAL; // Invalid argument
  } else if (*dest_size < src_size) {
    ret = -ENOMEM; // Not enough memory
  } else {
    memcpy(dest_buffer, src_buffer, src_size);
    *dest_size = src_size;
  }
  return ret;
}

static int callback_on_voice_ble_write(sl_ble_characteristic_id_t characteristic, const uint8_t *value)
{
  int sc = 0;
  bool is_recording = voice_is_recording();

  if (characteristic == SL_BLE_CID_TEST_VOICE_RECORD) {
    if (*value && !is_recording) {
      voice_start();
    } else if (!(*value) && is_recording) {
      voice_stop();
    } else {
      sc = -EINVAL; // Invalid argument
    }
  } else if (is_recording) {
    sc = -EBUSY; // Device or resource busy
  } else if (characteristic == SL_BLE_CID_VOICE_SAMPLE_RATE) {
    voice_set_sample_rate(*(uint8_t *)value);
  } else if (characteristic == SL_BLE_CID_VOICE_AUDIO_CHANNELS) {
    voice_set_channels(*(uint8_t *)value);
  } else if (characteristic == SL_BLE_CID_VOICE_FILTER_ENABLE) {
    voice_set_filter_enable(*(bool *)value);
  } else if (characteristic == SL_BLE_CID_VOICE_ENCODING_ENABLE) {
    voice_set_encoding_enable(*(bool *)value);
  } else if (characteristic == SL_BLE_CID_TEST_MIC_PDM_DELAY) {
    voice_set_mic_delay(*(uint8_t *)value);
  } else if (characteristic == SL_BLE_CID_TEST_MIC_USE_LEFT) {
    voice_set_mic_use_left(*(bool *)value);
  } else {
    sc = -ENOTSUP; // Operation not supported
  }
  return sc;
}

void sl_battery_meas_on_complete(uint32_t microvolt)
{
  sl_ble_characteristic_write(SL_BLE_CID_TEST_BATTERY, &microvolt, sizeof(microvolt));
  if (sl_ble_is_connected()) {
    sl_ble_characteristic_notify(SL_BLE_CID_TEST_BATTERY, &microvolt, sizeof(microvolt));
  }
  sl_log_info("Battery: %2.3f mV" SL_LOG_EOL, 0.001f * microvolt);
}

static int callback_on_battery_ble_read(sl_ble_characteristic_id_t characteristic, void *buffer, size_t *size)
{
  int ret = 0;
  uint32_t data = 0;

  if (SL_BLE_CID_TEST_BATTERY == characteristic) {
    data = sl_battery_meas_get_voltage_uv();
  } else if (SL_BLE_CID_TEST_BATTERY_MEAS_TIME == characteristic) {
    data = sl_battery_meas_get_period_time_ms() / 1000;
  } else {
    ret = -ENOTSUP;
  }
  if (!ret) {
    ret = callback_ble_read_result_copy(&data, sizeof(data), buffer, size);
  }
  return ret;
}

static int callback_on_led_ble_write(void (*led_test_set)(bool test_enable, bool led_on), callback_ble_led_test_state_t state)
{
  int ret = 0;

  switch (state) {
    case CALLBACK_BLE_LED_TEST_STATE_ON:
      led_test_set(true, true);
      break;
    case CALLBACK_BLE_LED_TEST_STATE_OFF:
      led_test_set(true, false);
      break;
    case CALLBACK_BLE_LED_TEST_STATE_AUTO:
      led_test_set(false, false);
      break;
    default:
      ret = -ENOTSUP; // Operation not supported
      break;
  }
  return ret;
}

static int callback_on_led_ble_read(bool (*test_is_on)(void), bool (*led_is_on)(void), void *buffer, size_t *size)
{
  uint8_t led_state = !test_is_on()
                      ? CALLBACK_BLE_LED_TEST_STATE_AUTO : led_is_on()
                      ? CALLBACK_BLE_LED_TEST_STATE_ON : CALLBACK_BLE_LED_TEST_STATE_OFF;
  return callback_ble_read_result_copy(&led_state, sizeof(led_state), buffer, size);
}
