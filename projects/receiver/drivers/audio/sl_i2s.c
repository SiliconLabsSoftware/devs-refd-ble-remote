/***************************************************************************//**
 * @file sl_i2s.c
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
#define SL_LOG_MODULE_NAME "I2S"
#include <string.h>
#include <errno.h>
#include "sl_i2s.h"
#include "assert.h"
#include "em_gpio.h"
#include "em_cmu.h"
#include "em_usart.h"
#include "em_ldma.h"
#include "sl_log.h"
#include "sl_common.h"
#include "sl_audio_common.h"
#include "sl_system_config.h"

// Macros ----------------------------------------------------------------------
#if SYS_CNF_I2S_DATA_WIDTH != 16 && SYS_CNF_I2S_DATA_WIDTH != 32
  #error "I2S data width must be either 16 or 32 bits"
#endif

#ifndef CAT3
  #define _CAT3(a, b, c) a##b##c
  #define CAT3(a, b, c) _CAT3(a, b, c)
#endif
#ifndef CAT2
  #define CAT2(a, b) CAT3(a, b, )
#endif

#define I2S_USART         CAT2(USART, SYS_CNF_I2S_USART_NUMBER)
#define I2S_FIFO_BUF_SIZE (AUDIO_FIFO_BUFFER_SIZE_MIN + AUDIO_BUFFER_MAX_ALLOWED_ADJUSTMENT_SIZE)
#define I2S_LDMA_MAX_LEN  2048
#define I2S_LDMA_CH_MAIN  SYS_CNF_I2S_DMA_CHANNEL_LEFT
#define I2S_LDMA_CH_DUMMY SYS_CNF_I2S_DMA_CHANNEL_RIGHT

// Private type definitions ----------------------------------------------------
typedef struct {
  int32_t sample_freq;
  int32_t freq_error;
  float sum_adjustments;
} i2s_freq_corr_ctx_t;

// Private function prototypes -------------------------------------------------
static inline void i2s_frequency_correction(i2s_freq_corr_ctx_t *ctx, int16_t *buffer, uint32_t *length);

// Private variables -----------------------------------------------------------
static bool i2s_streaming;
static bool i2s_streaming_stop_req;
static bool i2s_streaming_mono_dummy;
static const int16_t i2s_dma_dummy_buffer;
static i2s_freq_corr_ctx_t i2s_freq_corr_ctx;

static const LDMA_TransferCfg_t i2s_dma_transfer_cfg =
  LDMA_TRANSFER_CFG_PERIPHERAL(CAT3(ldmaPeripheralSignal_USART, SYS_CNF_I2S_USART_NUMBER, _TXBL));
static const LDMA_TransferCfg_t i2s_dma_transfer_cfg_mono_dummy =
  LDMA_TRANSFER_CFG_PERIPHERAL(CAT3(ldmaPeripheralSignal_USART, SYS_CNF_I2S_USART_NUMBER, _TXBLRIGHT));
static LDMA_Descriptor_t i2s_dma_descriptor[] = {
  LDMA_DESCRIPTOR_LINKREL_M2P_BYTE(&i2s_dma_dummy_buffer, &I2S_USART->TXDOUBLE, 1, 1),
  LDMA_DESCRIPTOR_LINKREL_M2P_BYTE(&i2s_dma_dummy_buffer, &I2S_USART->TXDOUBLE, 1, -1),
};
static LDMA_Descriptor_t i2s_dma_descriptor_mono_dummy =
  LDMA_DESCRIPTOR_LINKREL_M2P_BYTE(&i2s_dma_dummy_buffer, &I2S_USART->TXDOUBLE, I2S_LDMA_MAX_LEN, 0);

AUDIO_FIFO_DEFINE(i2s_fifo, I2S_FIFO_BUF_SIZE, SYS_CNF_I2S_FIFO_DEPTH);

// Function definitions --------------------------------------------------------
void sl_i2s_init(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(CAT2(cmuClock_USART, SYS_CNF_I2S_USART_NUMBER), true);

  GPIO_PinModeSet(SYS_CNF_I2S_SCK_PORT, SYS_CNF_I2S_SCK_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(SYS_CNF_I2S_WS_PORT, SYS_CNF_I2S_WS_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(SYS_CNF_I2S_SD_PORT, SYS_CNF_I2S_SD_PIN, gpioModePushPull, 0);
  GPIO->USARTROUTE[SYS_CNF_I2S_USART_NUMBER].ROUTEEN = GPIO_USART_ROUTEEN_TXPEN | GPIO_USART_ROUTEEN_CLKPEN | GPIO_USART_ROUTEEN_CSPEN;
  GPIO->USARTROUTE[SYS_CNF_I2S_USART_NUMBER].TXROUTE = (SYS_CNF_I2S_SD_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT) | (SYS_CNF_I2S_SD_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[SYS_CNF_I2S_USART_NUMBER].CLKROUTE = (SYS_CNF_I2S_SCK_PORT << _GPIO_USART_CLKROUTE_PORT_SHIFT) | (SYS_CNF_I2S_SCK_PIN << _GPIO_USART_CLKROUTE_PIN_SHIFT);
  GPIO->USARTROUTE[SYS_CNF_I2S_USART_NUMBER].CSROUTE = (SYS_CNF_I2S_WS_PORT << _GPIO_USART_CSROUTE_PORT_SHIFT) | (SYS_CNF_I2S_WS_PIN << _GPIO_USART_CSROUTE_PIN_SHIFT);

  i2s_dma_descriptor_mono_dummy.xfer.size = ldmaCtrlSizeHalf;
  i2s_dma_descriptor_mono_dummy.xfer.srcInc = ldmaCtrlSrcIncNone;
  i2s_dma_descriptor_mono_dummy.xfer.doneIfs = 0;
}

void sl_i2s_configure(uint32_t sample_rate_hz, uint8_t channels, uint32_t clock_adjustment_hz, uint32_t sample_buf_adjustment_hz)
{
  i2s_fifo.rd_idx = i2s_fifo.wr_idx;
  i2s_streaming_mono_dummy = !SYS_CNF_I2S_MONO_MODE_SUPPORTED && (channels == 1);
  i2s_freq_corr_ctx.sample_freq = sample_rate_hz;
  i2s_freq_corr_ctx.freq_error = sample_buf_adjustment_hz;

  USART_InitI2s_TypeDef init = USART_INITI2S_DEFAULT;
  init.sync.autoTx   = false;
  init.justify       = SYS_CNF_I2S_IS_LEFT_JUSTIFIED ? usartI2sJustifyLeft : usartI2sJustifyRight;
  init.mono          = SYS_CNF_I2S_MONO_MODE_SUPPORTED && (channels == 1);
  init.sync.baudrate = (sample_rate_hz + clock_adjustment_hz) * SYS_CNF_I2S_DATA_WIDTH * (init.mono ? 1 : 2);
  init.delay         = !init.mono;
  init.format        = CAT3(usartI2sFormatW, SYS_CNF_I2S_DATA_WIDTH, D16);
  init.dmaSplit      = i2s_streaming_mono_dummy;
  USART_Reset(I2S_USART);
  USART_InitI2s(I2S_USART, &init);
}

int sl_i2s_start_stream(void)
{
  int sc = audio_drv_check_stream_start_preconditions(i2s_streaming, i2s_fifo);

  if (!sc) {
    i2s_streaming = true;
    i2s_streaming_stop_req = false;
    i2s_freq_corr_ctx.sum_adjustments = 0;
    audio_drv_init_dma_descriptor(i2s_dma_descriptor, i2s_fifo);
    LDMA_StartTransfer(I2S_LDMA_CH_MAIN, &i2s_dma_transfer_cfg, i2s_dma_descriptor);
    if (i2s_streaming_mono_dummy) {
      LDMA_StartTransfer(I2S_LDMA_CH_DUMMY, &i2s_dma_transfer_cfg_mono_dummy, &i2s_dma_descriptor_mono_dummy);
    }
  }
  return sc;
}

int sl_i2s_stop_stream(void)
{
  int sc = -EALREADY;

  if (i2s_streaming) {
    sc = 0;
    i2s_streaming = false;
    i2s_streaming_stop_req = true;
    i2s_fifo.rd_idx = i2s_fifo.wr_idx;
  }
  return sc;
}

int sl_i2s_add_tx_buffer(const int16_t *buffer, uint32_t length)
{
  int sc = audio_drv_check_add_buffer_preconditions(i2s_fifo, buffer, length);

  if (!sc) {
    memcpy(i2s_fifo.data[i2s_fifo.wr_idx].buf, buffer, length * 2);
    i2s_fifo.data[i2s_fifo.wr_idx].len = length;
    i2s_fifo.wr_idx = audio_fifo_get_next_write_index(i2s_fifo);
  }
  return sc;
}

void sl_i2s_on_dma_transfer_complete(void)
{
  static bool print;
  static uint32_t seq_cnt;
  LDMA_Descriptor_t *next_desc = &i2s_dma_descriptor[seq_cnt++ % 2];

  if (i2s_fifo.rd_idx != i2s_fifo.wr_idx && !i2s_streaming_stop_req) {
    i2s_frequency_correction(&i2s_freq_corr_ctx, i2s_fifo.data[i2s_fifo.rd_idx].buf, &i2s_fifo.data[i2s_fifo.rd_idx].len);
    audio_drv_refresh_dma_descriptor(next_desc, i2s_fifo);
    print = true;
  } else if (   (i2s_dma_descriptor[0].xfer.srcInc == ldmaCtrlSrcIncNone)
                && (i2s_dma_descriptor[1].xfer.srcInc == ldmaCtrlSrcIncNone) ) {
    seq_cnt = 0;
    LDMA_StopTransfer(I2S_LDMA_CH_MAIN);
    if (i2s_streaming_mono_dummy) {
      LDMA_StopTransfer(I2S_LDMA_CH_DUMMY);
    }
    // Reset SrcInc to allow next transfer
    i2s_dma_descriptor[0].xfer.srcInc = ldmaCtrlSrcIncOne;
    i2s_dma_descriptor[1].xfer.srcInc = ldmaCtrlSrcIncOne;
  } else {
    if (SYS_CNF_VOICE_STOP_AT_ERROR || i2s_streaming_stop_req) {
      //Starting to stop the transfer (slow stop to avoid popping noise)
      next_desc->xfer.srcInc = ldmaCtrlSrcIncNone;
      next_desc->xfer.srcAddr = (uint32_t)&i2s_dma_dummy_buffer;
      next_desc->xfer.xferCnt = I2S_LDMA_MAX_LEN - 1;
    }
    sl_log_status_warning(print && !i2s_streaming_stop_req && (i2s_fifo.rd_idx == i2s_fifo.wr_idx), "FIFO underflow!" SL_LOG_EOL);
    print = false;
  }
}

static inline void i2s_frequency_correction(i2s_freq_corr_ctx_t *ctx, int16_t *buffer, uint32_t *length)
{
  if (ctx->freq_error != 0 ) {
    ctx->sum_adjustments += ((ctx->freq_error * (*length)) / ((float)ctx->sample_freq));
    int samples_to_adjust = ctx->sum_adjustments;

    if (ctx->freq_error < 0) {
      //extend samples with the last sample
      SL_ASSERT(*length - samples_to_adjust <= I2S_FIFO_BUF_SIZE, "Buffer overflow during I2S frequency correction!");
      for (int i = 0; i < (-samples_to_adjust); i++) {
        buffer[*length + i] = buffer[*length - 1];
      }
    } else {
      SL_ASSERT((int)*length - samples_to_adjust >= 0, "Buffer underflow during I2S frequency correction!");
    }
    *length -= samples_to_adjust;
    ctx->sum_adjustments -= samples_to_adjust;
  }
}
