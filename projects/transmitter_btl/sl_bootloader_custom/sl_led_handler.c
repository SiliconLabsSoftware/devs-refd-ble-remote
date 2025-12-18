/***************************************************************************//**
 * @file sl_led_handler.c
 * @brief LED handler implementation file
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc.  Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.  This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include "sl_led_handler.h"
#include "em_cmu.h"
#include "em_letimer.h"
#include "sl_gpio.h"
#include "assert.h"
#include "sl_system_config.h"

// Global variables -------------------------------------------------------------
static const uint32_t led_activity[] = SYS_CNF_LED_ACTIVITY_CONFIG;

// Function
void sl_led_handler_init(void)
{
  sl_status_t status;

  // Initialize the GPIO subsystem
  status = sl_gpio_init();
  SL_ASSERT(status == SL_STATUS_OK);

  for (uint32_t i = 0; i < SYS_CNF_ARR_LEN(led_activity); i++) {
    const sl_gpio_t led_config = {
      .port = SYS_CNF_PORT_PIN_GET_PORT(led_activity[i]),
      .pin = SYS_CNF_PORT_PIN_GET_PIN(led_activity[i])
    };

    SL_ASSERT(SL_STATUS_OK == sl_gpio_set_pin_mode(&led_config, SL_GPIO_MODE_PUSH_PULL, 0));
  }
}

void sl_led_handler_deinit(void)
{
  for (uint32_t i = 0; i < SYS_CNF_ARR_LEN(led_activity); i++) {
    const sl_gpio_t led_config = {
      .port = SYS_CNF_PORT_PIN_GET_PORT(led_activity[i]),
      .pin = SYS_CNF_PORT_PIN_GET_PIN(led_activity[i])
    };
    sl_gpio_set_pin_mode(&led_config, SL_GPIO_MODE_DISABLED, 0);
  }
  CMU_ClockEnable(cmuClock_GPIO, false);
}

void sl_led_on(void)
{
  for (uint32_t i = 0; i < SYS_CNF_ARR_LEN(led_activity); i++) {
    const sl_gpio_t led_config = {
      .port = SYS_CNF_PORT_PIN_GET_PORT(led_activity[i]),
      .pin = SYS_CNF_PORT_PIN_GET_PIN(led_activity[i])
    };
    sl_gpio_set_pin(&led_config);
  }
}

void sl_led_off(void)
{
  for (uint32_t i = 0; i < SYS_CNF_ARR_LEN(led_activity); i++) {
    const sl_gpio_t led_config = {
      .port = SYS_CNF_PORT_PIN_GET_PORT(led_activity[i]),
      .pin = SYS_CNF_PORT_PIN_GET_PIN(led_activity[i])
    };
    sl_gpio_clear_pin(&led_config);
  }
}

void sl_led_toggle(void)
{
  for (uint32_t i = 0; i < SYS_CNF_ARR_LEN(led_activity); i++) {
    const sl_gpio_t led_config = {
      .port = SYS_CNF_PORT_PIN_GET_PORT(led_activity[i]),
      .pin = SYS_CNF_PORT_PIN_GET_PIN(led_activity[i])
    };
    sl_gpio_toggle_pin(&led_config);
  }
}
