/***************************************************************************//**
 * @file
 * @brief
 * @version 1.0.0
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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
#ifndef SL_SYSTEM_CONFIG_H
#define SL_SYSTEM_CONFIG_H
#ifdef __cplusplus
extern "C" {
#endif
#include "../common/src/sl_transmitter_hw_config.h"

// Utility macros -------------------------------------------------------------
#ifndef CAT4
  #define _CAT4(a, b, c, d) a##b##c##d
  #define CAT4(a, b, c, d)  _CAT4(a, b, c, d)
#endif
#ifndef CAT3
  #define CAT3(a, b, c)  CAT4(a, b, c, )
#endif
#ifndef CAT2
  #define CAT2(a, b)  CAT3(a, b, )
#endif
#define SYS_CNF_PORT_PIN_GET_PORT(port_pin) HW_CNF_PORT_PIN_GET_PORT(port_pin)
#define SYS_CNF_PORT_PIN_GET_PIN(port_pin)  HW_CNF_PORT_PIN_GET_PIN(port_pin)

// Tests ---------------------------------------------------------------------
#if DEBUG
  #define SYS_CNF_TEST_EN_LED             1
  #define SYS_CNF_TEST_EN_KEYS            1
#endif

// DCDC configuration ---------------------------------------------------------
#if HW_CNF_HW_VERSION_IS(HW_CNF_HW_VERSION_REMOTE_V1)
  #define SYS_CNF_DCDC_BYPASS_ENABLE      1
#else
  #define SYS_CNF_DCDC_BYPASS_ENABLE      0
#endif

// NVM IDs -------------------------------------------------------------------
#define SYS_CNF_NVM_ID_OFFS_KEY_HANDLER   0

// Remote (main application) configuration -----------------------------------
// !!!Button ID is as per the schematic but with 0 based index (e.g. schematic_id - 1) !!!
#if HW_CNF_HW_VERSION_IS(HW_CNF_HW_VERSION_THUNDERBOARD)
  #define SYS_CNF_REMOTE_VOICE_BUTTON_ID                    3
#else
  #define SYS_CNF_REMOTE_VOICE_BUTTON_ID                    28
#endif
#define SYS_CNF_REMOTE_PICK_UP_DETECTION_EN_IN_STBY         1

// Key matrix driver ----------------------------------------------------------
#define SYS_CNF_KEY_MATRIX_CYCLE_TIME_MS                    50 /* Time between key matrix scans when a key is being pressed (otherwise will wait for GPIO IRQ)*/
#define SYS_CNF_KEY_MATRIX_USE_LETIMER                      1  /* If 0 sleeptimer will be used */
#define SYS_CNF_KEY_MATRIX_USE_INTERNAL_PULL_UP             1  /* If 0 external pull-up resistors are used */
#define SYS_CNF_KEY_MATRIX_ROWS                             HW_CNF_KEY_MATRIX_ROWS
#define SYS_CNF_KEY_MATRIX_COLS                             HW_CNF_KEY_MATRIX_COLS
#define SYS_CNF_KEY_MATRIX_TIMER_TIME_OUT_CHANGE_SETTLE_US  HW_CNF_KEY_MATRIX_TIMER_TIME_OUT_CHANGE_SETTLE_US
// Key handler  ---------------------------------------------------------------
#define SYS_CNF_KEY_HANDLER_BLE_SEND_BITFIELD               1
#define SYS_CNF_KEY_HANDLER_BLE_NOTIFICATION_PERIOD_MS      100
#define SYS_CNF_KEY_HANDLER_NON_VOLATILE_CONFIG_ENABLE      1
#define _SYS_CNF_KEY_HANDLER_DEF_KEY_BITFIELD_DONT_SEND     (1ULL << SYS_CNF_REMOTE_VOICE_BUTTON_ID)
#define SYS_CNF_KEY_HANDLER_DEF_ALLOWED_KEY_BITFIELD_BLE    (UINT64_MAX & ~_SYS_CNF_KEY_HANDLER_DEF_KEY_BITFIELD_DONT_SEND)
#define SYS_CNF_KEY_HANDLER_DEF_ALLOWED_KEY_BITFIELD_IR     (UINT64_MAX & ~_SYS_CNF_KEY_HANDLER_DEF_KEY_BITFIELD_DONT_SEND)
#define SYS_CNF_KEY_HANDLER_DEF_IR_REPEAT_LIMIT             UINT16_MAX
#define SYS_CNF_KEY_HANDLER_DEF_IR_ADDRESS                  0xA5
#define SYS_CNF_KEY_HANDLER_DEF_IR_KEY_TO_COMMAND_TABLE {           \
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,           \
    16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, \
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, \
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63  \
}

