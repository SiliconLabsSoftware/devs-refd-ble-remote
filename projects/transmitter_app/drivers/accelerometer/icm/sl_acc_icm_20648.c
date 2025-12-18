/***************************************************************************//**
 * @file sl_acc_icm_20648.c
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
#include <stdint.h>
#include "assert.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_usart.h"
#include "sl_gpio.h"
#include "sl_common.h"
#include "sl_sleeptimer.h"
#include "sl_system_config.h"
#include "sl_power_manager_config.h"
#include "sl_acc_icm_20648_registers.h"

// Macros ----------------------------------------------------------------------
#define ICM20648_USART                CAT2(USART, SYS_CNF_ACC_ICM_20648_USART_NUM)
#define ICM20648_DELAY_RESET_MS       100
#define ICM20648_DELAY_PLL_MS         30

// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
int sli_acc_set_thresholds_icm_20648(uint32_t x_ug, uint32_t y_ug, uint32_t z_ug);
static void icm20648_init_gpio(void);
static void icm20648_init_spi(void);
static void icm20648_init_device(void);
static void icm20648_on_gpioint(uint8_t interrupt_number, void *context);
static void icm20648_register_read(uint16_t addr, uint8_t *data, size_t size);
static void icm20648_register_write(uint16_t addr, uint8_t data);
static inline void icm20648_register_bank_select(uint8_t bank);

// Private variables -----------------------------------------------------------
static int32_t icm20648_int_number = SL_GPIO_INTERRUPT_UNAVAILABLE;

// Function definitions --------------------------------------------------------
void sli_acc_init_io_icm_20648(void)
{
  icm20648_init_gpio();
  icm20648_init_spi();
}

void sli_acc_init_device_icm_20648(void)
{
  icm20648_init_device();
}

void sli_acc_deinit_icm_20648(void)
{
#if defined(SYS_CNF_ACC_ICM_20648_ENABLE_PORT) && defined(SYS_CNF_ACC_ICM_20648_ENABLE_PIN)
  GPIO_PinOutClear(SYS_CNF_ACC_ICM_20648_ENABLE_PORT, SYS_CNF_ACC_ICM_20648_ENABLE_PIN);
#else
  icm20648_register_write(ICM20648_REG_PWR_MGMT_1, ICM20648_BIT_H_RESET);
#endif
}

int sli_acc_get_acceleration_icm_20648(int32_t *x_ug, int32_t *y_ug, int32_t *z_ug)
{
  int sc = -EINVAL;
  uint8_t raw_data[6] = { 0 };
  const uint64_t resolution = SYS_CNF_ACC_ICM_20648_RESOLUTION_G * 1000000; // Convert G to microG

  if (x_ug && y_ug && z_ug) {
    sc = 0;
    icm20648_register_read(ICM20648_REG_ACCEL_XOUT_H_SH, raw_data, sizeof(raw_data));

    /* Convert the MSB and LSB into a signed 16-bit value and multiply by the resolution to get the G value */
    int16_t temp = ( (int16_t) raw_data[0] << 8) | raw_data[1];
    *x_ug = (temp * resolution) / INT16_MAX;
    temp = ( (int16_t) raw_data[2] << 8) | raw_data[3];
    *y_ug = (temp * resolution) / INT16_MAX;
    temp = ( (int16_t) raw_data[4] << 8) | raw_data[5];
    *z_ug = (temp * resolution) / INT16_MAX;
  }
  return sc;
}

int sli_acc_set_thresholds_icm_20648(uint32_t x_ug, uint32_t y_ug, uint32_t z_ug)
{
  uint32_t wom_thd_mg = (x_ug + y_ug + z_ug) / 3000;
  icm20648_register_write(ICM20648_REG_ACCEL_WOM_THR, wom_thd_mg / 4);
  return 0;
}

void sli_acc_interrupt_enable_icm_20648(void)
{
  GPIO_IntClear(1 << icm20648_int_number);
  GPIO_IntEnable(1 << icm20648_int_number);
}

void sli_acc_interrupt_disable_icm_20648(void)
{
  GPIO_IntDisable(1 << icm20648_int_number);
}

void sli_acc_interrupt_clear_icm_20648(void)
{
  uint32_t int_status = 0;
  icm20648_register_read(ICM20648_REG_INT_STATUS, (void *)&int_status, sizeof(int_status));
}

SL_WEAK void sli_acc_on_threshold_reached_event_icm_20648(void)
{
}

static void icm20648_on_gpioint(uint8_t interrupt_number, void *context)
{
  (void)interrupt_number;
  (void)context;
  sli_acc_on_threshold_reached_event_icm_20648();
}

static void icm20648_init_gpio(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);

