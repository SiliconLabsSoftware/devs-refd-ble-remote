/***************************************************************************//**
 * @file callbacks.c
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
#include "sl_ble.h"
#include "voice.h"
#include "receiver.h"

// Macros ----------------------------------------------------------------------
// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
// Private variables -----------------------------------------------------------
// Function definitions --------------------------------------------------------
void sl_ble_on_remote_connected(const uint8_t *addr, size_t addr_len)
{
  (void)addr;
  (void)addr_len;
  sl_voice_on_remote_connected();
  sl_receiver_on_remote_connected();
}