// BLE driver -----------------------------------------------------------------
#define SYS_CNF_BLE_ADV_FAST_INTERVAL_MS  100
#define SYS_CNF_BLE_ADV_FAST_TIMEOUT_MS   30000
#define SYS_CNF_BLE_ADV_SLOW_INTERVAL_MS  1000
#if DEBUG
  #define SYS_CNF_BLE_ADV_SLOW_TIMEOUT_MS 0
  #define SYS_CNF_BLE_CONN_LATENCY        5
#else
  #define SYS_CNF_BLE_ADV_SLOW_TIMEOUT_MS 60000
  #define SYS_CNF_BLE_CONN_LATENCY        30
#endif
#define SYS_CNF_BLE_CONN_INTERVAL_MIN_MS  20
#define SYS_CNF_BLE_CONN_INTERVAL_MAX_MS  20
#define SYS_CNF_BLE_MAX_READ_SIZE         96
#define SYS_CNF_BLE_CHARACTERISTIC_MAP {                                             \
    [SL_BLE_CID_VOICE_AUDIO_DATA] = gattdb_voice_audio_data,                         \
    [SL_BLE_CID_VOICE_SAMPLE_RATE] = gattdb_voice_sample_rate,                       \
    [SL_BLE_CID_VOICE_FILTER_ENABLE] = gattdb_voice_filter_enable,                   \
    [SL_BLE_CID_VOICE_ENCODING_ENABLE] = gattdb_voice_encoding_enable,               \
    [SL_BLE_CID_VOICE_TRANSFER_STATUS] = gattdb_voice_transfer_status,               \
    [SL_BLE_CID_VOICE_AUDIO_CHANNELS] = gattdb_voice_audio_channels,                 \
    [SL_BLE_CID_KEY_STATUS] = gattdb_key_status,                                     \
    [SL_BLE_CID_KEY_CONFIG] = gattdb_key_config,                                     \
    [SL_BLE_CID_TEST_BATTERY] = gattdb_test_battery,                                 \
    [SL_BLE_CID_TEST_BATTERY_MEAS_TIME] = gattdb_test_battery_meas_time,             \
    [SL_BLE_CID_TEST_VOICE_RECORD] = gattdb_test_voice_record,                       \
    [SL_BLE_CID_TEST_MIC_USE_LEFT] = gattdb_test_mic_use_left,                       \
    [SL_BLE_CID_TEST_MIC_PDM_DELAY] = gattdb_test_mic_pdm_delay,                     \
    [SL_BLE_CID_TEST_LED_TX] = gattdb_test_led_tx,                                   \
    [SL_BLE_CID_TEST_LED_BACKLIGHT] = gattdb_test_led_backlight,                     \
    [SL_BLE_CID_TEST_KEY_FORCE] = gattdb_test_key_force,                             \
    [SL_BLE_CID_TEST_ACCELEROMETER] = gattdb_test_accelerometer,                     \
    [SL_BLE_CID_TEST_ACCELEROMETER_THRESHOLD] = gattdb_test_accelerometer_threshold, \
}
#define SYS_CNF_BLE_CHARACTERISTIC_MAP_INVERSE {                                     \
    [gattdb_voice_audio_data] = SL_BLE_CID_VOICE_AUDIO_DATA,                         \
    [gattdb_voice_sample_rate] = SL_BLE_CID_VOICE_SAMPLE_RATE,                       \
    [gattdb_voice_filter_enable] = SL_BLE_CID_VOICE_FILTER_ENABLE,                   \
    [gattdb_voice_encoding_enable] = SL_BLE_CID_VOICE_ENCODING_ENABLE,               \
    [gattdb_voice_transfer_status] = SL_BLE_CID_VOICE_TRANSFER_STATUS,               \
    [gattdb_voice_audio_channels] = SL_BLE_CID_VOICE_AUDIO_CHANNELS,                 \
    [gattdb_key_status] = SL_BLE_CID_KEY_STATUS,                                     \
    [gattdb_key_config] = SL_BLE_CID_KEY_CONFIG,                                     \
    [gattdb_test_battery] = SL_BLE_CID_TEST_BATTERY,                                 \
    [gattdb_test_battery_meas_time] = SL_BLE_CID_TEST_BATTERY_MEAS_TIME,             \
    [gattdb_test_voice_record] = SL_BLE_CID_TEST_VOICE_RECORD,                       \
    [gattdb_test_mic_use_left] = SL_BLE_CID_TEST_MIC_USE_LEFT,                       \
    [gattdb_test_mic_pdm_delay] = SL_BLE_CID_TEST_MIC_PDM_DELAY,                     \
    [gattdb_test_led_tx] = SL_BLE_CID_TEST_LED_TX,                                   \
    [gattdb_test_led_backlight] = SL_BLE_CID_TEST_LED_BACKLIGHT,                     \
    [gattdb_test_key_force] = SL_BLE_CID_TEST_KEY_FORCE,                             \
    [gattdb_test_accelerometer] = SL_BLE_CID_TEST_ACCELEROMETER,                     \
    [gattdb_test_accelerometer_threshold] = SL_BLE_CID_TEST_ACCELEROMETER_THRESHOLD, \
}

