/***************************************************************************//**
 * @file sl_dac.h
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
#ifndef SL_DAC_H
#define SL_DAC_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

// Macros ----------------------------------------------------------------------
// Type definitions ------------------------------------------------------------
// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------

/**
 * @brief Initialize the DAC peripheral.
 */
void sl_dac_init(void);

/**
 * @brief Configures the DAC peripheral.
 *
 * This function configures and (re-)initializes the DAC peripheral with the specified
 * sample rate and number of channels.
 *
 * @param sample_rate The desired sample rate in Hz.
 * @param channels    The number of audio channels (e.g., 1 for mono, 2 for stereo).
 *
 * @warning ONLY 1 channel is supported for the DAC.
 */
void sl_dac_configure(uint32_t sample_rate, uint8_t channels);

/**
 * @brief Start the DAC audio stream.
 *
 * @note DAC can only be started if at least 2 buffers were added using @ref sl_dac_add_tx_buffer().
 *
 * @return 0 on success, negative value on error.
 */
int sl_dac_start_stream(void);

/**
 * @brief Stop the DAC audio stream.
 *
 * @return 0 on success, negative value on error.
 */
int sl_dac_stop_stream(void);

/**
 * @brief Add a transmit buffer for DAC streaming.
 *
 * This function queues a buffer for transmission over the DAC interface.
 *
 * @param[in]  buffer               Pointer to the buffer containing audio data.
 * @param[in]  length               Length of the buffer in samples.
 *
 * @return 0 on success, negative value if the buffer queue is full or on error.
 *
 * @note The maximum buffer size must not exceed the LDMA maximum transfer count.
 * @note The buffer is not copied internally and must remain valid until the transfer is complete.
 */
int sl_dac_add_tx_buffer(const int16_t *buffer, uint32_t length);

/**
 * @brief LDMA IRQ handler for DAC.
 * This function handles the LDMA interrupt for the DAC peripheral.
 */
void sl_dac_on_dma_transfer_complete(void);

#ifdef __cplusplus
}
#endif
#endif /* SL_DAC_H */
