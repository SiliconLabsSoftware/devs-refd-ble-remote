/***************************************************************************//**
 * @file sl_ir_nec_decoder.h
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
#ifndef SL_IR_NEC_DECODER_H
#define SL_IR_NEC_DECODER_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>

// Includes --------------------------------------------------------------------
// Macros ----------------------------------------------------------------------
#define SL_IR_NEC_DEC_ADDR(data_u32)      (((data_u32) >> 0)  & 0xFF)
#define SL_IR_NEC_DEC_ADDR_INV(data_u32)  (((data_u32) >> 8)  & 0xFF)
#define SL_IR_NEC_DEC_ADDR_U16(data_u32)  ((data_u32) & 0xFFFF)
#define SL_IR_NEC_DEC_CMD(data_u32)       (((data_u32) >> 16) & 0xFF)
#define SL_IR_NEC_DEC_CMD_INV(data_u32)   (((data_u32) >> 24) & 0xFF)

// Type definitions ------------------------------------------------------------
// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------

/***************************************************************************//**
 * @brief Initialize the NEC IR decoder component.
 ******************************************************************************/
void sl_ir_nec_decoder_init(void);

/***************************************************************************//**
 * @brief Cyclic executor of the NEC IR decoder component.
 ******************************************************************************/
void sl_ir_nec_decoder_cyclic(void);

/***************************************************************************//**
 * @brief Weakly defined callback for decoded NEC IR data.
 * This function is called when a valid NEC frame or repeat code is received.
 *
 * @param[in] data      The 32-bit NEC frame data (valid only if is_repeat is false).
 *                      Use the macros (SL_IR_NEC_DEC_*) to extract address and command.
 * @param[in] is_repeat True if this is a repeat code, false for a normal frame.
 ******************************************************************************/
void sl_ir_nec_decoder_on_data(uint32_t data, bool is_repeat);

#ifdef __cplusplus
}
#endif
#endif /* SL_IR_NEC_DECODER_H */
