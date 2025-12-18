/***************************************************************************//**
 * @file sl_log.c
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
#include "sl_log.h"
#include "printf.h"

// Macros ----------------------------------------------------------------------
// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
// Private variables -----------------------------------------------------------
// Function definitions --------------------------------------------------------
int sli_internal_log(const char* format, ...)
{
  va_list args;
  va_start(args, format);
  int ret = vprintf(format, args);
  va_end(args);
  return ret;
}