// Voice ----------------------------------------------------------------------
#define SYS_CNF_VOICE_ENCODE_SEND_STATE   1 /* Send encoding state to the host, so even after a lost package the decoding is possible (payload is increased with a few bytes) */
#define SYS_CNF_VOICE_DEF_SAMPLE_RATE     sr_16k
#define SYS_CNF_VOICE_DEF_CHANNELS        1
#define SYS_CNF_VOICE_DEF_FILTER          true
#define SYS_CNF_VOICE_DEF_ENCODE          true
#define SYS_CNF_VOICE_DEF_DELAY           0
#define SYS_CNF_VOICE_DEF_USE_LEFT_MIC    false
#if defined(HW_CNF_MIC_IO_ENABLE_PORT) && defined(HW_CNF_MIC_IO_ENABLE_PIN)
  #define SYS_CNF_VOICE_MIC_ENABLE_PORT   HW_CNF_MIC_IO_ENABLE_PORT
  #define SYS_CNF_VOICE_MIC_ENABLE_PIN    HW_CNF_MIC_IO_ENABLE_PIN
#endif
#define SYS_CNF_MIC_IO_PDM_CLK_PORT       HW_CNF_MIC_IO_PDM_CLK_PORT
#define SYS_CNF_MIC_IO_PDM_CLK_PIN        HW_CNF_MIC_IO_PDM_CLK_PIN
#define SYS_CNF_MIC_IO_PDM_DATA_PORT      HW_CNF_MIC_IO_PDM_DATA_PORT
#define SYS_CNF_MIC_IO_PDM_DATA_PIN       HW_CNF_MIC_IO_PDM_DATA_PIN

// IR LED ---------------------------------------------------------------------
#define SYS_CNF_IR_TIMER_CASCADED_MODE    1 /*The demodulated signal has more stable period times if the timebases are in sync*/
#if HW_CNF_HW_VERSION_IS(HW_CNF_HW_VERSION_THUNDERBOARD)
  #define SYS_CNF_IR_TIMER_MODULATOR_NUM  3
  #define SYS_CNF_IR_TIMER_CARRIER_NUM    2
#else
  #define SYS_CNF_IR_TIMER_MODULATOR_NUM  4
  #define SYS_CNF_IR_TIMER_CARRIER_NUM    3
