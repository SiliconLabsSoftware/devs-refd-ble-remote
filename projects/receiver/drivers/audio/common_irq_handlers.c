/***************************************************************************//**
 * @file common_irq_handlers.c
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
#include "assert.h"
#include "em_ldma.h"
#include "sl_common.h"
#include "sl_i2s.h"
#include "sl_dac.h"
#include "sl_system_config.h"

// Macros ----------------------------------------------------------------------
// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
// Private variables -----------------------------------------------------------
// Function definitions --------------------------------------------------------
void LDMA_IRQHandler(void)
{
  uint32_t pending = LDMA_IntGetEnabled();
  LDMA_IntClear(pending);
  SL_ASSERT(0 == (pending & LDMA_IF_ERROR));

  while (pending) {
    uint32_t channel = SL_CTZ(pending);
    pending &= ~(1UL << channel);

    switch (channel) {
      case SYS_CNF_I2S_DMA_CHANNEL_LEFT:
      case SYS_CNF_I2S_DMA_CHANNEL_RIGHT:
        sl_i2s_on_dma_transfer_complete();
        break;
      case SYS_CNF_DAC_DMA_CHANNEL:
        sl_dac_on_dma_transfer_complete();
        break;
      default:
        break;
    }
  }
}
