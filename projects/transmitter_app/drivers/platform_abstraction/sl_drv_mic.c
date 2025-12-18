/***************************************************************************//**
 * @file sl_drv_mic.c
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
#include <errno.h>
#include "em_gpio.h"
#include "sl_gpio.h" //only to verify the pin configuration
#include "sl_system_config.h"
#include "sl_drv_mic.h"
#include "sl_mic.h"
#include "sl_mic_pdm_config.h"
#include "sl_pwr_ctrl.h"

// Macros ----------------------------------------------------------------------
//This driver is just a wrapper for the sl_mic driver. Thus we need to ensure that the pin configuration is OK.
#if SL_MIC_PDM_DAT0_PORT != SYS_CNF_MIC_IO_PDM_DATA_PORT || SL_MIC_PDM_DAT0_PIN != SYS_CNF_MIC_IO_PDM_DATA_PIN
  #error "PDM DAT0 port and pin mismatch. Please check the configuration."
#endif
#if SL_MIC_PDM_CLK_PORT != SYS_CNF_MIC_IO_PDM_CLK_PORT || SL_MIC_PDM_CLK_PIN != SYS_CNF_MIC_IO_PDM_CLK_PIN
  #error "PDM CLK port and pin mismatch. Please check the configuration."
#endif
#if SL_MIC_PDM_DSR < 64
  #error "PDM down sampling rate is too low and with that 8 kHz sampling is not supported. Please check the configuration."
#endif

// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
static void sl_drv_mic_perform_additional_config(const sl_drv_mic_config_t *const config);

// Private variables -----------------------------------------------------------
// Function definitions --------------------------------------------------------
int sl_drv_mic_init(void)
{
  //Disable the microphone to reduce power consumption
#if defined(SYS_CNF_VOICE_MIC_ENABLE_PORT) && defined(SYS_CNF_VOICE_MIC_ENABLE_PIN)
  GPIO_PinModeSet(SYS_CNF_VOICE_MIC_ENABLE_PORT, SYS_CNF_VOICE_MIC_ENABLE_PIN, gpioModePushPull, 0);
#else
  sl_pwr_ctrl_init();
#endif
  return 0;
}

int sl_drv_mic_start(const sl_drv_mic_config_t *const config,
                     sl_drv_mic_buffer_ready_callback_t callback)
{
#if defined(SYS_CNF_VOICE_MIC_ENABLE_PORT) && defined(SYS_CNF_VOICE_MIC_ENABLE_PIN)
  GPIO_PinOutSet(SYS_CNF_VOICE_MIC_ENABLE_PORT, SYS_CNF_VOICE_MIC_ENABLE_PIN);
#else
  sl_pwr_ctrl_enable(true, SL_PWR_CTRL_DOMAIN_MIC);
#endif

  if (   config == NULL || callback == NULL || config->buffer == NULL
         || config->frames == 0 || config->buffer_size == 0
         || config->sample_rate == 0 || config->channels == 0
         || config->channels > 2 || config->delay > 3
         || config->buffer_size < (2 * config->frames * config->channels * sizeof(int16_t))) {
    return -EINVAL;
  }

  int sc = sl_mic_init(config->sample_rate, config->channels);
  if (!sc) {
    sl_drv_mic_perform_additional_config(config);
    sc = sl_mic_start_streaming(config->buffer, config->frames, callback);
  }
  return sc;
}

int sl_drv_mic_stop(void)
{
#if defined(SYS_CNF_VOICE_MIC_ENABLE_PORT) && defined(SYS_CNF_VOICE_MIC_ENABLE_PIN)
  GPIO_PinOutClear(SYS_CNF_VOICE_MIC_ENABLE_PORT, SYS_CNF_VOICE_MIC_ENABLE_PIN);
#else
  sl_pwr_ctrl_enable(false, SL_PWR_CTRL_DOMAIN_MIC);
#endif
  return sl_mic_deinit();
}

static void sl_drv_mic_perform_additional_config(const sl_drv_mic_config_t *const config)
{
  if (config->use_left_mic || config->delay) {
    while (PDM->SYNCBUSY & PDM_SYNCBUSY_SYNCBUSY) ;
    PDM->EN &= ~PDM_EN_EN;
    if (config->channels == 1 && config->use_left_mic) {
      PDM->CFG0 |= (PDM_CFG0_CH0CLKPOL_INVERT | PDM_CFG0_CH1CLKPOL_INVERT);
    }
    if (config->delay > 0) {
      PDM->CFG1 = ((PDM->CFG1 & ~_PDM_CFG1_DLYMUXSEL_MASK) | (config->delay << _PDM_CFG1_DLYMUXSEL_SHIFT));
    }
    PDM->EN |= PDM_EN_EN;
  }
}