#endif
#define SYS_CNF_IR_LED_IO_PORT            HW_CNF_IR_LED_IO_PORT
#define SYS_CNF_IR_LED_IO_PIN             HW_CNF_IR_LED_IO_PIN

// Battery measurement --------------------------------------------------------
#define SYS_CNF_BATTERY_MEAS_DEF_PERIOD_MS    (600 * 1000)

// Accelerometer --------------------------------------------------------------
#define SYS_CNF_ACC_WAKEUP_THRESHOLD_UG       (100 * 1000)
#define SYS_CNF_ACC_INT_AUTO_CLEAR_EN         1
#define SYS_CNF_ACC_INT_CLEAR_IS_MANDATORY    (HW_CNF_ACC_SENSOR_TYPE == HW_CNF_ACC_SENSOR_TYPE_ADXL367)
#define SYS_CNF_ACC_INT_CLEAR_DELAY_MS        (SYS_CNF_LED_BACKLIGHT_TIMEOUT_MS / 2) /* Delay before clearing interrupt (to avoid frequent irq handling)*/
// ADXL367 specific configuration ------------------------------------------
#define SYS_CNF_ACC_ADXL367_RESOLUTION_G      2
#define SYS_CNF_ACC_ADXL367_SAMPLE_RATE_HZ    12
#define SYS_CNF_ACC_ADXL367_WAKE_UP_RATE_MS   640 /* Wake-up sample rate in milliseconds, valid values: 80, 160, 320, 640 */
#define SYS_CNF_ACC_ADXL367_ACTIVITY_TIME     2 /* Number of samples needs to be larger than threshold to report activity */
#define SYS_CNF_ACC_ADXL367_I2C_NUM           0
#define SYS_CNF_ACC_ADXL367_I2C_FREQ          I2C_FREQ_FAST_MAX
// LIS2Dx12 common configuration ----------------------------------------------
#define SYS_CNF_ACC_LIS2Dx12_RESOLUTION_G     2
#define SYS_CNF_ACC_LIS2Dx12_I2C_NUM          0
#define SYS_CNF_ACC_LIS2Dx12_I2C_FREQ         I2C_FREQ_STANDARD_MAX
#define SYS_CNF_ACC_LIS2Dx12_I2C_SDA_PORT     HW_CNF_ACC_LIS2Dx12_I2C_SDA_PORT
#define SYS_CNF_ACC_LIS2Dx12_I2C_SDA_PIN      HW_CNF_ACC_LIS2Dx12_I2C_SDA_PIN
#define SYS_CNF_ACC_LIS2Dx12_I2C_SCL_PORT     HW_CNF_ACC_LIS2Dx12_I2C_SCL_PORT
#define SYS_CNF_ACC_LIS2Dx12_I2C_SCL_PIN      HW_CNF_ACC_LIS2Dx12_I2C_SCL_PIN
#define SYS_CNF_ACC_LIS2Dx12_INT_PORT         HW_CNF_ACC_LIS2Dx12_INT_PORT
#define SYS_CNF_ACC_LIS2Dx12_INT_PIN          HW_CNF_ACC_LIS2Dx12_INT_PIN
// LIS2DW12 specific configuration --------------------------------------------
// Sample rate here is fixed for minimum power consumption (1.6 Hz)
#define SYS_CNF_ACC_LIS2DW12_BDW_ODR_DIV      2
#define SYS_CNF_ACC_LIS2DW12_FILTER_PATH_HIGH 1
#define SYS_CNF_ACC_LIS2DW12_LOW_NOISE_EN     1 /* +0.02 uA if enabled*/
#define SYS_CNF_ACC_LIS2DW12_EVT_DURATION_ODR 1
// LIS2DE12 specific configuration --------------------------------------------
#define SYS_CNF_ACC_LIS2DE12_SAMPLE_RATE_HZ   10
#define SYS_CNF_ACC_LIS2DE12_EVT_DURATION_ODR 10
#define SYS_CNF_ACC_LIS2DE12_HP_FILTER_MODE   LIS2DE12_AGGRESSIVE
// ICM-20648 specific configuration -------------------------------------------
#define SYS_CNF_ACC_ICM_20648_USART_NUM       0
#define SYS_CNF_ACC_ICM_20648_SPI_BAUDRATE    3300000
#define SYS_CNF_ACC_ICM_20648_RESOLUTION_G    2
#define SYS_CNF_ACC_ICM_20648_SAMPLE_RATE_HZ  10
#define SYS_CNF_ACC_ICM_20648_BANDWIDTH_HZ    111
#define SYS_CNF_ACC_ICM_20648_SPI_MOSI_PORT   HW_CNF_ACC_ICM_20648_SPI_MOSI_PORT
#define SYS_CNF_ACC_ICM_20648_SPI_MOSI_PIN    HW_CNF_ACC_ICM_20648_SPI_MOSI_PIN
#define SYS_CNF_ACC_ICM_20648_SPI_MISO_PORT   HW_CNF_ACC_ICM_20648_SPI_MISO_PORT
#define SYS_CNF_ACC_ICM_20648_SPI_MISO_PIN    HW_CNF_ACC_ICM_20648_SPI_MISO_PIN
#define SYS_CNF_ACC_ICM_20648_SPI_CLK_PORT    HW_CNF_ACC_ICM_20648_SPI_CLK_PORT
#define SYS_CNF_ACC_ICM_20648_SPI_CLK_PIN     HW_CNF_ACC_ICM_20648_SPI_CLK_PIN
#define SYS_CNF_ACC_ICM_20648_SPI_CS_PORT     HW_CNF_ACC_ICM_20648_SPI_CS_PORT
#define SYS_CNF_ACC_ICM_20648_SPI_CS_PIN      HW_CNF_ACC_ICM_20648_SPI_CS_PIN
#define SYS_CNF_ACC_ICM_20648_INT_PORT        HW_CNF_ACC_ICM_20648_INT_PORT
#define SYS_CNF_ACC_ICM_20648_INT_PIN         HW_CNF_ACC_ICM_20648_INT_PIN
#define SYS_CNF_ACC_ICM_20648_ENABLE_PORT     HW_CNF_ACC_ICM_20648_ENABLE_PORT
#define SYS_CNF_ACC_ICM_20648_ENABLE_PIN      HW_CNF_ACC_ICM_20648_ENABLE_PIN

