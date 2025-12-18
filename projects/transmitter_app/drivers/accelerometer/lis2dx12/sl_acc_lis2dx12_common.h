/***************************************************************************//**
 * @file sl_acc_lis2dx12_common.h
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
#ifndef SL_ACC_LIS2DX12_COMMON_H
#define SL_ACC_LIS2DX12_COMMON_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

// Macros ----------------------------------------------------------------------
#define lis2dx12_acc_raw_to_ug(raw_value) \
  ((raw_value) * (SYS_CNF_ACC_LIS2Dx12_RESOLUTION_G * 1000000LL) / 32768L)

// Type definitions ------------------------------------------------------------
// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------
void sli_acc_init_i2c_lis2dx12(void);
void sli_acc_init_gpio_lis2dx12(void (*irq_callback)(void));
void sli_acc_interrupt_enable_lis2dx12(void);
void sli_acc_interrupt_disable_lis2dx12(void);
int32_t sli_acc_register_write_lis2dx12(void *handle_as_address, uint8_t reg, const uint8_t *data, uint16_t len);
int32_t sli_acc_register_read_lis2dx12(void *handle_as_address, uint8_t reg, uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif
#endif /* SL_ACC_LIS2DX12_COMMON_H */
