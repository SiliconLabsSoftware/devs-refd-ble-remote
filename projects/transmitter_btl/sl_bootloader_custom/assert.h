/***************************************************************************//**
 * @file assert.h
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
#ifndef ASSERT_H
#define ASSERT_H
#ifdef __cplusplus
extern "C" {
#endif

// Macros ----------------------------------------------------------------------
#define __SLI_ASSERT_NUM_TO_STR(x) #x
#define SLI_ASSERT_NUM_TO_STR(x)   __SLI_ASSERT_NUM_TO_STR(x)

#define SL_STATIC_ASSERT(condition, ...) _Static_assert(condition, ##__VA_ARGS__)

#if SL_ASSERT_ENABLED || DEBUG
  #define SL_ASSERT(condition, ...) if (!(condition)) \
    sli_assert()
#else
  #define SL_ASSERT(condition, ...) ((void)(condition))
#endif

// Type definitions ------------------------------------------------------------
// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------
void sli_assert(void);

#ifdef __cplusplus
}
#endif
#endif /* ASSERT_H */
