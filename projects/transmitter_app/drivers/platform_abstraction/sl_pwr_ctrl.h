/***************************************************************************//**
 * @file sl_pwr_ctrl.h
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
#ifndef SL_PWR_CTRL_H
#define SL_PWR_CTRL_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

// Macros ----------------------------------------------------------------------
// Type definitions ------------------------------------------------------------
typedef enum {
  SL_PWR_CTRL_DOMAIN_BATTERY,
  SL_PWR_CTRL_DOMAIN_MIC,
  SL_PWR_CTRL_DOMAIN_MAX
} sl_pwr_ctrl_domain_t;

// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------
/**
 * @brief Initialize the power control module.
 *
 * This function initializes the power control system and sets up the necessary
 * hardware resources for managing power domains.
 */
void sl_pwr_ctrl_init(void);

/**
 * @brief Enable or disable power for a specific domain.
 *
 * @param enable True to enable power, false to disable power for the domain
 * @param domain The power domain to control (battery, microphone, etc.)
 */
void sl_pwr_ctrl_enable(bool enable, sl_pwr_ctrl_domain_t domain);

#ifdef __cplusplus
}
#endif
#endif /* SL_PWR_CTRL_H */
