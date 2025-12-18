/***************************************************************************//**
 * @file
 * @brief Voice transmission
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#define SL_LOG_MODULE_NAME "Voice"
#include <string.h>
#include <errno.h>
#include "assert.h"
#include "sl_log.h"
#include "voice.h"
#include "filter.h"
#include "adpcm.h"
#include "sl_ble.h"
#include "sl_drv_mic.h"
#include "../sl_system_config.h"

// -----------------------------------------------------------------------------
// Private macros
#define MIC_CHANNELS_MAX        2
#define MIC_SAMPLE_BUFFER_SIZE  112
#define MIC_SEND_BUFFER_SIZE    (MIC_SAMPLE_BUFFER_SIZE * MIC_CHANNELS_MAX)
#define ADPCM_COMPRESS_FACTOR   2 //2 * 16 bit -> 2 * 4 bit = 1 * 8 bit

#define SR2FS(sr)               ((sr) * 1000)

// -----------------------------------------------------------------------------
// Private type definitions
typedef struct {
  sample_rate_t sampleRate;
  uint8_t channels;
  bool filter_enabled;
  bool encoding_enabled;
  bool use_left_mic;
  uint8_t delay;
} voice_config_t;

// -----------------------------------------------------------------------------
// Private variables
static volatile bool event_process = false;
static bool voice_running = false;

static biquad_t biquads[MIC_CHANNELS_MAX];
static filter_context_t filter = { 0, biquads };

static adpcm_t adpcms[MIC_CHANNELS_MAX];
static adpcm_context_t adpcm = { 0, adpcms };

static struct {
  int16_t *buf;
  uint32_t frames;
} mic_out;

static struct {
  uint8_t buf[MIC_SEND_BUFFER_SIZE + ADPCM_HEADER_SIZE];
  uint32_t idx;
} ble_out;
SL_STATIC_ASSERT(250 >= sizeof(ble_out.buf), "Send buffer exceeds max. MTU!");

static voice_config_t voice_config = {
  .sampleRate = SYS_CNF_VOICE_DEF_SAMPLE_RATE,
  .channels = SYS_CNF_VOICE_DEF_CHANNELS,
  .filter_enabled = SYS_CNF_VOICE_DEF_FILTER,
  .encoding_enabled = SYS_CNF_VOICE_DEF_ENCODE,
  .use_left_mic = SYS_CNF_VOICE_DEF_USE_LEFT_MIC,
  .delay = SYS_CNF_VOICE_DEF_DELAY,
};
SL_STATIC_ASSERT(SYS_CNF_VOICE_DEF_CHANNELS >= 1 && SYS_CNF_VOICE_DEF_CHANNELS <= 2, "SYS_CNF_VOICE_DEF_CHANNELS must be 1 or 2");
SL_STATIC_ASSERT(SYS_CNF_VOICE_DEF_SAMPLE_RATE == 8 || SYS_CNF_VOICE_DEF_SAMPLE_RATE == 16, "SYS_CNF_VOICE_DEF_SAMPLE_RATE must be 8 or 16 kHz");
SL_STATIC_ASSERT(SYS_CNF_VOICE_DEF_DELAY < 4, "SYS_CNF_VOICE_DEF_DELAY must be less than 4");

// -----------------------------------------------------------------------------
// Private function declarations
static void voice_process_data(void);
static void voice_send_status(void);
static void voice_send_data(void);
static void voice_mic_buffer_ready(const void *buffer, uint32_t n_frames);

// -----------------------------------------------------------------------------
// Public function definitions

int voice_init(void)
{
  return sl_drv_mic_init();
}

void voice_cyclic(void)
{
  if (event_process) {
    event_process = false;
    if (sl_ble_is_connected()) {
      voice_process_data();
      voice_send_data();
    }
  }
}

int voice_start(void)
{
  int sc = voice_running ? -EBUSY : 0;

  // ADPCM encoder initialization
  if (!sc && voice_config.encoding_enabled) {
    adpcm.ch_count = voice_config.channels;
    ADPCM_init(&adpcm);
  }

  // Filter initialization
  if (!sc && voice_config.filter_enabled) {
    filter_parameters_t fp = DEFAULT_FILTER;
    fp.srate = SR2FS(voice_config.sampleRate);
    filter.ch_count = voice_config.channels;
    fil_init(&filter, &fp);
  }

  // Microphone initialization
  if (!sc) {
    static int16_t mic_buffer[2 * MIC_SAMPLE_BUFFER_SIZE];
    const sl_drv_mic_config_t mic_config = {
      .sample_rate = SR2FS(voice_config.sampleRate),
      .frames = MIC_SAMPLE_BUFFER_SIZE / voice_config.channels,
      .buffer = mic_buffer,
      .buffer_size = sizeof(mic_buffer),
      .channels = voice_config.channels,
      .use_left_mic = voice_config.use_left_mic,
      .delay = voice_config.delay,
    };

    ble_out.idx = 0;
    sc = sl_drv_mic_start(&mic_config, voice_mic_buffer_ready);
  }

  // Audio transfer started
  if (!sc) {
    voice_running = true;
    voice_send_status();
  }
  sl_log_info("Recording started, sc: %d, encoding: %u, filter: %u, ch: %u, SR: %u" SL_LOG_EOL,
              sc, voice_config.encoding_enabled, voice_config.filter_enabled, voice_config.channels, voice_config.sampleRate);
  return sc;
}

int voice_stop(void)
{
  int sc = voice_running ? 0 : -ESRCH;

  if (!sc) {
    voice_running = false;
    sc = sl_drv_mic_stop();
    voice_send_status();
  }
  sl_log_info("Recording stopped, sc: %d" SL_LOG_EOL, sc);
  return sc;
}

static void voice_send_status(void)
{
  int sc = sl_ble_characteristic_notify(SL_BLE_CID_VOICE_TRANSFER_STATUS, (void *)&voice_running, sizeof(voice_running));
  sl_log_status_error(sc, "BLE status notify error: %d" SL_LOG_EOL, sc);
}

bool voice_is_recording(void)
{
  return voice_running;
}

void voice_set_sample_rate(sample_rate_t sample_rate)
{
  if ((sample_rate == sr_16k) || (sample_rate == sr_8k)) {
    voice_config.sampleRate = sample_rate;
  }
}

void voice_set_channels(uint8_t channels)
{
  if ((channels > 0) && (channels <= MIC_CHANNELS_MAX)) {
    voice_config.channels = channels;
  }
}

void voice_set_filter_enable(bool status)
{
  voice_config.filter_enabled = status;
}

void voice_set_encoding_enable(bool status)
{
  voice_config.encoding_enabled = status;
}

void voice_set_mic_use_left(bool status)
{
  voice_config.use_left_mic = status;
}

void voice_set_mic_delay(uint8_t delay)
{
  if (delay < 4) {
    voice_config.delay = delay;
  }
}

static void voice_process_data(void)
{
  if (voice_config.filter_enabled) {
    fil_filter(&filter, mic_out.buf, mic_out.buf, mic_out.frames);
  }

  if (voice_config.encoding_enabled) {
#if SYS_CNF_VOICE_ENCODE_SEND_STATE
    if (0 == ble_out.idx) {
      ADPCM_save_state(&adpcm, ble_out.buf);
      ble_out.idx = ADPCM_HEADER_SIZE;
    }
#endif
    ADPCM_encode(&adpcm, mic_out.buf, &ble_out.buf[ble_out.idx], mic_out.frames);
    ble_out.idx += (mic_out.frames * voice_config.channels / ADPCM_COMPRESS_FACTOR);
  } else {
    const uint32_t sample_size = mic_out.frames * voice_config.channels * sizeof(int16_t);
    memcpy(&ble_out.buf[ble_out.idx], mic_out.buf, sample_size);
    ble_out.idx += sample_size;
  }
  SL_ASSERT(ble_out.idx <= sizeof(ble_out.buf), "Buffer overflow: %d", ble_out.idx);
}

static void voice_send_data(void)
{
  if (ble_out.idx >= MIC_SEND_BUFFER_SIZE) {
    int sc = sl_ble_characteristic_notify(SL_BLE_CID_VOICE_AUDIO_DATA, ble_out.buf, ble_out.idx);
    sl_log_status_error(sc, "BLE notify error: %d" SL_LOG_EOL, sc);
    ble_out.idx = 0;
  }
}

static void voice_mic_buffer_ready(const void *buffer, uint32_t n_frames)
{
  mic_out.buf = (int16_t *)buffer;
  mic_out.frames = n_frames;
  event_process = true;
}
