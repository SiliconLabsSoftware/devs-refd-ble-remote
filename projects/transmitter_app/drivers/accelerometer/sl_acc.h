/***************************************************************************//**
 * @file sl_acc.h
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
#ifndef SL_ACC_H
#define SL_ACC_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

// Macros ----------------------------------------------------------------------
// Type definitions ------------------------------------------------------------
// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------

/**
 * @brief Initialize the accelerometer GPIO and SPI interface.
 *
 * This function configures the GPIO pins and SPI peripheral required for the
 * ICM-20648 accelerometer. It must be called before device initialization.
 */
void sl_acc_init_io(void);

/**
 * @brief Initialize the accelerometer (ICM-20648) device.
 *
 * This function initializes the ICM-20648 device and prepares it for use.
 * Call after sl_acc_init_io().
 */
void sl_acc_init_device(void);

/**
 * @brief Deinitialize power down the accelerometer device.
 *
 * @note If the device is deinitialized, it must be reinitialized again before use.
 */
void sl_acc_deinit(void);

/**
 * @brief Get the current acceleration values from the accelerometer.
 *
 * Reads the current acceleration values in mg for the X, Y, and Z axes.
 * The values are returned through the provided pointers.
 *
 * @param x Pointer to store X axis acceleration value in ug.
 * @param y Pointer to store Y axis acceleration value in ug.
 * @param z Pointer to store Z axis acceleration value in ug.
 * @return 0 on success, or an error code on failure.
 */
int sl_acc_get_acceleration(int32_t *x_ug, int32_t *y_ug, int32_t *z_ug);

/**
 * @brief Configure accelerometer threshold values for motion detection.
 *
 * Sets the threshold values (in ug) for the X, Y, and Z axes. When the acceleration
 * on any axis exceeds its threshold, an interrupt will be generated and the user
 * callback will be invoked.
 *
 * @param x_ug Threshold for X axis in ug.
 * @param y_ug Threshold for Y axis in ug.
 * @param z_ug Threshold for Z axis in ug.
 */
int sl_acc_set_thresholds(uint32_t x_ug, uint32_t y_ug, uint32_t z_ug);

/**
 * @brief Enable the accelerometer interrupt for motion/threshold events.
 *
 * This function enables the interrupt source for the accelerometer, allowing
 * the user callback to be triggered when a threshold or motion event occurs.
 */
void sl_acc_interrupt_enable(void);

/**
 * @brief Disable the accelerometer interrupt for motion/threshold events.
 *
 * This function disables the interrupt source for the accelerometer, preventing
 * the user callback from being triggered by motion or threshold events.
 */
void sl_acc_interrupt_disable(void);

/**
 * @brief Clear the accelerometer interrupt flag.
 *
 * This function clears any pending interrupt flag from the accelerometer, so
 * that new events can be detected and handled.
 */
void sl_acc_interrupt_clear(void);

/**
 * @brief Clear the accelerometer interrupt flag in a lazy manner, only after the specified timeout.
 *
 * @note This function is useful to avoid excessive interrupt handling in case of frequent events.
 *       MUST BE CALLED PERIODICALLY FROM THE MAIN LOOP.
 *
 * @param timeout_ms Timeout in milliseconds to wait before clearing the interrupt.
 * @return true if the interrupt was cleared, false if the timeout has not yet elapsed.
 */
bool sl_acc_interrupt_clear_lazy(uint16_t timeout_ms);

/**
 * @brief User callback for accelerometer threshold event.
 *
 * This weakly defined function is called when the configured acceleration threshold
 * is exceeded on any axis. The user can override this function to handle the event.
 */
void sl_acc_on_threshold_reached_event(void);

#ifdef __cplusplus
}
#endif
#endif /* SL_ACC_H */
