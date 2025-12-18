/***************************************************************************//**
 * @file sl_dwt_handler.c
 * @brief DWT timer handler implementation
 *******************************************************************************
 * # License
 * <b>Copyright 2021 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc.  Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.  This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
#include "sl_dwt_handler.h"
#include "assert.h"
#include "em_cmu.h"
#include "sl_system_config.h"

// Local function prototypes ---------------------------------------------------

/***************************************************************************//**
 * @brief Get the current core clock frequency in Hz based on the active clock source.
 *
 * This function reads the current HFCLK source from CMU->SYSCLKCTRL and returns
 * the corresponding frequency as configured in compile-time constants.
 *
 * @return Core clock frequency in Hz.
 ******************************************************************************/
static uint32_t sl_dwt_handler_get_core_clock_freq(void);

// Local function definitions ---------------------------------------------------
static uint32_t sl_dwt_handler_get_core_clock_freq(void)
{
  const uint32_t clk_sel = CMU->SYSCLKCTRL & _CMU_SYSCLKCTRL_CLKSEL_MASK;

  switch (clk_sel) {
    case CMU_SYSCLKCTRL_CLKSEL_FSRCO:
      return HW_CNF_CORE_CLOCK_FSSRCO_FREQ_HZ;
    case CMU_SYSCLKCTRL_CLKSEL_HFRCODPLL:
      return HW_CNF_CORE_CLOCK_HFRCODPLL_STARTUP_FREQ_HZ;
    case CMU_SYSCLKCTRL_CLKSEL_HFXO:
      return HW_CNF_CORE_CLOCK_HFXO_FREQ_HZ;
    case CMU_SYSCLKCTRL_CLKSEL_CLKIN0:
      return HW_CNF_CORE_CLOCK_CLKIN0_FREQ_HZ;
    default:
      SL_ASSERT(false); // Unsupported clock source
      return 0; // Fallback, should not be reached
  }
}

// Function definitions --------------------------------------------------------
void sl_dwt_handler_init(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // Enable the trace
  ITM->LAR = 0xC5ACCE55; // Unlock ITM
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; // Enable cycle counter
}

void sl_dwt_handler_deinit(void)
{
  DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk; // Disable cycle counter
  ITM->LAR = 0x00000000;
  CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk; // Disable the trace
}

uint32_t sl_dwt_handler_get_cycle_count(void)
{
  SL_ASSERT(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk);
  return DWT->CYCCNT;
}

bool sl_dwt_handler_is_elapsed_ms(uint32_t start_cycle, uint32_t timeout_ms)
{
  SL_ASSERT(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk);

  const uint32_t elapsed_cycles = DWT->CYCCNT - start_cycle;
  const uint32_t core_freq_coef = sl_dwt_handler_get_core_clock_freq();

  SL_ASSERT(core_freq_coef > 0);

  const uint32_t timeout_cycles = timeout_ms * (core_freq_coef / 1000UL);

  return (elapsed_cycles >= timeout_cycles);
}

void sl_dwt_handler_blocking_delay_us(uint32_t timeout_us)
{
  SL_ASSERT(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk);

  const uint32_t start_cycle = DWT->CYCCNT;
  const uint32_t core_freq_coef = sl_dwt_handler_get_core_clock_freq();

  SL_ASSERT(core_freq_coef > 0);

  const uint32_t timeout_cycles = timeout_us * (core_freq_coef / 1000000UL);

  while ((DWT->CYCCNT - start_cycle) < timeout_cycles) ;
}
