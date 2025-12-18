/***************************************************************************//**
 * @file sl_acc_lis2dw12.c
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

// Macros ----------------------------------------------------------------------
// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
// Private variables -----------------------------------------------------------
// Function definitions --------------------------------------------------------
#include <errno.h>
#include "assert.h"
#include "lis2dw12_reg.h"
#include "sl_acc_lis2dx12_common.h"
#include "sl_sleeptimer.h"
#include "sl_system_config.h"

// Macros ----------------------------------------------------------------------
#define LIS2DW12_RESET_DELAY_MS         1
#define LIS2DW12_BOOT_DELAY_MS          20
#define lis2dw12_wr_reg_simple(address, register_ptr) \
  sli_acc_register_write_lis2dx12(lis2dw12_ctx.handle, (address), (void *)(register_ptr), 1)
#define lis2dw12_acc_mg_threshold_to_raw(acc_thd_mg) \
  ((uint8_t)((64 * (acc_thd_mg)) / (SYS_CNF_ACC_LIS2Dx12_RESOLUTION_G * 1000)))

// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
// Private variables -----------------------------------------------------------
static const stmdev_ctx_t lis2dw12_ctx = {
  .write_reg = sli_acc_register_write_lis2dx12,
  .read_reg = sli_acc_register_read_lis2dx12,
  .handle = (void *)LIS2DW12_I2C_ADD_H,
};

// Function definitions --------------------------------------------------------
SL_WEAK void sli_acc_on_threshold_reached_event_lis2dw12(void)
{
}

void sli_acc_init_io_lis2dw12(void)
{
  sli_acc_init_i2c_lis2dx12();
  sli_acc_init_gpio_lis2dx12(sli_acc_on_threshold_reached_event_lis2dw12);
}

void sli_acc_deinit_lis2dw12(void)
{
  // Set ODR to 0 thus powering down the device
  lis2dw12_ctrl1_t ctrl_reg1 = { .odr = LIS2DW12_XL_ODR_OFF };
  lis2dw12_wr_reg_simple(LIS2DW12_CTRL1, &ctrl_reg1);
}

int sli_acc_set_thresholds_lis2dw12(uint32_t x_ug, uint32_t y_ug, uint32_t z_ug)
{
  uint32_t wom_thd_mg = (x_ug + y_ug + z_ug) / 3000;
  lis2dw12_wake_up_ths_t wake_up_ths = {
    .sleep_on = PROPERTY_ENABLE,
    .wk_ths = lis2dw12_acc_mg_threshold_to_raw(wom_thd_mg),
  };
  return lis2dw12_wr_reg_simple(LIS2DW12_WAKE_UP_THS, &wake_up_ths);
}

void sli_acc_init_device_lis2dw12(void)
{
  // !!!
  // Note that the LIS2Dx12 APIs are deliberately not used because that would
  // result in multiple read/writes on the same register thus slowing down the init process
  // and draining the battery.
  // !!!
  sli_acc_interrupt_disable_lis2dx12();

  //Reset
  lis2dw12_ctrl2_t ctrl_reg2 = {
    .boot = 0,
    .soft_reset = 1,
    .if_add_inc = 1,
    .bdu = 1, // Block data update (because read is not synchronized)
  };
  lis2dw12_wr_reg_simple(LIS2DW12_CTRL2, &ctrl_reg2);
  sl_sleeptimer_delay_millisecond(LIS2DW12_RESET_DELAY_MS);
  ctrl_reg2.boot = 1;
  ctrl_reg2.soft_reset = 0;
  lis2dw12_wr_reg_simple(LIS2DW12_CTRL2, &ctrl_reg2);
  sl_sleeptimer_delay_millisecond(LIS2DW12_BOOT_DELAY_MS);

#if DEBUG
  // Check device ID
  uint8_t whoami = 0;
  lis2dw12_device_id_get(&lis2dw12_ctx, &whoami);
  SL_ASSERT(whoami == LIS2DW12_ID, "Invalid device ID 0x%02X, expected 0x%02X", whoami, LIS2DW12_ID);
#endif

  //Configure low power mode
  lis2dw12_ctrl1_t ctrl_reg1 = {
    .odr = LIS2DW12_XL_ODR_1Hz6_LP_ONLY,
    .mode = 0, //low power mode (lis2dw12_mode_t enum is the combination of mode and lp_mode...)
    .lp_mode = 0, // low power mode 1
  };
  lis2dw12_wr_reg_simple(LIS2DW12_CTRL1, &ctrl_reg1);
  //Set interrupt mode
#if !SYS_CNF_ACC_INT_AUTO_CLEAR_EN
  lis2dw12_ctrl3_t ctrl_reg3 = { .lir = !SYS_CNF_ACC_INT_AUTO_CLEAR_EN };
  lis2dw12_wr_reg_simple(LIS2DW12_CTRL3, &ctrl_reg3);
#endif
  //Set wake-up thresholds
  // In order to have stationary detection, we need to set both stationary and sleep_on bits to 1
  // Otherwise mode changes would occur during wake/sleep events
  sli_acc_set_thresholds_lis2dw12(SYS_CNF_ACC_WAKEUP_THRESHOLD_UG, SYS_CNF_ACC_WAKEUP_THRESHOLD_UG, SYS_CNF_ACC_WAKEUP_THRESHOLD_UG);
  lis2dw12_wake_up_dur_t wake_up_dur = {
    .stationary = PROPERTY_ENABLE,
    .wake_dur = SYS_CNF_ACC_LIS2DW12_EVT_DURATION_ODR,
  };
  lis2dw12_wr_reg_simple(LIS2DW12_WAKE_UP_DUR, &wake_up_dur);
  SL_STATIC_ASSERT(SYS_CNF_ACC_LIS2DW12_EVT_DURATION_ODR < 4, "Wake-up duration must be less than 4");
  //Configure wake-up interrupt on INT1
  lis2dw12_ctrl4_int1_pad_ctrl_t ctrl_reg4 = {
    .int1_wu = PROPERTY_ENABLE,
  };
  lis2dw12_wr_reg_simple(LIS2DW12_CTRL4_INT1_PAD_CTRL, &ctrl_reg4);
  //Full-scale, bandwidth, low-noise en/dis, low or high pass filter
  lis2dw12_ctrl6_t ctrl_reg6 = {
    .fs = CAT3(LIS2DW12_, SYS_CNF_ACC_LIS2Dx12_RESOLUTION_G, g),
    .bw_filt = CAT2(LIS2DW12_ODR_DIV_, SYS_CNF_ACC_LIS2DW12_BDW_ODR_DIV),
    .fds = SYS_CNF_ACC_LIS2DW12_FILTER_PATH_HIGH,
    .low_noise = SYS_CNF_ACC_LIS2DW12_LOW_NOISE_EN
  };
  lis2dw12_wr_reg_simple(LIS2DW12_CTRL6, &ctrl_reg6);
  //Enable interrupt
  lis2dw12_ctrl_reg7_t ctrl_reg7 = { .interrupts_enable = PROPERTY_ENABLE };
  lis2dw12_wr_reg_simple(LIS2DW12_CTRL_REG7, &ctrl_reg7);

  //Re-enable interrupt
  sli_acc_interrupt_enable_lis2dx12();
}

int sli_acc_get_acceleration_lis2dw12(int32_t *x_ug, int32_t *y_ug, int32_t *z_ug)
{
  int sc = -EINVAL;

  if (x_ug && y_ug && z_ug) {
    int16_t raw_data[3];
    sc = sli_acc_register_read_lis2dx12(lis2dw12_ctx.handle, LIS2DW12_OUT_X_L, (uint8_t *)raw_data, sizeof(raw_data));
    if (!sc) {
      *x_ug = lis2dx12_acc_raw_to_ug(raw_data[0]);
      *y_ug = lis2dx12_acc_raw_to_ug(raw_data[1]);
      *z_ug = lis2dx12_acc_raw_to_ug(raw_data[2]);
    } else {
      sc = -EIO;
    }
  }
  return sc;
}

void sli_acc_interrupt_enable_lis2dw12(void)
{
  sli_acc_interrupt_enable_lis2dx12();
}

void sli_acc_interrupt_disable_lis2dw12(void)
{
  sli_acc_interrupt_disable_lis2dx12();
}

void sli_acc_interrupt_clear_lis2dw12(void)
{
  lis2dw12_all_int_src_t int_src;
  lis2dw12_read_reg(&lis2dw12_ctx, LIS2DW12_ALL_INT_SRC, (uint8_t *)&int_src, 1);
  (void)int_src; // Suppress unused variable warning
}
