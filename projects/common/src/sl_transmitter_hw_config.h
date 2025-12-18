/***************************************************************************//**
 * @file
 * @brief
 * @version 1.0.0
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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
#ifndef SL_TRANSMITTER_HW_CONFIG_H
#define SL_TRANSMITTER_HW_CONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

// Utility macros -------------------------------------------------------------
// This macro is used to set/get a port and pin number into a single 16-bit value.
#define HW_CNF_PORT_PIN_SET(port, pin)        ((port << 8) | pin)
#define HW_CNF_PORT_PIN_GET_PORT(port_pin)    ((GPIO_Port_TypeDef)(port_pin >> 8))
#define HW_CNF_PORT_PIN_GET_PIN(port_pin)     ((uint8_t)(port_pin & 0xFF))
// Hardware revisions --------------------------------------------------------
#define HW_CNF_HW_VERSION_THUNDERBOARD   1
#define HW_CNF_HW_VERSION_REMOTE_V1      2
#define HW_CNF_HW_VERSION_REMOTE_V2      3
#ifndef _HW_CNF_HW_VERSION
  #define _HW_CNF_HW_VERSION              HW_CNF_HW_VERSION_REMOTE_V2
#endif
#if _HW_CNF_HW_VERSION == 0 || _HW_CNF_HW_VERSION > 3
  #error "Invalid hardware version defined in the project configuration!"
#endif

//THIS MACRO MUST BE USED WITH THE ABOVE LIST!
//If this file is not included then the macro check will be always true regardless of the actual board type (0 == 0) (in the preprocessor phase).
//However if a macro like function is used then it will be a compilation time error.
#define HW_CNF_HW_VERSION_IS(board_hw_version)         ((_HW_CNF_HW_VERSION) == (board_hw_version))
#define HW_CNF_HW_VERSION_GREATER_EQ(board_hw_version) ((_HW_CNF_HW_VERSION) >= (board_hw_version))

// Microphone and battery measurement power enable -----------------------------
#if HW_CNF_HW_VERSION_GREATER_EQ(HW_CNF_HW_VERSION_REMOTE_V2)
  #define HW_CNF_SS_PWR_CTRL_IO_PORT              SL_GPIO_PORT_C
  #define HW_CNF_SS_PWR_CTRL_IO_PIN               3
#endif

// Battery measurement --------------------------------------------------------
#if HW_CNF_HW_VERSION_GREATER_EQ(HW_CNF_HW_VERSION_REMOTE_V2)
  #define HW_CNF_BATTERY_MEAS_ADC_IN              iadcPosInputPortAPin8
  #define HW_CNF_BATTERY_MEAS_ADC_BUS_ALLOC()     (GPIO->ABUSALLOC |= GPIO_ABUSALLOC_AEVEN1_ADC0)
  #define HW_CNF_BATTERY_MEAS_CNV_MULT            (22 + 82)
  #define HW_CNF_BATTERY_MEAS_CNV_DIV             (22)
  #define HW_CNF_BATTERY_MEAS_CNV_DELAY_MS        25
#else
  #define HW_CNF_BATTERY_MEAS_ADC_IN              iadcPosInputAvdd
  #define HW_CNF_BATTERY_MEAS_ADC_BUS_ALLOC()
  #define HW_CNF_BATTERY_MEAS_CNV_MULT            (4)
  #define HW_CNF_BATTERY_MEAS_CNV_DIV             (1)
  #define HW_CNF_BATTERY_MEAS_CNV_DELAY_MS        0
#endif

// Key matrix driver ----------------------------------------------------------
#define HW_CNF_KEY_MATRIX_TIMER_TIME_OUT_CHANGE_SETTLE_US    100
#if HW_CNF_HW_VERSION_IS(HW_CNF_HW_VERSION_THUNDERBOARD)
   #define _HW_CNF_KEY_MATRIX_ROWS_RAW_LIST \
  HW_CNF_PORT_PIN_SET(SL_GPIO_PORT_A, 5),   \
  HW_CNF_PORT_PIN_SET(SL_GPIO_PORT_D, 2)
  #define _HW_CNF_KEY_MATRIX_COLS_RAW_LIST \
  HW_CNF_PORT_PIN_SET(SL_GPIO_PORT_A, 7),  \
  HW_CNF_PORT_PIN_SET(SL_GPIO_PORT_D, 3)
#else
  #define _HW_CNF_KEY_MATRIX_ROWS_RAW_LIST \
  HW_CNF_PORT_PIN_SET(SL_GPIO_PORT_C, 7),  \
  HW_CNF_PORT_PIN_SET(SL_GPIO_PORT_C, 5),  \
  HW_CNF_PORT_PIN_SET(SL_GPIO_PORT_C, 0),  \
  HW_CNF_PORT_PIN_SET(SL_GPIO_PORT_D, 2),  \
  HW_CNF_PORT_PIN_SET(SL_GPIO_PORT_B, 3),  \
  HW_CNF_PORT_PIN_SET(SL_GPIO_PORT_B, 1)
  #define _HW_CNF_KEY_MATRIX_COLS_RAW_LIST \
  HW_CNF_PORT_PIN_SET(SL_GPIO_PORT_B, 4),  \
  HW_CNF_PORT_PIN_SET(SL_GPIO_PORT_B, 2),  \
  HW_CNF_PORT_PIN_SET(SL_GPIO_PORT_B, 0),  \
  HW_CNF_PORT_PIN_SET(SL_GPIO_PORT_A, 0),  \
  HW_CNF_PORT_PIN_SET(SL_GPIO_PORT_A, 3),  \
  HW_CNF_PORT_PIN_SET(SL_GPIO_PORT_A, 4),  \
  HW_CNF_PORT_PIN_SET(SL_GPIO_PORT_A, 6),  \
  HW_CNF_PORT_PIN_SET(SL_GPIO_PORT_A, 7)
#endif
#define HW_CNF_KEY_MATRIX_ROWS { _HW_CNF_KEY_MATRIX_ROWS_RAW_LIST }
#define HW_CNF_KEY_MATRIX_COLS { _HW_CNF_KEY_MATRIX_COLS_RAW_LIST }

// Microphone driver ---------------------------------------------------------
#if HW_CNF_HW_VERSION_IS(HW_CNF_HW_VERSION_THUNDERBOARD)
  #define HW_CNF_MIC_IO_ENABLE_PORT                   SL_GPIO_PORT_A
  #define HW_CNF_MIC_IO_ENABLE_PIN                    0
  #define HW_CNF_MIC_IO_PDM_CLK_PORT                  SL_GPIO_PORT_C
  #define HW_CNF_MIC_IO_PDM_CLK_PIN                   6
  #define HW_CNF_MIC_IO_PDM_DATA_PORT                 SL_GPIO_PORT_C
  #define HW_CNF_MIC_IO_PDM_DATA_PIN                  7
#else
  #define HW_CNF_MIC_IO_PDM_CLK_PORT                  SL_GPIO_PORT_C
  #define HW_CNF_MIC_IO_PDM_CLK_PIN                   6
  #define HW_CNF_MIC_IO_PDM_DATA_PORT                 SL_GPIO_PORT_C
  #define HW_CNF_MIC_IO_PDM_DATA_PIN                  4
#endif

// IR LED ---------------------------------------------------------------------
#if HW_CNF_HW_VERSION_IS(HW_CNF_HW_VERSION_THUNDERBOARD)
  #define HW_CNF_IR_TIMER_MODULATOR_NUM               3
  #define HW_CNF_IR_TIMER_CARRIER_NUM                 2
  #define HW_CNF_IR_LED_IO_PORT                       SL_GPIO_PORT_A
  #define HW_CNF_IR_LED_IO_PIN                        6
#else
  #define HW_CNF_IR_TIMER_MODULATOR_NUM               4
  #define HW_CNF_IR_TIMER_CARRIER_NUM                 3
  #define HW_CNF_IR_LED_IO_PORT                       SL_GPIO_PORT_D
  #define HW_CNF_IR_LED_IO_PIN                        3
#endif

// Accelerometer --------------------------------------------------------------
#define HW_CNF_ACC_SENSOR_TYPE_ICM                    1
#define HW_CNF_ACC_SENSOR_TYPE_LIS2DE12               2
#define HW_CNF_ACC_SENSOR_TYPE_LIS2DW12               3
#define HW_CNF_ACC_SENSOR_TYPE_ADXL367                4
#if HW_CNF_HW_VERSION_IS(HW_CNF_HW_VERSION_THUNDERBOARD)
#define HW_CNF_ACC_SENSOR_TYPE                        HW_CNF_ACC_SENSOR_TYPE_ICM
#elif HW_CNF_HW_VERSION_IS(HW_CNF_HW_VERSION_REMOTE_V1)
#define HW_CNF_ACC_SENSOR_TYPE                        HW_CNF_ACC_SENSOR_TYPE_LIS2DW12
#else
#define HW_CNF_ACC_SENSOR_TYPE                        HW_CNF_ACC_SENSOR_TYPE_ADXL367
#endif
// LIS2Dx12 specific configuration --------------------------------------------
#define HW_CNF_ACC_LIS2Dx12_I2C_SDA_PORT              SL_GPIO_PORT_C
#define HW_CNF_ACC_LIS2Dx12_I2C_SDA_PIN               1
#define HW_CNF_ACC_LIS2Dx12_I2C_SCL_PORT              SL_GPIO_PORT_C
#define HW_CNF_ACC_LIS2Dx12_I2C_SCL_PIN               2
#define HW_CNF_ACC_LIS2Dx12_INT_PORT                  SL_GPIO_PORT_A
#define HW_CNF_ACC_LIS2Dx12_INT_PIN                   5
// ADXL367 specific configuration ----------------------------------------------
#define HW_CNF_ACC_ADXL367_I2C_SDA_PORT               HW_CNF_ACC_LIS2Dx12_I2C_SDA_PORT
#define HW_CNF_ACC_ADXL367_I2C_SDA_PIN                HW_CNF_ACC_LIS2Dx12_I2C_SDA_PIN
#define HW_CNF_ACC_ADXL367_I2C_SCL_PORT               HW_CNF_ACC_LIS2Dx12_I2C_SCL_PORT
#define HW_CNF_ACC_ADXL367_I2C_SCL_PIN                HW_CNF_ACC_LIS2Dx12_I2C_SCL_PIN
#define HW_CNF_ACC_ADXL367_INT_PORT                   HW_CNF_ACC_LIS2Dx12_INT_PORT
#define HW_CNF_ACC_ADXL367_INT_PIN                    HW_CNF_ACC_LIS2Dx12_INT_PIN
// ICM-20648 specific configuration -------------------------------------------
#define HW_CNF_ACC_ICM_20648_SPI_MOSI_PORT            SL_GPIO_PORT_C
#define HW_CNF_ACC_ICM_20648_SPI_MOSI_PIN             0
#define HW_CNF_ACC_ICM_20648_SPI_MISO_PORT            SL_GPIO_PORT_C
#define HW_CNF_ACC_ICM_20648_SPI_MISO_PIN             1
#define HW_CNF_ACC_ICM_20648_SPI_CLK_PORT             SL_GPIO_PORT_C
#define HW_CNF_ACC_ICM_20648_SPI_CLK_PIN              2
#define HW_CNF_ACC_ICM_20648_SPI_CS_PORT              SL_GPIO_PORT_B
#define HW_CNF_ACC_ICM_20648_SPI_CS_PIN               2
#define HW_CNF_ACC_ICM_20648_INT_PORT                 SL_GPIO_PORT_B
#define HW_CNF_ACC_ICM_20648_INT_PIN                  3
#define HW_CNF_ACC_ICM_20648_ENABLE_PORT              SL_GPIO_PORT_B
#define HW_CNF_ACC_ICM_20648_ENABLE_PIN               4
// EM4 wakeup pin for accelerometer interrupt ----------------------------------
#if !defined(HW_CNF_ACC_SENSOR_TYPE) /* Sensor disabled */
  #define _HW_CNF_ACC_SENSOR_DEEP_SLEEP_PINS_RAW_LIST
