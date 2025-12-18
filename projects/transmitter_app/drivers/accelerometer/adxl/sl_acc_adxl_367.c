/***************************************************************************//**
 * @file sl_acc_adxl_367.c
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
#include <string.h>
#include "assert.h"
#include "em_cmu.h"
#include "em_i2c.h"
#include "em_gpio.h"
#include "sl_gpio.h"
#include "sl_i2c_drv.h"
#include "sl_sleeptimer.h"
#include "sl_system_config.h"
#include "sl_acc_adxl_367_registers.h"

// Macros ----------------------------------------------------------------------
#define ADXL367_DEVICE_ADDRESS      0x3A //Depending on ASEL pin, VDD:0xA6 or GND:0x3A.
#define ADXL367_ADC_BIT_COUNT       14

#define ADXL367_STS_REG_AWAKE_BIT   6

#define ADXL367_ID_DEVICE_AD        0xAD
#define ADXL367_ID_DEVICE_MST       0x1D
#define ADXL367_ID_PART_ID          0xF7
#define ADXL367_RESET_COMMAND       0x52
#define ADXL367_RESET_DELAY_MS      20

#define ADXL367_ACTIVITY_EN         0x03
#define ADXL367_MEAS_AND_WAKE_EN    0x0A
#define ADXL367_INT1_MASK           0x10

#define adxl367_wr_registers(reg_address, register_ptr, register_ptr_size) \
  sl_i2c_write_register(SYS_CNF_ACC_ADXL367_I2C_NUM, ADXL367_DEVICE_ADDRESS, reg_address, (void *)(register_ptr), register_ptr_size)
#define adxl367_rd_registers(reg_address, register_ptr, register_ptr_size) \
  sl_i2c_read_register(SYS_CNF_ACC_ADXL367_I2C_NUM, ADXL367_DEVICE_ADDRESS, reg_address, (void *)(register_ptr), register_ptr_size)
#define adxl367_wr_reg(reg_address, register_ptr) adxl367_wr_registers(reg_address, register_ptr, 1)
#define adxl367_rd_reg(reg_address, register_ptr) adxl367_rd_registers(reg_address, register_ptr, 1)
//
#define ADXL367_ACC_CNV_MULT (2 * SYS_CNF_ACC_ADXL367_RESOLUTION_G * 1000000LL)
#define ADXL367_ACC_CNV_DIV  (1LL << ADXL367_ADC_BIT_COUNT)
//
#define adxl367_int16_to_raw_high_byte(raw_value) \
  ((uint8_t)(((raw_value) >> 6) & 0xFF))
#define adxl367_int16_to_raw_low_byte(raw_value) \
  ((uint8_t)(((raw_value) << 2) & 0xFC))
#define adxl367_acc_raw_bytes_to_int16(raw_byte_h, raw_byte_l) \
  (((int16_t)((((raw_byte_h) << 8) | ((raw_byte_l))))) / 4)
//
#define adxl367_acc_raw_to_ug(raw_value) \
  ((raw_value) * ADXL367_ACC_CNV_MULT / ADXL367_ACC_CNV_DIV)
#define adxl367_acc_ug_to_raw(ug_value) \
  ((int16_t)(((ug_value) * ADXL367_ACC_CNV_DIV) / ADXL367_ACC_CNV_MULT))

// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
static void adxl367_init_gpio(void);
static void adxl367_init_i2c(void);
static void adxl367_on_gpioint(uint8_t interrupt_number, void *context);
int sli_acc_set_thresholds_adxl367(uint32_t x_ug, uint32_t y_ug, uint32_t z_ug);

// Private variables -----------------------------------------------------------
static int32_t adxl367_int_number = SL_GPIO_INTERRUPT_UNAVAILABLE;

// Function definitions --------------------------------------------------------
void sli_acc_init_io_adxl367(void)
{
  adxl367_init_gpio();
  adxl367_init_i2c();
}

void sli_acc_deinit_adxl367(void)
{
  uint8_t cmd = ADXL367_RESET_COMMAND;
  adxl367_wr_reg(ADXL367_REG_SOFT_RESET, &cmd);
}

void sli_acc_init_device_adxl367(void)
{
  // Reset the device
  sli_acc_deinit_adxl367();
  sl_sleeptimer_delay_millisecond(ADXL367_RESET_DELAY_MS);

#if DEBUG
  // Check device ID
  uint8_t ids[3] = { 0 };
  const uint8_t expected_ids[] = { ADXL367_ID_DEVICE_AD, ADXL367_ID_DEVICE_MST, ADXL367_ID_PART_ID };
  int id_read = adxl367_rd_registers(ADXL367_REG_DEVID_AD, ids, sizeof(ids));
  SL_ASSERT(0 == id_read, "I2C ID read failure %d", id_read);
  SL_ASSERT(memcmp(ids, expected_ids, sizeof(expected_ids)) == 0,
            "Invalid device IDs! Read: 0x%02X 0x%02X 0x%02X, expected: 0x%02X 0x%02X 0x%02X",
            ids[0], ids[1], ids[2], expected_ids[0], expected_ids[1], expected_ids[2]);
#endif

  // Initialization routine
  // Set activity threshold
  sli_acc_set_thresholds_adxl367(SYS_CNF_ACC_WAKEUP_THRESHOLD_UG,
                                 SYS_CNF_ACC_WAKEUP_THRESHOLD_UG,
                                 SYS_CNF_ACC_WAKEUP_THRESHOLD_UG);
  // Set activity timer
  uint8_t command = SYS_CNF_ACC_ADXL367_ACTIVITY_TIME;
  adxl367_wr_reg(ADXL367_REG_TIME_ACT, &command);
  // Enable activity detection
  command = ADXL367_ACTIVITY_EN;
  adxl367_wr_reg(ADXL367_REG_ACT_INACT_CTL, &command);
  // Set full scale resolution and sample rate
  uint8_t scale = CAT3(ADXL367_RANGE_, SYS_CNF_ACC_ADXL367_RESOLUTION_G, G);
  uint8_t sample_rate = CAT3(ADXL367_SAMPLE_RATE_, SYS_CNF_ACC_ADXL367_SAMPLE_RATE_HZ, _HZ);
  command = ADXL367_CREATE_COMMAND_SCALE_AND_SAMPLE_RATE(scale, sample_rate);
  adxl367_wr_reg(ADXL367_REG_FILTER_CTL, &command);
  // Set wake-up sample rate
  uint8_t wake_rate = CAT3(ADXL367_WAKE_UP_RATE_, SYS_CNF_ACC_ADXL367_WAKE_UP_RATE_MS, _MS) & 0x03;
  command = (uint8_t)(wake_rate << 6);
  adxl367_wr_reg(ADXL367_REG_TIMER_CTL, &command);
  // Enter measurement and wake_up mode
  command = ADXL367_MEAS_AND_WAKE_EN;
  adxl367_wr_reg(ADXL367_REG_POWER_CTL, &command);
  // Map interrupts to INT1 pin
  command = ADXL367_INT1_MASK;
  adxl367_wr_reg(ADXL367_REG_INTMAP1_LWR, &command);
}

int sli_acc_get_acceleration_adxl367(int32_t *x_ug, int32_t *y_ug, int32_t *z_ug)
{
  int sc = -EINVAL;

  if (x_ug && y_ug && z_ug) {
    uint8_t raw_data[6];
    sc = adxl367_rd_registers(ADXL367_REG_XDATA_H, raw_data, sizeof(raw_data));
    if (!sc) {
      *x_ug = adxl367_acc_raw_to_ug(adxl367_acc_raw_bytes_to_int16(raw_data[0], raw_data[1]));
      *y_ug = adxl367_acc_raw_to_ug(adxl367_acc_raw_bytes_to_int16(raw_data[2], raw_data[3]));
      *z_ug = adxl367_acc_raw_to_ug(adxl367_acc_raw_bytes_to_int16(raw_data[4], raw_data[5]));
    } else {
      sc = -EIO;
    }
  }
  return sc;
}

int sli_acc_set_thresholds_adxl367(uint32_t x_ug, uint32_t y_ug, uint32_t z_ug)
{
  uint32_t thd_ug_avg = (x_ug + y_ug + z_ug) / 3;
  int16_t thd_raw_activity = adxl367_acc_ug_to_raw(thd_ug_avg);
  uint8_t thd_raw_activity_bytes[2] = { adxl367_int16_to_raw_high_byte(thd_raw_activity), adxl367_int16_to_raw_low_byte(thd_raw_activity) };

  return adxl367_wr_registers(ADXL367_REG_THRESH_ACT_H, &thd_raw_activity_bytes, sizeof(thd_raw_activity_bytes));
}

void sli_acc_interrupt_enable_adxl367(void)
{
  GPIO_IntClear(1 << adxl367_int_number);
  GPIO_IntEnable(1 << adxl367_int_number);
}

void sli_acc_interrupt_disable_adxl367(void)
{
  GPIO_IntDisable(1 << adxl367_int_number);
}

void sli_acc_interrupt_clear_adxl367(void)
{
  static uint8_t sts;
  static const uint8_t register_address = ADXL367_REG_STATUS;
  sl_i2c_transfer_async(SYS_CNF_ACC_ADXL367_I2C_NUM, ADXL367_DEVICE_ADDRESS, SL_I2C_OPERATION_WRITE_READ,
                        (void *)&register_address, sizeof(register_address), &sts, sizeof(sts));
}

SL_WEAK void sli_acc_on_threshold_reached_event_adxl367(void)
{
}

static void adxl367_init_i2c(void)
{
  int sc = sl_i2c_init(SYS_CNF_ACC_ADXL367_I2C_NUM,
                       SYS_CNF_ACC_ADXL367_I2C_FREQ,
                       HW_CNF_PORT_PIN_SET(HW_CNF_ACC_ADXL367_I2C_SCL_PORT, HW_CNF_ACC_ADXL367_I2C_SCL_PIN),
                       HW_CNF_PORT_PIN_SET(HW_CNF_ACC_ADXL367_I2C_SDA_PORT, HW_CNF_ACC_ADXL367_I2C_SDA_PIN));
  SL_ASSERT(sc == 0, "I2C init failure %d!", sc);
}

static void adxl367_init_gpio(void)
{
  const sl_gpio_t gpio = {
    .port = HW_CNF_ACC_ADXL367_INT_PORT,
    .pin = HW_CNF_ACC_ADXL367_INT_PIN
  };
  sl_gpio_configure_wakeup_em4_interrupt(&gpio, &adxl367_int_number, true, adxl367_on_gpioint, NULL); //default interrupt polarity is active high
  GPIO_PinModeSet(HW_CNF_ACC_ADXL367_INT_PORT, HW_CNF_ACC_ADXL367_INT_PIN, gpioModeInput, 0); //The API above set pull-up but it is not needed
  SL_ASSERT(adxl367_int_number != SL_GPIO_INTERRUPT_UNAVAILABLE, "Failed to configure ADXL367 interrupt pin!");
  adxl367_int_number += _GPIO_EM4WUEN_EM4WUEN_SHIFT;
}

static void adxl367_on_gpioint(uint8_t interrupt_number, void *context)
{
  (void)interrupt_number;
  (void)context;
  sli_acc_on_threshold_reached_event_adxl367();
}
