/***************************************************************************//**
 * @file voice.c
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
#define SL_LOG_MODULE_NAME "Voice"
#include "assert.h"
#include "voice.h"
#include <string.h> // For memset if needed
#include "sl_ble.h"
#include "sl_log.h"
#include "adpcm.h"
#include "sl_i2s.h"
#include "sl_dac.h"
#include "sl_audio_common.h"
#include "../sl_system_config.h"

// Macros ----------------------------------------------------------------------
#define voice_min(a, b) ((a) < (b) ? (a) : (b))
SL_STATIC_ASSERT(SYS_CNF_VOICE_STREAM_DELAY_IN_PACKETS < SYS_CNF_DAC_FIFO_DEPTH, "SYS_CNF_VOICE_STREAM_DELAY_IN_PACKETS must be less than SYS_CNF_DAC_FIFO_DEPTH");
SL_STATIC_ASSERT(SYS_CNF_VOICE_STREAM_DELAY_IN_PACKETS < SYS_CNF_I2S_FIFO_DEPTH, "SYS_CNF_VOICE_STREAM_DELAY_IN_PACKETS must be less than SYS_CNF_I2S_FIFO_DEPTH");

#define VOICE_BLE_INVALID_SIZE_FOR_NOTIFICATION 0

// Private type definitions ----------------------------------------------------
typedef struct {
  bool filter_enabled;
  bool encoding_enabled;
  uint8_t channel_number;
  bool i2s_output_enabled;
  bool dac_output_enabled;
  bool mic_use_left;
  uint8_t mic_pdm_delay;
  uint8_t sample_rate_khz;
  bool log_output_enabled;
} voice_config_t;

// Private function prototypes -------------------------------------------------
static void voice_on_audio_status_change(sl_ble_characteristic_id_t characteristic, const void *data, size_t size);
static void voice_on_audio_frame(sl_ble_characteristic_id_t characteristic, const void *data, size_t size);
static inline void voice_handle_audio_frame(const int16_t *data, size_t size);
static void voice_log_frame(const uint8_t *data, size_t size);

// Private variables -----------------------------------------------------------
static volatile bool voice_streaming;

static adpcm_t voice_adpcm_states[SYS_CNF_VOICE_MAX_CHANNELS];
static adpcm_context_t voice_adpcm_ctx = { sizeof(voice_adpcm_states) / sizeof(voice_adpcm_states[0]), voice_adpcm_states };

static voice_config_t voice_config = {
  .sample_rate_khz = SYS_CNF_VOICE_DEF_SAMPLE_RATE_KHZ,
  .filter_enabled = SYS_CNF_VOICE_DEF_FILTER_ENABLED,
  .encoding_enabled = SYS_CNF_VOICE_DEF_ENCODING_ENABLED,
  .channel_number = SYS_CNF_VOICE_DEF_CHANNEL_NUMBER,
  .mic_use_left = SYS_CNF_VOICE_DEF_MIC_USE_LEFT,
  .mic_pdm_delay = SYS_CNF_VOICE_DEF_MIC_PDM_DELAY,
  .i2s_output_enabled = SYS_CNF_VOICE_DEF_OUT_I2S_EN,
  .dac_output_enabled = SYS_CNF_VOICE_DEF_OUT_DAC_EN,
  .log_output_enabled = SYS_CNF_VOICE_DEF_OUT_LOG_EN,
};
SL_STATIC_ASSERT(SYS_CNF_VOICE_MAX_CHANNELS >= 1 && SYS_CNF_VOICE_MAX_CHANNELS <= 2, "SYS_CNF_VOICE_MAX_CHANNELS must be 1 or 2");
SL_STATIC_ASSERT(SYS_CNF_VOICE_DEF_CHANNEL_NUMBER >= 1 && SYS_CNF_VOICE_DEF_CHANNEL_NUMBER <= SYS_CNF_VOICE_MAX_CHANNELS, "SYS_CNF_VOICE_DEF_CHANNEL_NUMBER must be between 1 and SYS_CNF_VOICE_MAX_CHANNELS");
SL_STATIC_ASSERT(SYS_CNF_VOICE_DEF_SAMPLE_RATE_KHZ == 8 || SYS_CNF_VOICE_DEF_SAMPLE_RATE_KHZ == 16, "SYS_CNF_VOICE_DEF_SAMPLE_RATE_KHZ must be 8 or 16 kHz");
SL_STATIC_ASSERT(SYS_CNF_VOICE_DEF_MIC_PDM_DELAY < 4, "SYS_CNF_VOICE_DEF_MIC_PDM_DELAY must be less than 4");

static const struct {
  void *data;
  size_t size;
  sl_ble_characteristic_id_t characteristic;
} voice_audio_config[] = {
  { .data = &voice_config.sample_rate_khz, .size = sizeof(voice_config.sample_rate_khz), .characteristic = SL_BLE_CID_VOICE_SAMPLE_RATE },
  { .data = &voice_config.filter_enabled, .size = sizeof(voice_config.filter_enabled), .characteristic = SL_BLE_CID_VOICE_FILTER_ENABLE },
  { .data = &voice_config.encoding_enabled, .size = sizeof(voice_config.encoding_enabled), .characteristic = SL_BLE_CID_VOICE_ENCODING_ENABLE },
  { .data = &voice_config.channel_number, .size = sizeof(voice_config.channel_number), .characteristic = SL_BLE_CID_VOICE_AUDIO_CHANNELS },
  { .data = &voice_config.mic_use_left, .size = sizeof(voice_config.mic_use_left), .characteristic = SL_BLE_CID_TEST_MIC_USE_LEFT },
  { .data = &voice_config.mic_pdm_delay, .size = sizeof(voice_config.mic_pdm_delay), .characteristic = SL_BLE_CID_TEST_MIC_PDM_DELAY },
  { .data = &voice_on_audio_status_change, .size = VOICE_BLE_INVALID_SIZE_FOR_NOTIFICATION, .characteristic = SL_BLE_CID_VOICE_TRANSFER_STATUS },
  { .data = &voice_on_audio_frame, .size = VOICE_BLE_INVALID_SIZE_FOR_NOTIFICATION, .characteristic = SL_BLE_CID_VOICE_AUDIO_DATA },
};

static const struct {
  int32_t i2s_clock_offset_hz;
  int32_t i2s_buf_offset_hz;
  int32_t dac_clock_offset_hz;
} voice_freq_offs_cnf[] = {
  { SYS_CNF_VOICE_I2S_FREQ_CLOCK_OFFSET_AT_8KHZ_HZ, SYS_CNF_VOICE_I2S_FREQ_BUF_OFFSET_AT_8KHZ_HZ, SYS_CNF_VOICE_DAC_FREQ_OFFSET_AT_8KHZ_HZ },
  { SYS_CNF_VOICE_I2S_FREQ_CLOCK_OFFSET_AT_16KHZ_HZ, SYS_CNF_VOICE_I2S_FREQ_BUF_OFFSET_AT_16KHZ_HZ, SYS_CNF_VOICE_DAC_FREQ_OFFSET_AT_16KHZ_HZ },
};

// Function definitions --------------------------------------------------------
void sl_voice_init(void)
{
  sl_i2s_init();
  sl_dac_init();
}

void sl_voice_on_remote_connected(void)
{
  //Configure the audio output(s)
  uint32_t freq_offs_idx = voice_min((voice_config.sample_rate_khz / 8UL) - 1UL, AUDIO_ARRAY_LEN(voice_freq_offs_cnf) - 1UL);
  voice_adpcm_ctx.ch_count = voice_min(voice_config.channel_number, 2UL);
  ADPCM_init(&voice_adpcm_ctx);
  sl_i2s_configure(voice_config.sample_rate_khz * 1000,
                   voice_config.channel_number,
                   voice_freq_offs_cnf[freq_offs_idx].i2s_clock_offset_hz,
                   voice_freq_offs_cnf[freq_offs_idx].i2s_buf_offset_hz);
  sl_dac_configure(voice_config.sample_rate_khz * 1000 + voice_freq_offs_cnf[freq_offs_idx].dac_clock_offset_hz, voice_config.channel_number);

  // Configure remote device
  for (uint32_t conf_idx = 0; conf_idx < AUDIO_ARRAY_LEN(voice_audio_config); conf_idx++) {
    int result = 0;
    if (VOICE_BLE_INVALID_SIZE_FOR_NOTIFICATION == voice_audio_config[conf_idx].size) {
      result = sl_ble_characteristic_notify(voice_audio_config[conf_idx].characteristic, voice_audio_config[conf_idx].data);
    } else {
      result = sl_ble_characteristic_write(voice_audio_config[conf_idx].characteristic,
                                           voice_audio_config[conf_idx].data,
                                           voice_audio_config[conf_idx].size,
                                           NULL);
    }
    sl_log_status_error(result, "BLE failure! Characteristic=%d, res=%d" SL_LOG_EOL, voice_audio_config[conf_idx].characteristic, result);
  }
  sl_log_info("Configuration Done!" SL_LOG_EOL);
}

void sl_voice_stop_stream(void)
{
  if (voice_streaming) {
    voice_streaming = false;
    int res = sl_i2s_stop_stream();
    sl_log_status_warning(res != 0, "I2S stop failed: %d" SL_LOG_EOL, res);
    res = sl_dac_stop_stream();
    sl_log_status_warning(res != 0, "DAC stop failed: %d" SL_LOG_EOL, res);
  }
}

int sl_voice_trigger_start_stream_on_remote(void)
{
  bool enable = true;
  return sl_ble_characteristic_write(SL_BLE_CID_TEST_VOICE_RECORD, &enable, sizeof(enable), NULL);
}

int sl_voice_trigger_stop_stream_on_remote(void)
{
  bool enable = false;
  return sl_ble_characteristic_write(SL_BLE_CID_TEST_VOICE_RECORD, &enable, sizeof(enable), NULL);
}

static void voice_on_audio_status_change(sl_ble_characteristic_id_t characteristic, const void *data, size_t size)
{
  (void)characteristic;
  (void)size;
  const uint8_t *voice_running = data;

  if (!(*voice_running)) {
    sl_log_info("Voice streaming stopped!" SL_LOG_EOL);
    sl_voice_stop_stream();
  }
}

static void voice_on_audio_frame(sl_ble_characteristic_id_t characteristic, const void *data, size_t size)
{
  (void)characteristic;
  static int16_t adpcm_buffer[AUDIO_FIFO_BUFFER_SIZE_MIN * AUDIO_ADPCM_COMPRESSION_RATIO];

  if (voice_config.encoding_enabled) {
    size -= ADPCM_HEADER_SIZE;
    uint32_t frame_count = (size * AUDIO_ADPCM_COMPRESSION_RATIO) / voice_config.channel_number;
    ADPCM_load_state(&voice_adpcm_ctx, data);
    ADPCM_decode(&voice_adpcm_ctx, ((uint8_t *)data) + ADPCM_HEADER_SIZE, adpcm_buffer, frame_count);
    voice_handle_audio_frame(adpcm_buffer, size * AUDIO_ADPCM_COMPRESSION_RATIO);
  } else {
    voice_handle_audio_frame(data, size / 2);
  }
}

static inline void voice_handle_audio_frame(const int16_t *data, size_t len)
{
  static uint32_t delay_counter = 0;

  if (voice_config.i2s_output_enabled) {
    int res = sl_i2s_add_tx_buffer(data, len);
    SL_ASSERT(!SYS_CNF_VOICE_STOP_AT_ERROR || (SYS_CNF_VOICE_STOP_AT_ERROR && res == 0), "I2S buffer overflow: %d" SL_LOG_EOL, res);
    sl_log_status_warning(res != 0, "I2S buffer overflow: %d" SL_LOG_EOL, res);
  }
  if (voice_config.dac_output_enabled) {
    int res = sl_dac_add_tx_buffer(data, len);
    SL_ASSERT(!SYS_CNF_VOICE_STOP_AT_ERROR || (SYS_CNF_VOICE_STOP_AT_ERROR && res == 0), "DAC buffer overflow: %d" SL_LOG_EOL, res);
    sl_log_status_warning(res != 0, "DAC buffer overflow: %d" SL_LOG_EOL, res);
  }

  if (!voice_streaming && delay_counter++ >= SYS_CNF_VOICE_STREAM_DELAY_IN_PACKETS) {
    voice_streaming = true;
    delay_counter = 0;
    if (voice_config.i2s_output_enabled) {
      int res = sl_i2s_start_stream();
      SL_ASSERT(res == 0, "I2S start failed: %d" SL_LOG_EOL, res);
    }
    if (voice_config.dac_output_enabled) {
      int res = sl_dac_start_stream();
      SL_ASSERT(res == 0, "DAC start failed: %d" SL_LOG_EOL, res);
    }
    sl_log_info("Voice streaming started!" SL_LOG_EOL);
  }

  if (voice_config.log_output_enabled) {
    voice_log_frame((const uint8_t *)data, len * 2);
  }
}

static void voice_log_frame(const uint8_t *data, size_t size)
{
  static char voice_frame_string_buffer[256 * 2 + 1];
  const char *hex = "0123456789ABCDEF";

  size_t str_len = 0;
  for (; (str_len / 2) < size && str_len < (sizeof(voice_frame_string_buffer) - 3); str_len += 2) {
    voice_frame_string_buffer[str_len] = hex[(data[str_len / 2] >> 4) & 0xF];
    voice_frame_string_buffer[str_len + 1] = hex[(data[str_len / 2]) & 0xF];
  }
  voice_frame_string_buffer[str_len] = '\0';
  sl_log_info("Frame (%zuB): 0x%s" SL_LOG_EOL, size, voice_frame_string_buffer);
}

void sl_voice_config_sample_rate(uint8_t sample_rate)
{
  if (sample_rate != 8 && sample_rate != 16) {
    sl_log_error("Invalid sample rate: %u. Supported rates are 8 and 16 kHz." SL_LOG_EOL, sample_rate);
  } else {
    voice_config.sample_rate_khz = sample_rate;
  }
}

void sl_voice_config_filter(bool enable)
{
  voice_config.filter_enabled = enable;
}

void sl_voice_config_encoding(bool enable)
{
  voice_config.encoding_enabled = enable;
}

void sl_voice_config_channel_number(uint8_t channels)
{
  if (!channels || channels > SYS_CNF_VOICE_MAX_CHANNELS) {
    sl_log_error("Invalid channel number: %u. Supported value must be less or equal than %d." SL_LOG_EOL, channels, SYS_CNF_VOICE_MAX_CHANNELS);
  } else {
    voice_config.channel_number = channels;
  }
}

void sl_voice_config_mic_use_left(bool use_left)
{
  voice_config.mic_use_left = use_left;
}

void sl_voice_config_mic_pdm_delay(uint8_t delay)
{
  voice_config.mic_pdm_delay = delay;
}

void sl_voice_config_i2s_output(bool enable)
{
  voice_config.i2s_output_enabled = enable;
}

void sl_voice_config_dac_output(bool enable)
{
  voice_config.dac_output_enabled = enable;
}

void sl_voice_config_log_output(bool enable)
{
  voice_config.log_output_enabled = enable;
}