#elif HW_CNF_ACC_SENSOR_TYPE == HW_CNF_ACC_SENSOR_TYPE_ICM && !defined(HW_CNF_ACC_ICM_20648_ENABLE_PORT)
  #define _HW_CNF_ACC_SENSOR_DEEP_SLEEP_PINS_RAW_LIST \
  HW_CNF_PORT_PIN_SET(HW_CNF_ACC_ICM_20648_INT_PORT, HW_CNF_ACC_ICM_20648_INT_PIN)
#elif HW_CNF_ACC_SENSOR_TYPE == HW_CNF_ACC_SENSOR_TYPE_ICM && defined(HW_CNF_ACC_ICM_20648_ENABLE_PORT)
  #define _HW_CNF_ACC_SENSOR_DEEP_SLEEP_PINS_RAW_LIST                               \
  HW_CNF_PORT_PIN_SET(HW_CNF_ACC_ICM_20648_INT_PORT, HW_CNF_ACC_ICM_20648_INT_PIN), \
  HW_CNF_PORT_PIN_SET(HW_CNF_ACC_ICM_20648_ENABLE_PORT, HW_CNF_ACC_ICM_20648_ENABLE_PIN)
#elif HW_CNF_ACC_SENSOR_TYPE == HW_CNF_ACC_SENSOR_TYPE_LIS2DE12 \
  || HW_CNF_ACC_SENSOR_TYPE == HW_CNF_ACC_SENSOR_TYPE_LIS2DW12
  #define _HW_CNF_ACC_SENSOR_DEEP_SLEEP_PINS_RAW_LIST \
  HW_CNF_PORT_PIN_SET(HW_CNF_ACC_LIS2Dx12_INT_PORT, HW_CNF_ACC_LIS2Dx12_INT_PIN)
