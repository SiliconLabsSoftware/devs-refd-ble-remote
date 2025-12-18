/***************************************************************************//**
 * @file sl_led_effect.c
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
#include <string.h>
#include <errno.h>
#include "assert.h"
#include "dmadrv.h"
#include "em_cmu.h"
#include "em_timer.h"
#include "sl_led_effect.h"
#include "sl_system_config.h"
#include "sl_sleeptimer.h"

// Macros ----------------------------------------------------------------------
#ifndef CAT3
  #define _CAT3(a, b, c) a##b##c
  #define CAT3(a, b, c) _CAT3(a, b, c)
#endif
#ifndef CAT2
  #define CAT2(a, b) CAT3(a, b, )
#endif
#define LED_EFFECT_TIMER_INST CAT2(TIMER, SYS_CNF_LED_EFFECT_TIMER_NUM)

// Private type definitions ----------------------------------------------------
typedef struct {
  LDMA_Descriptor_t dma_descr;
  unsigned int dma_channel;
  const sl_led_t *led;
  sl_led_effect_callback_t cbk_effect_done;
  sl_led_effect_callback_t cbk_timeout;
  sl_sleeptimer_timer_handle_t tout_timer_handle;
} led_effect_t;

// Private function prototypes -------------------------------------------------
static int  led_effect_init_timer(led_effect_t *eff);
static int  led_effect_init_dma(led_effect_t *eff);
static bool led_effect_dma_done_callback(unsigned int channel, unsigned int sequenceNo, void *userParam);
static void led_effect_timeout_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data);

// Private variables -----------------------------------------------------------
// Function definitions --------------------------------------------------------
int sl_led_effect_init(sl_led_effect_t *effect, const sl_led_t *led)
{
  SL_STATIC_ASSERT(sizeof(effect->context_buffer) >= sizeof(led_effect_t), "Provided context buffer is too small!");
  int sc = 0;

  if (!effect || !led || led->type != SL_LED_TYPE_PWM) {
    sc = -EINVAL; // Invalid parameters
  } else {
    led_effect_t *l_eff = (led_effect_t *)effect->context_buffer;
    l_eff->led = led;
    l_eff->cbk_effect_done = NULL;
    l_eff->cbk_timeout = NULL;
    memset(&l_eff->tout_timer_handle, 0, sizeof(l_eff->tout_timer_handle));
    sc = sl_led_init(l_eff->led);
    if (!sc) {
      sc = led_effect_init_dma(l_eff);
    }
    if (!sc) {
      sc = led_effect_init_timer(l_eff);
    }
  }
  return sc;
}

static int led_effect_init_timer(led_effect_t *eff)
{
  (void)eff;
  CMU_ClockEnable(CAT2(cmuClock_TIMER, SYS_CNF_LED_EFFECT_TIMER_NUM), true);

  //find the divider for the 16 bit timer which allows the timer to be on for the configured max time
  uint32_t scaled_freq = 0;
  uint32_t timer_freq = CMU_ClockFreqGet(CAT2(cmuClock_TIMER, SYS_CNF_LED_EFFECT_TIMER_NUM));
  for (uint32_t i = 1; i <= 1024 && !scaled_freq; i *= 2) {
    if (((timer_freq / i) * SL_LED_PATTERN_MIN_TIME_MS / 1000) < UINT16_MAX) {
      scaled_freq = (timer_freq / i);
    }
  }
  SL_ASSERT(scaled_freq, "Timer frequency not set!");

  TIMER_Init_TypeDef timer_init = TIMER_INIT_DEFAULT;
  timer_init.dmaClrAct = true;
  timer_init.prescale = (timer_freq / scaled_freq) - 1;
  TIMER_Init(LED_EFFECT_TIMER_INST, &timer_init);
  TIMER_TopSet(LED_EFFECT_TIMER_INST, (SL_LED_PATTERN_MIN_TIME_MS * scaled_freq / 1000));
  return 0;
}

static int led_effect_init_dma(led_effect_t *eff)
{
  int sc = DMADRV_Init();
  if (sc == (int)ECODE_EMDRV_DMADRV_ALREADY_INITIALIZED) {
    sc = 0; // Already initialized, no error
  }

  if (!sc) {
    eff->dma_descr = (LDMA_Descriptor_t)LDMA_DESCRIPTOR_SINGLE_M2P_BYTE(NULL, &CAT2(TIMER, SYS_CNF_LED_PWM_TIMER_NUM)->CC[eff->led->tim_oc_nbr].OC, 1);
    eff->dma_descr.xfer.size = ldmaCtrlSizeHalf; // Set transfer size to half-word
    eff->dma_descr.xfer.ignoreSrec = false;
    sc = DMADRV_AllocateChannel(&eff->dma_channel, NULL);
  }
  return sc;
}

int sl_led_effect_start(sl_led_effect_t *effect, const sl_led_effect_pattern_t *pattern, uint8_t repeat_count, sl_led_effect_callback_t effect_done_callback)
{
  SL_ASSERT(effect != NULL);
  SL_ASSERT(pattern != NULL);
  SL_ASSERT(pattern->pwm_values != NULL);
  SL_ASSERT(pattern->length > 0);

  led_effect_t *l_eff = (led_effect_t *)effect->context_buffer;
  l_eff->dma_descr.xfer.srcAddr = (uint32_t)(pattern->pwm_values);
  l_eff->dma_descr.xfer.xferCnt = pattern->length - 1;
  l_eff->dma_descr.xfer.link = (repeat_count != 1) ? 1 : 0;
  l_eff->dma_descr.xfer.linkMode = ldmaLinkModeRel;
  l_eff->dma_descr.xfer.linkAddr = 0;
  l_eff->dma_descr.xfer.decLoopCnt = (repeat_count > 1);
  l_eff->dma_descr.xfer.doneIfs = effect_done_callback ? true : false;
  l_eff->cbk_effect_done = effect_done_callback;

  LDMA_TransferCfg_t transf_cfg = LDMA_TRANSFER_CFG_PERIPHERAL(CAT3(ldmaPeripheralSignal_TIMER, SYS_CNF_LED_EFFECT_TIMER_NUM, _UFOF));
  transf_cfg.ldmaLoopCnt = repeat_count;
  return DMADRV_LdmaStartTransfer(l_eff->dma_channel, &transf_cfg, &l_eff->dma_descr, led_effect_dma_done_callback, l_eff);
}

static bool led_effect_dma_done_callback(unsigned int channel, unsigned int sequenceNo, void *userParam)
{
  (void)channel;
  (void)sequenceNo;
  led_effect_t *l_eff = (led_effect_t *)userParam;

  if (l_eff && l_eff->cbk_effect_done) {
    l_eff->cbk_effect_done(userParam);
  }
  return false;
}

int sl_led_effect_stop(sl_led_effect_t *effect)
{
  SL_ASSERT(effect);
  return DMADRV_StopTransfer(((led_effect_t *)effect)->dma_channel);
}

bool sl_led_effect_is_running(sl_led_effect_t *effect)
{
  SL_ASSERT(effect);
  bool active = false;
  int sc = DMADRV_TransferActive(((led_effect_t *)effect)->dma_channel, &active);
  return sc == 0 && active;
}

int sl_led_effect_timer_start(sl_led_effect_t *effect, uint32_t time_ms, sl_led_effect_callback_t timeout_callback)
{
  int sc = -EINVAL;
  led_effect_t *l_eff = (led_effect_t *)effect->context_buffer;

  if (timeout_callback && time_ms) {
    l_eff->cbk_timeout = timeout_callback;
    sc = sl_sleeptimer_restart_timer_ms(&l_eff->tout_timer_handle,
                                        time_ms,
                                        led_effect_timeout_timer_callback,
                                        l_eff,
                                        0xFE,
                                        SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
  }
  return sc;
}

static void led_effect_timeout_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void)handle;
  led_effect_t *l_eff = (led_effect_t *)((sl_led_effect_t *)data)->context_buffer;

  if (l_eff && l_eff->cbk_timeout) {
    l_eff->cbk_timeout(data);
  }
}
