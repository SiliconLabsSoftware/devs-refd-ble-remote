/***************************************************************************//**
 * @file sl_pwr_ctrl.c
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
// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
// Private variables -----------------------------------------------------------
// Function definitions --------------------------------------------------------
#include "assert.h"
#include "em_gpio.h"
#include "sl_atomic.h"
#include "sl_pwr_ctrl.h"
#include "../sl_system_config.h"

// Macros ----------------------------------------------------------------------
#if defined(HW_CNF_SS_PWR_CTRL_IO_PORT) && defined(HW_CNF_SS_PWR_CTRL_IO_PIN)
  #define PWR_CTRL_ENABLED 1
#else
  #define PWR_CTRL_ENABLED 0
#endif
// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
// Private variables -----------------------------------------------------------
// Function definitions --------------------------------------------------------
void sl_pwr_ctrl_init(void)
{
#if PWR_CTRL_ENABLED
  if (gpioModePushPull != GPIO_PinModeGet(HW_CNF_SS_PWR_CTRL_IO_PORT, HW_CNF_SS_PWR_CTRL_IO_PIN)) {
    GPIO_PinModeSet(HW_CNF_SS_PWR_CTRL_IO_PORT, HW_CNF_SS_PWR_CTRL_IO_PIN, gpioModePushPull, 0);
  }
#endif
}

void sl_pwr_ctrl_enable(bool enable, sl_pwr_ctrl_domain_t domain)
{
  (void)enable;
  (void)domain; //currently only 1 domain exists, only for future proofing

#if PWR_CTRL_ENABLED
  static int pwr_ctrl_enabled_count = 0;
  SL_ATOMIC_SECTION({
    pwr_ctrl_enabled_count = enable ? (pwr_ctrl_enabled_count + 1) : (pwr_ctrl_enabled_count - 1);
    if (enable && pwr_ctrl_enabled_count == 1) {
      GPIO_PinOutSet(HW_CNF_SS_PWR_CTRL_IO_PORT, HW_CNF_SS_PWR_CTRL_IO_PIN);
    } else if (!enable && pwr_ctrl_enabled_count == 0) {
      GPIO_PinOutClear(HW_CNF_SS_PWR_CTRL_IO_PORT, HW_CNF_SS_PWR_CTRL_IO_PIN);
    }
  });
  SL_ASSERT(pwr_ctrl_enabled_count >= 0, "Power control enable counter is negative!");
#endif
}
