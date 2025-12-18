/***************************************************************************//**
 * @file sl_drv_mic.h
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
#ifndef SL_DRV_MIC_H
#define SL_DRV_MIC_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

// Macros ----------------------------------------------------------------------
// Type definitions ------------------------------------------------------------
typedef struct {
  uint32_t sample_rate;  ///< Sample rate in Hz
  uint32_t frames;       ///< Number of audio frames to receive before the callback is called
  void *buffer;          ///< Pointer to the sample buffer. 16-bit channel data is stored consecutively, starting with ch0. Shall be big enough to hold twice the n_frames
  uint32_t buffer_size;  ///< Size of the sample buffer in bytes
  uint8_t channels;      ///< Number of audio channels (1 or 2)
  bool use_left_mic;     ///< Use left microphone (only relevant for mono mode)
  uint8_t delay;        ///< Delay on microphone (corresponds to DLYMUXSEL in PDM CFG1 register)
} sl_drv_mic_config_t;

/***************************************************************************//**
 * @brief Callback function indicating that the sample buffer is ready.
 *
 * @param[in] buffer Pointer to the sample buffer.
 * @param[in] n_frames Number of audio frames in the sample buffer.
 ******************************************************************************/
typedef void (*sl_drv_mic_buffer_ready_callback_t)(const void *buffer, uint32_t frames);

// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------

/***************************************************************************//**
 * @brief Initialize the microphone driver.
 * @return Returns 0 on success, non-zero otherwise
 ******************************************************************************/
int sl_drv_mic_init(void);

/***************************************************************************//**
 * @brief
 *    Read samples from the microphone into a sample buffer continuously.
 * @details
 *    This function starts the microphone sampling and stops only upon calling
 *    @ref sl_mic_stop or @ref sl_mic_deinit. The buffer is used in a "ping-pong"
 *    manner meaning that one half of the buffer is used for sampling while the
 *    other half is being processed.
 *
 * @param[in] config
 *    Configuration of the microphone for details see @ref sl_drv_mic_config_t.
 * @param[in] callback
 *    Callback is called when n_frames in the sample buffer is ready.
 *
 * @return Returns 0 on success, non-zero otherwise
 ******************************************************************************/
int sl_drv_mic_start(const sl_drv_mic_config_t *const config,
                     sl_drv_mic_buffer_ready_callback_t callback);

/***************************************************************************//**
 * @brief Stop the microphone sampling.
 * @return Returns 0 on success, non-zero otherwise
 ******************************************************************************/
int sl_drv_mic_stop(void);

#ifdef __cplusplus
}
#endif
#endif /* SL_DRV_MIC_H */
