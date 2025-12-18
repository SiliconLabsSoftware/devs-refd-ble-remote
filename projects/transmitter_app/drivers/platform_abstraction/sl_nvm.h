/***************************************************************************//**
 * @file
 * @brief Non-Volatile Memory (NVM) driver header.
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#ifndef SL_NVM_H
#define SL_NVM_H
#include <stdint.h>
#include <stddef.h>

// Macros ----------------------------------------------------------------------
// Type definitions ------------------------------------------------------------
// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------

/***************************************************************************//**
 * @brief Write data to NVM.
 *
 * @param id Identifier in NVM to write to.
 * @param data Pointer to the data to write.
 * @param length Length of the data in bytes.
 *
 * @return 0 on success, non-zero error code on failure.
 ******************************************************************************/
int sl_nvm_write(uint32_t id, const void *data, size_t length);

/***************************************************************************//**
 * @brief Read data from NVM.
 *
 * @param id Identifier in NVM to read from.
 * @param data Pointer to the buffer to store the read data.
 * @param length Length of the data in bytes.
 *
 * @return 0 on success, non-zero error code on failure.
 ******************************************************************************/
int sl_nvm_read(uint32_t id, void *data, size_t length);

#endif // SL_NVM_H
