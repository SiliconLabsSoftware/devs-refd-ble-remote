/***************************************************************************//**
 * @file sl_dac.c
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
#define SL_LOG_MODULE_NAME "DAC"
#include <errno.h>
#include "assert.h"
#include "sl_dac.h"
#include "em_cmu.h"
#include "em_vdac.h"
#include "em_timer.h"
#include "em_ldma.h"
#include "sl_log.h"
#include "sl_audio_common.h"
#include "sl_system_config.h"

// Macros ----------------------------------------------------------------------
#ifndef CAT4
  #define _CAT4(a, b, c, d) a##b##c##d
  #define CAT4(a, b, c, d) _CAT4(a, b, c, d)
#endif
#ifndef CAT3
  #define CAT3(a, b, c) CAT4(a, b, c, )
#endif
#ifndef CAT2
  #define CAT2(a, b) CAT3(a, b, )
#endif

#define DAC_VDAC    CAT2(VDAC, SYS_CNF_DAC_NUMBER)
#define DAC_TIMER   CAT2(TIMER, SYS_CNF_DAC_TIMER_NUMBER)

// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
// Private variables -----------------------------------------------------------
static bool dac_streaming;
static uint32_t dac_streaming_seq_cnt;
static const int16_t dac_dma_dummy_buffer;
static const LDMA_TransferCfg_t dac_dma_transfer_cfg =
  LDMA_TRANSFER_CFG_PERIPHERAL(CAT3(ldmaPeripheralSignal_TIMER, SYS_CNF_DAC_TIMER_NUMBER, _UFOF));
static LDMA_Descriptor_t dac_dma_descriptor[] = {
  LDMA_DESCRIPTOR_LINKREL_M2P_BYTE(&dac_dma_dummy_buffer, &DAC_VDAC->CAT3(CH, SYS_CNF_DAC_CH_NUMBER, F), 1, 1),
  LDMA_DESCRIPTOR_LINKREL_M2P_BYTE(&dac_dma_dummy_buffer, &DAC_VDAC->CAT3(CH, SYS_CNF_DAC_CH_NUMBER, F), 1, -1),
};

AUDIO_FIFO_DEFINE(dac_fifo, AUDIO_FIFO_BUFFER_SIZE_MIN, SYS_CNF_DAC_FIFO_DEPTH);

// Function definitions --------------------------------------------------------
void sl_dac_init(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);
  CMU_ClockEnable(CAT2(cmuClock_VDAC, SYS_CNF_DAC_NUMBER), true);
  CMU_ClockEnable(CAT2(cmuClock_TIMER, SYS_CNF_DAC_TIMER_NUMBER), true);

#if SYS_CNF_DAC_AUX_ENABLE
  SYS_CNF_DAC_AUX_ABUS_GPIO_ALLOC_REG |= SYS_CNF_DAC_AUX_ABUS_GPIO_ALLOC_VALUE;
#endif

  TIMER_Init_TypeDef init_timer = TIMER_INIT_DEFAULT;
  init_timer.dmaClrAct = true;
  TIMER_Init(DAC_TIMER, &init_timer);
  TIMER_TopBufSet(DAC_TIMER, 8);

  VDAC_Init_TypeDef init_vdac = VDAC_INIT_DEFAULT;
  init_vdac.reference = SYS_CNF_DAC_REF_VOLTAGE;
  VDAC_Init(DAC_VDAC, &init_vdac);

  VDAC_InitChannel_TypeDef init_vdac_ch = VDAC_INITCHANNEL_DEFAULT;
  init_vdac_ch.mainOutEnable = !SYS_CNF_DAC_AUX_ENABLE;
  init_vdac_ch.auxOutEnable = SYS_CNF_DAC_AUX_ENABLE;
  init_vdac_ch.port = SYS_CNF_DAC_AUX_PORT;
  init_vdac_ch.pin = SYS_CNF_DAC_AUX_PIN;
  VDAC_InitChannel(DAC_VDAC, &init_vdac_ch, SYS_CNF_DAC_CH_NUMBER);

  VDAC_Enable(DAC_VDAC, SYS_CNF_DAC_CH_NUMBER, true);
#if SYS_CNF_DAC_AUX_ENABLE
  SL_ASSERT(0 == (DAC_VDAC->STATUS & VDAC_STATUS_ABUSALLOCERR), "VDAC bus allocation problem!");
#endif
}

void sl_dac_configure(uint32_t sample_rate, uint8_t channels)
{
  SL_ASSERT(channels == 1, "Only mono mode is supported for DAC");

  dac_fifo.rd_idx = dac_fifo.wr_idx;
  uint32_t top_value = (CMU_ClockFreqGet(CAT2(cmuClock_TIMER, SYS_CNF_DAC_TIMER_NUMBER)) / sample_rate) - 1;
  TIMER_TopBufSet(DAC_TIMER, top_value);
}

int sl_dac_start_stream(void)
{
  int sc = audio_drv_check_stream_start_preconditions(dac_streaming, dac_fifo);

  if (!sc) {
    dac_streaming = true;
    dac_streaming_seq_cnt = 2;
    audio_drv_init_dma_descriptor(dac_dma_descriptor, dac_fifo);
    LDMA_StartTransfer(SYS_CNF_DAC_DMA_CHANNEL, &dac_dma_transfer_cfg, dac_dma_descriptor);
  }
  return sc;
}

int sl_dac_stop_stream(void)
{
  int sc = -EALREADY;

  if (dac_streaming) {
    sc = 0;
    dac_streaming = false;
    LDMA_StopTransfer(SYS_CNF_DAC_DMA_CHANNEL);
    dac_fifo.rd_idx = dac_fifo.wr_idx;
  }
  return sc;
}

int sl_dac_add_tx_buffer(const int16_t *buffer, uint32_t length)
{
  int sc = audio_drv_check_add_buffer_preconditions(dac_fifo, buffer, length);

  if (!sc) {
    for (uint32_t i = 0; i < length; i++) {
      dac_fifo.data[dac_fifo.wr_idx].buf[i] = (uint16_t)((buffer[i] + 32768L) >> 4UL);
    }
    dac_fifo.data[dac_fifo.wr_idx].len = length;
    dac_fifo.wr_idx = audio_fifo_get_next_write_index(dac_fifo);
  }
  return sc;
}

void sl_dac_on_dma_transfer_complete(void)
{
  static bool print;
  LDMA_Descriptor_t *next_desc = &dac_dma_descriptor[dac_streaming_seq_cnt++ % 2];

  if (dac_fifo.rd_idx != dac_fifo.wr_idx) {
    audio_drv_refresh_dma_descriptor(next_desc, dac_fifo);
    print = true;
  } else {
    SL_ASSERT(!SYS_CNF_VOICE_STOP_AT_ERROR, "FIFO underflow!");
    sl_log_status_warning(print, "FIFO underflow!" SL_LOG_EOL);
    print = false;
  }
}
