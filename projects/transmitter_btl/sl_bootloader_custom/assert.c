/***************************************************************************//**
 * @file assert.c
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
#include <cmsis_compiler.h>
#include "assert.h"
#include "sl_assert.h"

// Macros ----------------------------------------------------------------------
// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
// Private variables -----------------------------------------------------------
// Function definitions --------------------------------------------------------
void sli_assert(void)
{
#if DEBUG
  __BKPT(0);
#endif
  __disable_irq();
  while (1) ;
}

void assertEFM(const char *file, int line)
{
  (void)file;
  (void)line;
  sli_assert();
}
