/***************************************************************************//**
 * @file sl_sleep.h
 * @brief General sleep/timer component
 * @version 1.0.0
 ******************************************************************************/
#ifndef SL_SLEEP_H
#define SL_SLEEP_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

// Macros ----------------------------------------------------------------------
// Type definitions ------------------------------------------------------------
// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------

/**
 * @brief Initialize the sleep component.
 *
 * This function can also initializes a timer with the specified timeout in milliseconds.
 * When the timer expires, the system will enter standby state.
 */
void sl_sleep_init(void);

/**
 * @brief Enable or disable (light) sleep (when waiting for interrupts).
 *
 * @warning To be able to use by multiple components, it uses a call count mechanism (disable and enable count shall match to re-enable).
 *
 * @param enable If true, light sleep is enabled. If false, light sleep is disabled (CPU continuously runs).
 */
void sl_sleep_light_enable(bool enable);

/**
 * @brief Restrict or unrestrict deep sleep entry.
 *  This function enables or disables the ability of the system to enter deep sleep mode.
 *
 * @warning To be able to use by multiple components, it uses a call count mechanism (disable and enable count shall match to re-enable).
 *
 *  @param enable If true, sleep entry is restricted (disabled). If false, sleep entry is unrestricted (enabled).
 */
void sl_sleep_deep_enable(bool enable);

/**
 * @brief Reset the sleep timer.
 * * This function resets (restarts) the standby sleep timer, postponing standby state (EM4) entry.
 */
void sl_sleep_reset_standby_timer(void);

/**
 * @brief Hook function called before entering standby state.
 *
 * This function is called before the system enters standby state (EM4).
 * It can be used to perform any necessary actions before entering deep sleep.
 */
void sl_sleep_standby_hook(void);

#ifdef __cplusplus
}
#endif
#endif /* SL_SLEEP_H */
