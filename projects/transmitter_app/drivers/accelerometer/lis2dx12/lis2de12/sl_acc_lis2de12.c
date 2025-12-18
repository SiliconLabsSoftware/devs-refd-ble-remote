/***************************************************************************//**
 * @file sl_acc_lis2de12.c
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
#include <errno.h>
#include "assert.h"
#include "lis2de12_reg.h"
#include "sl_acc_lis2dx12_common.h"
#include "sl_sleeptimer.h"
#include "sl_system_config.h"

// Macros ----------------------------------------------------------------------
#define LIS2DE12_BOOT_DELAY_MS          5
#define LIS2DE12_REBOOT_DELAY_MS        10
#define LIS2DE12_ACC_THD_LSB_FS_2G_MG   16
#define LIS2DE12_ACC_THD_LSB_FS_4G_MG   32
#define LIS2DE12_ACC_THD_LSB_FS_8G_MG   62
#define LIS2DE12_ACC_THD_LSB_FS_16G_MG  186

#define lis2de12_wr_reg_simple(address, register_ptr) \
  sli_acc_register_write_lis2dx12(lis2de12_ctx.handle, (address), (void *)(register_ptr), 1)
#define lis2de12_acc_mg_threshold_to_raw(acc_thd_mg) \
  (uint8_t)((acc_thd_mg) / CAT3(LIS2DE12_ACC_THD_LSB_FS_, SYS_CNF_ACC_LIS2Dx12_RESOLUTION_G, G_MG))

// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
// Private variables -----------------------------------------------------------
static const stmdev_ctx_t lis2de12_ctx = {
  .write_reg = sli_acc_register_write_lis2dx12,
  .read_reg = sli_acc_register_read_lis2dx12,
  .handle = (void *)LIS2DE12_I2C_ADD_H,
};

// Function definitions --------------------------------------------------------
SL_WEAK void sli_acc_on_threshold_reached_event_lis2de12(void)
{
}

void sli_acc_init_io_lis2de12(void)
{
  sli_acc_init_i2c_lis2dx12();
  sli_acc_init_gpio_lis2dx12(sli_acc_on_threshold_reached_event_lis2de12);
}

void sli_acc_deinit_lis2de12(void)
{
  // Set ODR to 0 thus powering down the device
  lis2de12_ctrl_reg1_t ctrl_reg1 = { .odr = LIS2DE12_POWER_DOWN };
  lis2de12_wr_reg_simple(LIS2DE12_CTRL_REG1, &ctrl_reg1);
}

int sli_acc_set_thresholds_lis2de12(uint32_t x_ug, uint32_t y_ug, uint32_t z_ug)
{
  uint32_t wom_thd_mg = (x_ug + y_ug + z_ug) / 3000;

  lis2de12_int1_ths_t int_ths = { .ths = lis2de12_acc_mg_threshold_to_raw(wom_thd_mg) };
  return lis2de12_wr_reg_simple(LIS2DE12_INT1_THS, &int_ths);
}

void sli_acc_init_device_lis2de12(void)
{
  // !!!
  // Note that the LIS2DE12 APIs are deliberately not used because that would
  // result in multiple read/writes on the same register thus slowing down the init process
  // and draining the battery.
  // !!!
  sli_acc_interrupt_disable_lis2dx12();

  // Reboot
  sl_sleeptimer_delay_millisecond(LIS2DE12_BOOT_DELAY_MS);
  lis2de12_ctrl_reg5_t ctrl_reg5 = { .boot = PROPERTY_ENABLE };
  lis2de12_wr_reg_simple(LIS2DE12_CTRL_REG5, &ctrl_reg5);
  sl_sleeptimer_delay_millisecond(LIS2DE12_REBOOT_DELAY_MS);

#if DEBUG
  // Check device ID
  uint8_t whoami = 0;
  lis2de12_device_id_get(&lis2de12_ctx, &whoami);
  SL_ASSERT(whoami == LIS2DE12_ID, "Invalid device ID 0x%02X, expected 0x%02X", whoami, LIS2DE12_ID);
#endif

  // Configure output data rate
  lis2de12_ctrl_reg1_t ctrl_reg1 = {
    .odr = CAT3(LIS2DE12_ODR_, SYS_CNF_ACC_LIS2DE12_SAMPLE_RATE_HZ, Hz),
    .lpen = PROPERTY_ENABLE,
    .zen = PROPERTY_ENABLE,
    .yen = PROPERTY_ENABLE,
    .xen = PROPERTY_ENABLE
  };
  lis2de12_wr_reg_simple(LIS2DE12_CTRL_REG1, &ctrl_reg1);
  //Configure high pass filter
  lis2de12_ctrl_reg2_t ctrl_reg2 = {
    .hpm = LIS2DE12_NORMAL,
    .hpcf = SYS_CNF_ACC_LIS2DE12_HP_FILTER_MODE, // Filter mode
    .hp = LIS2DE12_ON_INT1_GEN, // Route HP filter output on interrupt generator 1
    .fds = PROPERTY_ENABLE // Route HP filter output on outputs registers
  };
  lis2de12_wr_reg_simple(LIS2DE12_CTRL_REG2, &ctrl_reg2);
  // Enable interrupt on INT1
  lis2de12_ctrl_reg3_t ctrl_reg3 = { .i1_ia1 = PROPERTY_ENABLE };
  lis2de12_wr_reg_simple(LIS2DE12_CTRL_REG3, &ctrl_reg3);
  // Set full scale
  lis2de12_ctrl_reg4_t ctrl_reg4 = { .fs = CAT3(LIS2DE12_, SYS_CNF_ACC_LIS2Dx12_RESOLUTION_G, g) };
  lis2de12_wr_reg_simple(LIS2DE12_CTRL_REG4, &ctrl_reg4);
  // Interrupt mode and FIFO configuration
  lis2de12_ctrl_reg5_t ctrl_reg5_new = {
    .fifo_en = PROPERTY_ENABLE,
    .lir_int1 = !SYS_CNF_ACC_INT_AUTO_CLEAR_EN  //interrupt is latched (0 - NOT latched by default)
  };
  lis2de12_wr_reg_simple(LIS2DE12_CTRL_REG5, &ctrl_reg5_new);
  // Configure FIFO
  lis2de12_fifo_ctrl_reg_t fifo_ctrl = {
    .fm = LIS2DE12_DYNAMIC_STREAM_MODE,
    .tr = LIS2DE12_INT1_GEN,
    .fth = 0
  };
  lis2de12_wr_reg_simple(LIS2DE12_FIFO_CTRL_REG, &fifo_ctrl);
  // Set interrupt threshold
  sli_acc_set_thresholds_lis2de12(SYS_CNF_ACC_WAKEUP_THRESHOLD_UG, SYS_CNF_ACC_WAKEUP_THRESHOLD_UG, SYS_CNF_ACC_WAKEUP_THRESHOLD_UG);
  // Configure interrupt on threshold low / high events
  lis2de12_int1_cfg_t int1_cfg = {
    .aoi = PROPERTY_DISABLE,
    ._6d = PROPERTY_DISABLE,
    .xhie = PROPERTY_ENABLE,
    .yhie = PROPERTY_ENABLE,
    .zhie = PROPERTY_ENABLE,
    .xlie = PROPERTY_DISABLE,
    .ylie = PROPERTY_DISABLE,
    .zlie = PROPERTY_DISABLE
  };
  lis2de12_int1_gen_conf_set(&lis2de12_ctx, &int1_cfg);

  // Set duration to be recognized - 1 bit = 1/ODR
  lis2de12_int1_duration_t int1_duration = { .d =  SYS_CNF_ACC_LIS2DE12_EVT_DURATION_ODR };
  lis2de12_wr_reg_simple(LIS2DE12_INT1_DURATION, &int1_duration);

  //Re-enable interrupt
  sli_acc_interrupt_enable_lis2dx12();
}

int sli_acc_get_acceleration_lis2de12(int32_t *x_ug, int32_t *y_ug, int32_t *z_ug)
{
  int sc = -EINVAL;

  if (x_ug && y_ug && z_ug) {
    int16_t raw_data[3];
    sc = sli_acc_register_read_lis2dx12(lis2de12_ctx.handle, LIS2DE12_FIFO_READ_START, (uint8_t *)raw_data, sizeof(raw_data));
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

void sli_acc_interrupt_enable_lis2de12(void)
{
  sli_acc_interrupt_enable_lis2dx12();
}

void sli_acc_interrupt_disable_lis2de12(void)
{
  sli_acc_interrupt_disable_lis2dx12();
}

void sli_acc_interrupt_clear_lis2de12(void)
{
  lis2de12_int1_src_t int1_src;
  lis2de12_int1_gen_source_get(&lis2de12_ctx, &int1_src);
  (void)int1_src; // Suppress unused variable warning
}
