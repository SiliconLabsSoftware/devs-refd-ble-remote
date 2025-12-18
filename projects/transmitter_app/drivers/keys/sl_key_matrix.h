/***************************************************************************//**
 * @file sl_key_matrix.h
 * @brief Key Matrix Driver for 6x8 matrix with extra button
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
#ifndef SL_KEY_MATRIX_H
#define SL_KEY_MATRIX_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

// Macros ----------------------------------------------------------------------
// Type definitions ------------------------------------------------------------
// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------

/***************************************************************************//**
 * @brief Initialize the key matrix driver.
 *
 * This function initializes the GPIO pins for the key matrix rows and columns,
 * configures interrupts, and sets up the scanning mechanism.
 *
 * @return 0 on success, or an error code on failure.
 ******************************************************************************/
int sl_key_matrix_init(void);

/***************************************************************************//**
 * @brief User callback for key matrix events (pressed or released).
 *
 * This weakly defined function is called when a key event occurs.
 * The user can override this function to handle key events.
 *
 * @param[in] keys_pressed    Bitmask of currently pressed keys.
 *                            1 if the key is pressed, 0 if not.
 ******************************************************************************/
void sl_key_matrix_on_event(uint64_t keys_pressed);

/***************************************************************************//**
 * @brief Inject a key press event for testing purposes.
 *
 * This function simulates a key press event by directly injecting a key ID
 * and a duration for which the key is considered pressed.
 *
 * @note Only available if SYS_CNF_TEST_EN_KEYS is set (by default in DEBUG builds).
 *
 * @param[in] key_id         The ID of the key to simulate (0-63).
 * @param[in] duration_ms    Duration in milliseconds for which the key is pressed.
 *
 * @return 0 on success, or an error code on failure.
 ******************************************************************************/
int sl_key_matrix_test_inject_key_press(uint8_t key_id, uint32_t duration_ms);

#ifdef __cplusplus
}
#endif
#endif /* SL_KEY_MATRIX_H */