#if defined(SYS_CNF_ACC_ICM_20648_ENABLE_PORT) && defined(SYS_CNF_ACC_ICM_20648_ENABLE_PIN)
  GPIO_PinModeSet(SYS_CNF_ACC_ICM_20648_ENABLE_PORT, SYS_CNF_ACC_ICM_20648_ENABLE_PIN, gpioModePushPull, 1);
#endif
  const sl_gpio_t gpio = {
    .port = SYS_CNF_ACC_ICM_20648_INT_PORT,
    .pin = SYS_CNF_ACC_ICM_20648_INT_PIN
  };
  sl_gpio_configure_wakeup_em4_interrupt(&gpio, &icm20648_int_number, false, icm20648_on_gpioint, NULL);
  SL_ASSERT(icm20648_int_number != SL_GPIO_INTERRUPT_UNAVAILABLE, "Failed to configure ICM-20648 interrupt pin!");
  icm20648_int_number += _GPIO_EM4WUEN_EM4WUEN_SHIFT;
}

static void icm20648_init_spi(void)
{
  CMU_ClockEnable(CAT2(cmuClock_USART, SYS_CNF_ACC_ICM_20648_USART_NUM), true);

  //SPI config
  USART_InitSync_TypeDef init = USART_INITSYNC_DEFAULT;
  init.msbf = true;
  init.baudrate = SYS_CNF_ACC_ICM_20648_SPI_BAUDRATE;
  USART_Reset(ICM20648_USART);
  USART_InitSync(ICM20648_USART, &init);

  //IO configuration
  GPIO_PinModeSet(SYS_CNF_ACC_ICM_20648_SPI_MOSI_PORT, SYS_CNF_ACC_ICM_20648_SPI_MOSI_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(SYS_CNF_ACC_ICM_20648_SPI_MISO_PORT, SYS_CNF_ACC_ICM_20648_SPI_MISO_PIN, gpioModeInput, 0);
  GPIO_PinModeSet(SYS_CNF_ACC_ICM_20648_SPI_CLK_PORT, SYS_CNF_ACC_ICM_20648_SPI_CLK_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(SYS_CNF_ACC_ICM_20648_SPI_CS_PORT, SYS_CNF_ACC_ICM_20648_SPI_CS_PIN, gpioModePushPull, 1);
  GPIO->USARTROUTE[SYS_CNF_ACC_ICM_20648_USART_NUM].ROUTEEN = GPIO_USART_ROUTEEN_RXPEN | GPIO_USART_ROUTEEN_TXPEN | GPIO_USART_ROUTEEN_CLKPEN;
  GPIO->USARTROUTE[SYS_CNF_ACC_ICM_20648_USART_NUM].TXROUTE =
    ((SYS_CNF_ACC_ICM_20648_SPI_MOSI_PORT << _GPIO_USART_TXROUTE_PORT_SHIFT)
     | (SYS_CNF_ACC_ICM_20648_SPI_MOSI_PIN << _GPIO_USART_TXROUTE_PIN_SHIFT));
  GPIO->USARTROUTE[SYS_CNF_ACC_ICM_20648_USART_NUM].RXROUTE =
    ((SYS_CNF_ACC_ICM_20648_SPI_MISO_PORT << _GPIO_USART_RXROUTE_PORT_SHIFT)
     | (SYS_CNF_ACC_ICM_20648_SPI_MISO_PIN << _GPIO_USART_RXROUTE_PIN_SHIFT));
  GPIO->USARTROUTE[SYS_CNF_ACC_ICM_20648_USART_NUM].CLKROUTE =
    ((SYS_CNF_ACC_ICM_20648_SPI_CLK_PORT << _GPIO_USART_CLKROUTE_PORT_SHIFT)
     | (SYS_CNF_ACC_ICM_20648_SPI_CLK_PIN << _GPIO_USART_CLKROUTE_PIN_SHIFT));
}

static void icm20648_init_device(void)
{
  // Reset
  icm20648_register_write(ICM20648_REG_PWR_MGMT_1, ICM20648_BIT_H_RESET);
  sl_sleeptimer_delay_millisecond(ICM20648_DELAY_RESET_MS);

#if DEBUG
  //Check the device ID
  uint8_t who_am_i = 0;
  icm20648_register_read(ICM20648_REG_WHO_AM_I, &who_am_i, sizeof(who_am_i));
  SL_ASSERT(who_am_i == ICM20648_DEVICE_ID, "Invalid device ID 0x%02X, expected 0x%02X", who_am_i, ICM20648_DEVICE_ID);
#endif

  // Configure the accelerometer
  icm20648_register_write(ICM20648_REG_USER_CTRL, ICM20648_BIT_I2C_IF_DIS); //Disable I2C interface
  icm20648_register_write(ICM20648_REG_PWR_MGMT_1, ICM20648_BIT_CLK_PLL | ICM20648_BIT_TEMP_DIS | ICM20648_BIT_LP_EN); //Auto selects the best available clock source
  sl_sleeptimer_delay_millisecond(ICM20648_DELAY_PLL_MS); //PLL startup time
  icm20648_register_write(ICM20648_REG_PWR_MGMT_2, ICM20648_BIT_PWR_GYRO_STBY); //Disable gyroscope
  icm20648_register_write(ICM20648_REG_INT_PIN_CFG, ICM20648_BIT_INT_ACTL | ICM20648_BIT_INT_OPEN
#if !SYS_CNF_ACC_INT_AUTO_CLEAR_EN // If auto clear is not enabled, use latch mode which will keep the interrupt active until status is read
                          | ICM20648_BIT_INT_LATCH_EN
#endif //Otherwise the interrupt will be cleared automatically after 50 us (nothing to do in this driver)
                          );
  // Set the accelerometer sample rate
  int32_t div = (1125UL / SYS_CNF_ACC_ICM_20648_SAMPLE_RATE_HZ) - 1;
  div = SL_MIN(SL_MAX(div, 0), 4095);
  icm20648_register_write(ICM20648_REG_ACCEL_SMPLRT_DIV_1, (uint8_t)(div >> 8));
  icm20648_register_write(ICM20648_REG_ACCEL_SMPLRT_DIV_2, (uint8_t)(div & 0xFF));
  // Set the accelerometer bandwidth and full scale
  uint8_t acc_cnf =   CAT3(ICM20648_ACCEL_BW_, SYS_CNF_ACC_ICM_20648_BANDWIDTH_HZ, HZ)
                    | CAT3(ICM20648_ACCEL_FULLSCALE_, SYS_CNF_ACC_ICM_20648_RESOLUTION_G, G);
  icm20648_register_write(ICM20648_REG_ACCEL_CONFIG, acc_cnf);
  //Enable Wake-up On Motion interrupt
  icm20648_register_write(ICM20648_REG_ACCEL_INTEL_CTRL, ICM20648_BIT_ACCEL_INTEL_EN | ICM20648_BIT_ACCEL_INTEL_MODE);
  sli_acc_set_thresholds_icm_20648(SYS_CNF_ACC_WAKEUP_THRESHOLD_UG, SYS_CNF_ACC_WAKEUP_THRESHOLD_UG, SYS_CNF_ACC_WAKEUP_THRESHOLD_UG);
  icm20648_register_write(ICM20648_REG_INT_ENABLE, ICM20648_BIT_WOM_INT_EN);
}

static void icm20648_register_read(uint16_t addr, uint8_t *data, size_t size)
{
  uint8_t regAddr = addr & 0x7F;
  uint8_t bank = addr >> 7;

  icm20648_register_bank_select(bank);
  GPIO_PinOutClear(SYS_CNF_ACC_ICM_20648_SPI_CS_PORT, SYS_CNF_ACC_ICM_20648_SPI_CS_PIN);
  USART_Tx(ICM20648_USART, (regAddr | 0x80) );
  USART_Rx(ICM20648_USART);
  while ( size-- ) {
    USART_Tx(ICM20648_USART, 0x00);
    *data++ = USART_Rx(ICM20648_USART);
  }
  GPIO_PinOutSet(SYS_CNF_ACC_ICM_20648_SPI_CS_PORT, SYS_CNF_ACC_ICM_20648_SPI_CS_PIN);
}

static void icm20648_register_write(uint16_t addr, uint8_t data)
{
  uint8_t regAddr = addr & 0x7F;
  uint8_t bank = addr >> 7;

  icm20648_register_bank_select(bank);
  GPIO_PinOutClear(SYS_CNF_ACC_ICM_20648_SPI_CS_PORT, SYS_CNF_ACC_ICM_20648_SPI_CS_PIN);
  USART_Tx(ICM20648_USART, (regAddr & 0x7F) );
  USART_Tx(ICM20648_USART, data);
  USART_RxDouble(ICM20648_USART); //dummy read to clear RX buffer
  GPIO_PinOutSet(SYS_CNF_ACC_ICM_20648_SPI_CS_PORT, SYS_CNF_ACC_ICM_20648_SPI_CS_PIN);
}

static inline void icm20648_register_bank_select(uint8_t bank)
{
  static uint8_t last_bank = 0xFF;

  if (last_bank != bank) {
    last_bank = bank;
    GPIO_PinOutClear(SYS_CNF_ACC_ICM_20648_SPI_CS_PORT, SYS_CNF_ACC_ICM_20648_SPI_CS_PIN);
    USART_Tx(ICM20648_USART, ICM20648_REG_BANK_SEL);
    USART_Tx(ICM20648_USART, bank << 4);
    USART_RxDouble(ICM20648_USART); //dummy read to clear RX buffer
    GPIO_PinOutSet(SYS_CNF_ACC_ICM_20648_SPI_CS_PORT, SYS_CNF_ACC_ICM_20648_SPI_CS_PIN);
  }
}
