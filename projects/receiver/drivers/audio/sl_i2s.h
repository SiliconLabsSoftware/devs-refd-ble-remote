/***************************************************************************//**
 * @file sl_i2s.h
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
#ifndef SL_I2S_H
#define SL_I2S_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

// Includes --------------------------------------------------------------------
// Macros ----------------------------------------------------------------------
// Type definitions ------------------------------------------------------------
// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------

/**
 * @brief Initialize the I2S peripheral.
 */
void sl_i2s_init(void);

/**
 * @brief Configure and (re-)initialize the I2S peripheral.
 *
 * This function configures the I2S peripheral with the specified sample rate,
 * number of channels, and optional clock/sample buffer adjustments.
 *
 * @param[in] sample_rate_hz            Desired sample rate in Hz.
 * @param[in] channels                  Number of audio channels (e.g., 1 for mono, 2 for stereo).
 * @param[in] clock_adjustment_hz       Adjustment value for the I2S clock in Hz.
 * @param[in] sample_buf_adjustment_hz  Adjustment value for the sample buffer in Hz,
 *                                      depending on its sign samples will be added/discarded from received audio buffer.
 */
void sl_i2s_configure(uint32_t sample_rate_hz, uint8_t channels, uint32_t clock_adjustment_hz, uint32_t sample_buf_adjustment_hz);

/**
 * @brief Start the I2S audio stream.
 *
 * @note I2S can only be started if at least 2 buffers were added using @ref sl_i2s_add_tx_buffer().
 *
 * @return 0 on success, negative value on error.
 */
int sl_i2s_start_stream(void);

/**
 * @brief Stop the I2S audio stream.
 *
 * @return 0 on success, negative value on error.
 */
int sl_i2s_stop_stream(void);

/**
 * @brief Add a transmit buffer for I2S streaming.
 *
 * This function queues a buffer for transmission over the I2S interface.
 *
 * @param[in]  buffer               Pointer to the buffer containing audio data.
 * @param[in]  length               Length of the buffer in samples.
 *
 * @return 0 on success, negative value if the buffer queue is full or on error.
 *
 * @note The maximum buffer size must not exceed the LDMA maximum transfer count.
 * @note The buffer is not copied internally and must remain valid until the transfer is complete.
 */
int sl_i2s_add_tx_buffer(const int16_t *buffer, uint32_t length);

/**
 * @brief LDMA IRQ handler for I2S.
 * This function handles the LDMA interrupt for the I2S peripheral.
 */
void sl_i2s_on_dma_transfer_complete(void);

#ifdef __cplusplus
}
#endif
#endif /* SL_I2S_H */
