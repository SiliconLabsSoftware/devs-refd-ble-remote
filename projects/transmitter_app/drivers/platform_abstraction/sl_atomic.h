/***************************************************************************//**
 * @file sl_atomic.h
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
#ifndef SL_ATOMIC_H
#define SL_ATOMIC_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

// Macros ----------------------------------------------------------------------
#define SL_ATOMIC_STATE_ENTER()    sli_atomic_state_enter()
#define SL_ATOMIC_STATE_EXIT(ctx)  sli_atomic_state_exit(ctx)
#define SL_ATOMIC_SECTION(atomic_code_part)             \
  do {                                                  \
    sl_atomic_context_t ctx =  SL_ATOMIC_STATE_ENTER(); \
    {                                                   \
      atomic_code_part                                  \
    }                                                   \
    SL_ATOMIC_STATE_EXIT(ctx);                          \
  } while (0)

// Type definitions ------------------------------------------------------------
typedef uint32_t sl_atomic_context_t;

// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------
sl_atomic_context_t sli_atomic_state_enter(void);
void sli_atomic_state_exit(sl_atomic_context_t context);

#ifdef __cplusplus
}
#endif
#endif /* SL_ATOMIC_H */
