/***************************************************************************//**
 * @file sl_led.c
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
#include "assert.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_timer.h"
#include "sl_led.h"
#include "sl_system_config.h"

// Macros ----------------------------------------------------------------------
#ifndef CAT3
  #define _CAT3(a, b, c) a##b##c
  #define CAT3(a, b, c) _CAT3(a, b, c)
#endif
#ifndef CAT2
  #define CAT2(a, b) CAT3(a, b, )
#endif
#define LED_TIMER_INST CAT2(TIMER, SYS_CNF_LED_PWM_TIMER_NUM)
#if SYS_CNF_LED_PWM_TIMER_NUM != 0 && SYS_CNF_LED_PWM_TIMER_NUM != 1
  #error "LED PWM timer number must be 0 or 1! (To surely support all GPIO routes!)"
#endif

// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
// Private variables -----------------------------------------------------------
// Function definitions --------------------------------------------------------
int sl_led_init(const sl_led_t *led)
{
  int sc = 0;
  volatile uint32_t *cc_route = (&GPIO->TIMERROUTE[SYS_CNF_LED_PWM_TIMER_NUM].CC0ROUTE) + led->tim_oc_nbr;

  if ((NULL == led)
      || (led->max_level == 0)
      || (led->tim_oc_nbr >= CAT3(TIMER, SYS_CNF_LED_PWM_TIMER_NUM, _CC_NUM))
      || ((SL_LED_TYPE_PWM != led->type) && (SL_LED_TYPE_GPIO != led->type))
      || ((SL_LED_POLARITY_ACTIVE_LOW != led->polarity) && (SL_LED_POLARITY_ACTIVE_HIGH != led->polarity))) {
    sc = -EINVAL;
  } else if (SL_LED_TYPE_PWM == led->type && *cc_route != GPIO_TIMER_CC0ROUTE_PIN_DEFAULT) {
    sc = -EBUSY; // The output compare channel is already in use
  } else {
    CMU_ClockEnable(cmuClock_GPIO, true);
    if (SL_LED_TYPE_PWM == led->type) {
      CMU_ClockEnable(CAT2(cmuClock_TIMER, SYS_CNF_LED_PWM_TIMER_NUM), true);
      // First time initialization
      if (0 == (LED_TIMER_INST->STATUS & TIMER_STATUS_RUNNING)) {
        // Init the TIMER
        const TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
        TIMER_Init(LED_TIMER_INST, &timerInit);
        TIMER_TopSet(LED_TIMER_INST, UINT16_MAX);
      }
      // Configure TIMER for PWM mode
      TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
      timerCCInit.mode = timerCCModePWM; // PWM mode
      timerCCInit.outInvert = (SL_LED_POLARITY_ACTIVE_LOW == led->polarity);
      TIMER_InitCC(LED_TIMER_INST, led->tim_oc_nbr, &timerCCInit);
      TIMER_CompareSet(LED_TIMER_INST, led->tim_oc_nbr, 0);
      TIMER_Enable(LED_TIMER_INST, true);
      // Route the PWM signal to a GPIO pin
      GPIO_PinModeSet(led->port, led->pin, gpioModePushPullAlternate, 0);
      *cc_route = ((led->port << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT) | (led->pin << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT));
      GPIO->TIMERROUTE[SYS_CNF_LED_PWM_TIMER_NUM].ROUTEEN |= (1UL << led->tim_oc_nbr);
    } else {
      GPIO_PinModeSet(led->port, led->pin, gpioModePushPull, !led->polarity);
    }
  }
  return sc;
}

void sl_led_turn_on(const sl_led_t *led)
{
  SL_ASSERT(led);
  sl_led_set_level(led, led->max_level);
}

void sl_led_turn_off(const sl_led_t *led)
{
  SL_ASSERT(led);
  sl_led_set_level(led, 0);
}

void sl_led_toggle(const sl_led_t *led)
{
  SL_ASSERT(led);
  if (sl_led_get_state(led)) {
    sl_led_turn_off(led);
  } else {
    sl_led_turn_on(led);
  }
}

sl_led_state_t sl_led_get_state(const sl_led_t *led)
{
  SL_ASSERT(led);
  return sl_led_get_level(led) ? SL_LED_STATE_ON : SL_LED_STATE_OFF;
}

void sl_led_set_level(const sl_led_t *led, uint16_t level)
{
  SL_ASSERT(led);
  if (SL_LED_TYPE_PWM == led->type) {
    TIMER_CompareSet(LED_TIMER_INST, led->tim_oc_nbr, level);
  } else if ((level && (led->polarity == SL_LED_POLARITY_ACTIVE_LOW))
             || (!level && (led->polarity == SL_LED_POLARITY_ACTIVE_HIGH))) {
    GPIO_PinOutClear(led->port, led->pin);
  } else {
    GPIO_PinOutSet(led->port, led->pin);
  }
}

uint16_t sl_led_get_level(const sl_led_t *led)
{
  SL_ASSERT(led);
  uint16_t level;

  if (SL_LED_TYPE_PWM == led->type) {
    level = (uint16_t)LED_TIMER_INST->CC[led->tim_oc_nbr].OC;
  } else if (led->polarity == SL_LED_POLARITY_ACTIVE_LOW) {
    level = (uint16_t)(!GPIO_PinOutGet(led->port, led->pin) * led->max_level);
  } else {
    level = (uint16_t)(GPIO_PinOutGet(led->port, led->pin) * led->max_level);
  }
  return level;
}
