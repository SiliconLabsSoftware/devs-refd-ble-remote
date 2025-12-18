/***************************************************************************//**
 * @file voice.h
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
#ifndef VOICE_H
#define VOICE_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

// Macros ----------------------------------------------------------------------
// Type definitions ------------------------------------------------------------
// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------

/***************************************************************************//**
 * @brief Initialize the voice module.
 ******************************************************************************/
void sl_voice_init(void);

/***************************************************************************//**
 * @brief Stops the voice audio stream on the local audio devices.
 * @return 0 on success, or an error code on failure.
 ******************************************************************************/
void sl_voice_stop_stream(void);

/***************************************************************************//**
 * @brief Callback function invoked when the
 *        connection is established with the remote over BLE.
 ******************************************************************************/
void sl_voice_on_remote_connected(void);

/***************************************************************************//**
 * @brief Start the voice audio stream on the remote.
 * @return 0 on success, or an error code on failure.
 ******************************************************************************/
int sl_voice_trigger_start_stream_on_remote(void);

/***************************************************************************//**
 * @brief Stop the voice audio stream on the remote.
 * @return 0 on success, or an error code on failure.
 ******************************************************************************/
int sl_voice_trigger_stop_stream_on_remote(void);

/***************************************************************************//**
 * @brief Configure the voice sample rate.
 * @param[in] sample_rate The desired sample rate in kHz (only 8/16 is supported).
 ******************************************************************************/
void sl_voice_config_sample_rate(uint8_t sample_rate_khz);

/***************************************************************************//**
 * @brief Enable or disable the voice filter.
 * @param[in] enable True to enable, false to disable.
 ******************************************************************************/
void sl_voice_config_filter(bool enable);

/***************************************************************************//**
 * @brief Enable or disable voice encoding.
 * @param[in] enable True to enable encoding, false to disable.
 ******************************************************************************/
void sl_voice_config_encoding(bool enable);

/***************************************************************************//**
 * @brief Configure the number of audio channels.
 * @param[in] channels Number of audio channels.
 ******************************************************************************/
void sl_voice_config_channel_number(uint8_t channels);

/***************************************************************************//**
 * @brief Configure microphone to use left channel.
 * @param[in] use_left True to use left mic, false otherwise.
 ******************************************************************************/
void sl_voice_config_mic_use_left(bool use_left);

/***************************************************************************//**
 * @brief Configure microphone PDM delay.
 * @param[in] delay Delay value.
 ******************************************************************************/
void sl_voice_config_mic_pdm_delay(uint8_t delay);

/***************************************************************************//**
 * @brief Enable or disable I2S output.
 * @param[in] enable True to enable I2S output, false to disable.
 ******************************************************************************/
void sl_voice_config_i2s_output(bool enable);

/***************************************************************************//**
 * @brief Enable or disable DAC output.
 * @param[in] enable True to enable DAC output, false to disable.
 ******************************************************************************/
void sl_voice_config_dac_output(bool enable);

/***************************************************************************//**
 * @brief Enable or disable log output for audio frames.
 * @param[in] enable True to enable log output, false to disable.
 ******************************************************************************/
void sl_voice_config_log_output(bool enable);

#ifdef __cplusplus
}
#endif
#endif /* VOICE_H */
