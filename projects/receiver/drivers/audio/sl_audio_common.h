/***************************************************************************//**
 * @file sl_audio_common.h
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
#ifndef SL_AUDIO_COMMON_H
#define SL_AUDIO_COMMON_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

// Includes --------------------------------------------------------------------
// Macros ----------------------------------------------------------------------
#define AUDIO_ADPCM_COMPRESSION_RATIO             2
#define AUDIO_BUFFER_MAX_ALLOWED_ADJUSTMENT_SIZE ((10 * AUDIO_FIFO_BUFFER_SIZE_MIN) / 100)
#define AUDIO_FIFO_BUFFER_SIZE_MIN                (SYS_CNF_VOICE_DEF_MAX_FRAME_SIZE_IN * AUDIO_ADPCM_COMPRESSION_RATIO)
#define AUDIO_ARRAY_LEN(array)                    (sizeof(array) / sizeof(array[0]))

#define AUDIO_FIFO_DEFINE(name, buf16_len, fifo_len) \
  static struct {                                    \
    struct {                                         \
      int16_t buf[buf16_len];                        \
      uint32_t len;                                  \
    } data[fifo_len];                                \
    volatile uint32_t rd_idx;                        \
    volatile uint32_t wr_idx;                        \
  } name;

#define audio_fifo_get_next_read_index(fifo)  ((fifo.rd_idx + 1) % AUDIO_ARRAY_LEN(fifo.data))
#define audio_fifo_get_next_write_index(fifo) ((fifo.wr_idx + 1) % AUDIO_ARRAY_LEN(fifo.data))

#define audio_drv_check_stream_start_preconditions(is_streaming, fifo) \
  is_streaming ? -EALREADY : ((fifo.wr_idx == fifo.rd_idx) ? -EINVAL : 0)

#define audio_drv_check_add_buffer_preconditions(fifo, buffer, length) \
  ((length >= AUDIO_ARRAY_LEN(fifo.data[0].buf)) ? -EINVAL             \
   : (audio_fifo_get_next_write_index(fifo) == fifo.rd_idx) ? -ENOMEM : 0)

#define audio_drv_init_dma_descriptor(descriptor, fifo)                \
  for (size_t i = 0; i < AUDIO_ARRAY_LEN(descriptor); i++) {           \
    SL_ASSERT(fifo.rd_idx != fifo.wr_idx, "FIFO underflow at start!"); \
    descriptor[i].xfer.size = ldmaCtrlSizeHalf;                        \
    descriptor[i].xfer.srcAddr = (uint32_t)fifo.data[fifo.rd_idx].buf; \
    descriptor[i].xfer.xferCnt = fifo.data[fifo.rd_idx].len - 1;       \
    fifo.rd_idx = audio_fifo_get_next_read_index(fifo);                \
  }

#define audio_drv_refresh_dma_descriptor(next_descriptor_ptr, fifo)           \
  do {                                                                        \
    next_descriptor_ptr->xfer.srcAddr = (uint32_t)fifo.data[fifo.rd_idx].buf; \
    next_descriptor_ptr->xfer.xferCnt = fifo.data[fifo.rd_idx].len - 1;       \
    fifo.rd_idx = audio_fifo_get_next_read_index(fifo);                       \
  } while (0)

// Type definitions ------------------------------------------------------------
// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------

#ifdef __cplusplus
}
#endif
#endif /* SL_AUDIO_COMMON_H */
