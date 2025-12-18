/***************************************************************************//**
 * @file
 * @brief Non-Volatile Memory (NVM) driver implementation.
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
#include <errno.h>
#include "sl_nvm.h"
#include "nvm3_default.h"

// Macros ----------------------------------------------------------------------
#define SL_NVM_ID_OFFSET  0x00100
#define SL_NVM_ID_MAX     (0xFFFFF - SL_NVM_ID_OFFSET)

// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
// Private variables -----------------------------------------------------------
// Function definitions --------------------------------------------------------
int sl_nvm_write(uint32_t id, const void *data, size_t length)
{
  int sc = -EBADF;
  if (id <= SL_NVM_ID_MAX) {
    sc = nvm3_writeData(nvm3_defaultHandle, id + SL_NVM_ID_OFFSET, data, length);
  }
  return sc;
}

int sl_nvm_read(uint32_t id, void *data, size_t length)
{
  int sc = -EBADF;
  if (id <= SL_NVM_ID_MAX) {
    sc = nvm3_readData(nvm3_defaultHandle, id + SL_NVM_ID_OFFSET, data, length);
  }
  return sc;
}