#elif HW_CNF_ACC_SENSOR_TYPE == HW_CNF_ACC_SENSOR_TYPE_ADXL367
  #define _HW_CNF_ACC_SENSOR_DEEP_SLEEP_PINS_RAW_LIST \
  HW_CNF_PORT_PIN_SET(HW_CNF_ACC_ADXL367_INT_PORT, HW_CNF_ACC_ADXL367_INT_PIN)
#else
  #error "Unknown accelerometer sensor type!"
#endif

// LED activity ----------------------------------------------------------------
#if HW_CNF_HW_VERSION_IS(HW_CNF_HW_VERSION_THUNDERBOARD)
  #define HW_CNF_LED_ACTIVITY_IO_PORT                 1
  #define HW_CNF_LED_ACTIVITY_IO_PIN                  0
#else
  #define HW_CNF_LED_ACTIVITY_IO_PORT                 3
  #define HW_CNF_LED_ACTIVITY_IO_PIN                  0
#endif

// LED backlight ----------------------------------------------------------------
#if HW_CNF_HW_VERSION_IS(HW_CNF_HW_VERSION_THUNDERBOARD)
  #define HW_CNF_LED_BACKLIGHT_IO_PORT                0
  #define HW_CNF_LED_BACKLIGHT_IO_PIN                 3
#else
  #define HW_CNF_LED_BACKLIGHT_IO_PORT                3
  #define HW_CNF_LED_BACKLIGHT_IO_PIN                 1
#endif

// Core clock configuration -----------------------------------------------------
#define HW_CNF_CORE_CLOCK_FSSRCO_FREQ_HZ              20000000UL
#define HW_CNF_CORE_CLOCK_HFRCODPLL_STARTUP_FREQ_HZ   19000000UL
#define HW_CNF_CORE_CLOCK_HFXO_FREQ_HZ                38400000UL
#define HW_CNF_CORE_CLOCK_CLKIN0_FREQ_HZ              0UL

#ifdef __cplusplus
}
#endif
  #endif // SL_TRANSMITTER_HW_CONFIG_H