// Deep sleep ------------------------------------------------------------------
#if DEBUG
  #define SYS_CNF_DEEP_SLEEP_TIMEOUT_MS               0 // Disable deep sleep in debug mode
#else
  #define SYS_CNF_DEEP_SLEEP_TIMEOUT_MS               (5 * 60 * 1000)
#endif
#define SYS_CNF_DEEP_SLEEP_IGNORE_PERIOD_PERCENT      10 // Percentage of the timeout period during which the deep sleep timer is not reset (to avoid frequent resets when remote is active)
#define SYS_CNF_DEEP_SLEEP_PIN_LATCHING_EN            1  // Enable pin latching in EM4 mode
#define SYS_CNF_DEEP_SLEEP_SELECTIVE_PIN_LATCHING_EN  1  // Disables not needed GPIOs in EM4 mode
#define SYS_CNF_DEEP_SLEEP_GPIO_LATCHED_PINS {  \
    _HW_CNF_KEY_MATRIX_ROWS_RAW_LIST,           \
    _HW_CNF_KEY_MATRIX_COLS_RAW_LIST,           \
    _HW_CNF_ACC_SENSOR_DEEP_SLEEP_PINS_RAW_LIST \
}

// LED driver ------------------------------------------------------------------
#define SYS_CNF_LED_PWM_TIMER_NUM         1
// LED effect configuration ---------------------------------------------------
#if HW_CNF_HW_VERSION_IS(HW_CNF_HW_VERSION_THUNDERBOARD)
  #define SYS_CNF_LED_EFFECT_TIMER_NUM    4
#else
  #define SYS_CNF_LED_EFFECT_TIMER_NUM    2
