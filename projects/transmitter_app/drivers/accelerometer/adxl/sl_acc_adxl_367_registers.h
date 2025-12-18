/***************************************************************************//**
 * @file sl_acc_adxl_367_registers.h
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
#ifndef SL_ACC_ADXL_367_REGISTERS_H
#define SL_ACC_ADXL_367_REGISTERS_H
#ifdef __cplusplus
extern "C" {
#endif

// Macros ----------------------------------------------------------------------
#define ADXL367_CREATE_COMMAND_SCALE_AND_SAMPLE_RATE(scale, sample_rate) \
  ((uint8_t)(((scale & 0x3) << 6) | (1 << 5) | (sample_rate & 0x07)))

// Type definitions ------------------------------------------------------------
typedef enum {
  ADXL367_REG_DEVID_AD = 0x00,
  ADXL367_REG_DEVID_MST = 0x01,
  ADXL367_REG_PARTID = 0x02,
  ADXL367_REG_REVID = 0x03,
  ADXL367_REG_SERIAL_NUMBER_3 = 0x04,
  ADXL367_REG_SERIAL_NUMBER_2 = 0x05,
  ADXL367_REG_SERIAL_NUMBER_1 = 0x06,
  ADXL367_REG_SERIAL_NUMBER_0 = 0x07,
  ADXL367_REG_XDATA = 0x08,
  ADXL367_REG_YDATA = 0x09,
  ADXL367_REG_ZDATA = 0x0A,
  ADXL367_REG_STATUS = 0x0B,
  ADXL367_REG_FIFO_ENTRIES_L = 0x0C,
  ADXL367_REG_FIFO_ENTRIES_H = 0x0D,
  ADXL367_REG_XDATA_H = 0x0E,
  ADXL367_REG_XDATA_L = 0x0F,
  ADXL367_REG_YDATA_H = 0x10,
  ADXL367_REG_YDATA_L = 0x11,
  ADXL367_REG_ZDATA_H = 0x12,
  ADXL367_REG_ZDATA_L = 0x13,
  ADXL367_REG_TEMP_H = 0x14,
  ADXL367_REG_TEMP_L = 0x15,
  ADXL367_REG_EX_ADC_H = 0x16,
  ADXL367_REG_EX_ADC_L = 0x17,
  ADXL367_REG_I2C_FIFO_DATA = 0x18,
  ADXL367_REG_SOFT_RESET = 0x1F,
  ADXL367_REG_THRESH_ACT_H = 0x20,
  ADXL367_REG_THRESH_ACT_L = 0x21,
  ADXL367_REG_TIME_ACT = 0x22,
  ADXL367_REG_THRESH_INACT_H = 0x23,
  ADXL367_REG_THRESH_INACT_L = 0x24,
  ADXL367_REG_TIME_INACT_H = 0x25,
  ADXL367_REG_TIME_INACT_L = 0x26,
  ADXL367_REG_ACT_INACT_CTL = 0x27,
  ADXL367_REG_FIFO_CONTROL = 0x28,
  ADXL367_REG_FIFO_SAMPLES = 0x29,
  ADXL367_REG_INTMAP1_LWR = 0x2A,
  ADXL367_REG_INTMAP2_LWR = 0x2B,
  ADXL367_REG_FILTER_CTL = 0x2C,
  ADXL367_REG_POWER_CTL = 0x2D,
  ADXL367_REG_SELF_TEST = 0x2E,
  ADXL367_REG_TAP_THRESH = 0x2F,
  ADXL367_REG_TAP_DUR = 0x30,
  ADXL367_REG_TAP_LATENT = 0x31,
  ADXL367_REG_TAP_WINDOW = 0x32,
  ADXL367_REG_X_OFFSET = 0x33,
  ADXL367_REG_Y_OFFSET = 0x34,
  ADXL367_REG_Z_OFFSET = 0x35,
  ADXL367_REG_X_SENS = 0x36,
  ADXL367_REG_Y_SENS = 0x37,
  ADXL367_REG_Z_SENS = 0x38,
  ADXL367_REG_TIMER_CTL = 0x39,
  ADXL367_REG_INTMAP1_UPPER = 0x3A,
  ADXL367_REG_INTMAP2_UPPER = 0x3B,
  ADXL367_REG_ADC_CTL = 0x3C,
  ADXL367_REG_TEMP_CTL = 0x3D,
  ADXL367_REG_TEMP_ADC_OV_TH_H = 0x3E,
  ADXL367_REG_TEMP_ADC_OV_TH_L = 0x3F,
  ADXL367_REG_TEMP_ADC_UN_TH_H = 0x40,
  ADXL367_REG_TEMP_ADC_UN_TH_L = 0x41,
  ADXL367_REG_TEMP_ADC_TIMER = 0x42,
  ADXL367_REG_AXIS_MASK = 0x43,
  ADXL367_REG_STATUS_COPY = 0x44,
  ADXL367_REG_STATUS_2 = 0x45,
  ADXL367_REG_STATUS_3 = 0x46,
  ADXL367_REG_PEDOMETER_STEP_CNT_H = 0x47,
  ADXL367_REG_PEDOMETER_STEP_CNT_L = 0x48,
  ADXL367_REG_PEDOMETER_CTL = 0x49,
  ADXL367_REG_PEDOMETER_THRESH_H = 0x4A,
  ADXL367_REG_PEDOMETER_THRESH_L = 0x4B,
  ADXL367_REG_PEDOMETER_SENS_H = 0x4C,
  ADXL367_REG_PEDOMETER_SENS_L = 0x4D
} adxl367_registers_t;

typedef enum {
  ADXL367_RANGE_2G,
  ADXL367_RANGE_4G,
  ADXL367_RANGE_8G,
} adxl367_range_t;

typedef enum {
  ADXL367_SAMPLE_RATE_12_HZ, //Actually 12.5 Hz
  ADXL367_SAMPLE_RATE_25_HZ,
  ADXL367_SAMPLE_RATE_50_HZ,
  ADXL367_SAMPLE_RATE_100_HZ,
  ADXL367_SAMPLE_RATE_200_HZ,
  ADXL367_SAMPLE_RATE_400_HZ,
} adxl367_sample_rate_t;

typedef enum {
  ADXL367_WAKE_UP_RATE_80_MS,
  ADXL367_WAKE_UP_RATE_160_MS,
  ADXL367_WAKE_UP_RATE_320_MS,
  ADXL367_WAKE_UP_RATE_640_MS
} adxl367_wake_up_rate_t;

// Global variables ------------------------------------------------------------
// Function prototypes ---------------------------------------------------------

#ifdef __cplusplus
}
#endif
#endif /* SL_ACC_ADXL_367_REGISTERS_H */
