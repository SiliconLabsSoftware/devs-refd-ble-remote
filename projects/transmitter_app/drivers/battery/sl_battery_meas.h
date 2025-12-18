/***************************************************************************//**
 * @file sl_battery_meas.h
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
#ifndef SL_BATTERY_MEAS_H
#define SL_BATTERY_MEAS_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

// Macros ----------------------------------------------------------------------
// Type definitions ------------------------------------------------------------
// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------

/**
 * @brief Initialize the battery measurement module.
 *
 * This function initializes the battery measurement hardware and prepares it for use.
 * Default period time is set from the system configuration.
 */
void sl_battery_meas_init(void);

/**
 * @brief Cyclic function of the battery measurement.
 */
void sl_battery_meas_cyclic(void);

/**
 * @brief Get the last measured battery voltage in microvolts (uV).
 *
 * This function returns the most recent battery voltage measurement result.
 *
 * @return Battery voltage in microvolts (uV).
 */
uint32_t sl_battery_meas_get_voltage_uv(void);

/**
 * @brief Set the battery measurement period time in milliseconds.
 *
 * Sets the period time (in milliseconds) for automatic battery measurement.
 * A value of 0 disables periodic measurement.
 *
 * @param period_ms Period time in milliseconds. 0 disables periodic measurement.
 */
void sl_battery_meas_set_period_time_ms(uint32_t period_ms);

/**
 * @brief Get the current battery measurement period time in milliseconds.
 *
 * This function returns the currently configured period time for automatic battery measurement.
 *
 * @return Period time in milliseconds. 0 means periodic measurement is disabled.
 */
uint32_t sl_battery_meas_get_period_time_ms(void);

/**
 * @brief Callback invoked when a battery measurement is complete.
 *
 * This is a weakly defined function that can be overridden by the application
 * to handle battery measurement completion events.
 *
 * @param microvolt The measured battery voltage in  uV.
 */
void sl_battery_meas_on_complete(uint32_t microvolt);

#ifdef __cplusplus
}
#endif
#endif /* SL_BATTERY_MEAS_H */