#endif
// LED activity ----------------------------------------------------------------
#define SYS_CNF_LED_ACTIVITY_ENABLE       1
#define SYS_CNF_LED_ACTIVITY_MAX_LEVEL    ((20 * UINT16_MAX) / 100)
#define SYS_CNF_LED_ACTIVITY_CONFIG {            \
    .port = HW_CNF_LED_ACTIVITY_IO_PORT,         \
    .pin = HW_CNF_LED_ACTIVITY_IO_PIN,           \
    .tim_oc_nbr = 0,                             \
    .max_level = SYS_CNF_LED_ACTIVITY_MAX_LEVEL, \
    .polarity = SL_LED_POLARITY_ACTIVE_HIGH,     \
    .type = SL_LED_TYPE_PWM                      \
}
#define SYS_CNF_LED_ACTIVITY_PATTERN {                                           \
    SL_LED_PATTERN_SAME_VALUE_FOR_20_MS(SYS_CNF_LED_ACTIVITY_MAX_LEVEL / 3),     \
    SL_LED_PATTERN_SAME_VALUE_FOR_20_MS(2 * SYS_CNF_LED_ACTIVITY_MAX_LEVEL / 3), \
    SL_LED_PATTERN_SAME_VALUE_FOR_20_MS(SYS_CNF_LED_ACTIVITY_MAX_LEVEL),         \
    SL_LED_PATTERN_SAME_VALUE_FOR_20_MS(SYS_CNF_LED_ACTIVITY_MAX_LEVEL),         \
    SL_LED_PATTERN_SAME_VALUE_FOR_20_MS(SYS_CNF_LED_ACTIVITY_MAX_LEVEL / 2),     \
    SL_LED_PATTERN_SAME_VALUE_FOR_100_MS(0),                                     \
}
// LED backlight ----------------------------------------------------------------
#define SYS_CNF_LED_BACKLIGHT_ENABLE      1
#define SYS_CNF_LED_BACKLIGHT_MAX_LEVEL   ((20 * UINT16_MAX) / 100)
#define SYS_CNF_LED_BACKLIGHT_CONFIG {            \
    .port = HW_CNF_LED_BACKLIGHT_IO_PORT,         \
    .pin = HW_CNF_LED_BACKLIGHT_IO_PIN,           \
    .tim_oc_nbr = 1,                              \
    .max_level = SYS_CNF_LED_BACKLIGHT_MAX_LEVEL, \
    .polarity = SL_LED_POLARITY_ACTIVE_HIGH,      \
    .type = SL_LED_TYPE_PWM                       \
}
#define SYS_CNF_LED_BACKLIGHT_FADE_IN_PATTERN {                        \
    SL_LED_PATTERN_FADE_500_MS(0, SYS_CNF_LED_BACKLIGHT_MAX_LEVEL, 0), \
}
#define SYS_CNF_LED_BACKLIGHT_FADE_OUT_PATTERN {                                                      \
    SL_LED_PATTERN_FADE_100_MS(SYS_CNF_LED_BACKLIGHT_MAX_LEVEL, -SYS_CNF_LED_BACKLIGHT_MAX_LEVEL, 0), \
}
#define SYS_CNF_LED_BACKLIGHT_TIMEOUT_MS 5000

// TIMER configuration check ---------------------------------------------------
 #if    SYS_CNF_LED_PWM_TIMER_NUM == SYS_CNF_IR_TIMER_MODULATOR_NUM \
  || SYS_CNF_LED_PWM_TIMER_NUM == SYS_CNF_IR_TIMER_CARRIER_NUM      \
  || SYS_CNF_LED_PWM_TIMER_NUM == SYS_CNF_LED_EFFECT_TIMER_NUM      \
  || SYS_CNF_IR_TIMER_MODULATOR_NUM == SYS_CNF_IR_TIMER_CARRIER_NUM \
  || SYS_CNF_IR_TIMER_MODULATOR_NUM == SYS_CNF_LED_EFFECT_TIMER_NUM \
  || SYS_CNF_IR_TIMER_CARRIER_NUM == SYS_CNF_LED_EFFECT_TIMER_NUM
  #error "Same timer was assigned to 1 or multiple components!"
#endif

#ifdef __cplusplus
}
#endif
#endif // SL_SYSTEM_CONFIG_H
