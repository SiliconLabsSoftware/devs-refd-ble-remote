/***************************************************************************//**
 * @file sl_i2c_drv.c
 *
 * @version 1.0.0
 *******************************************************************************
 * # License
 * <b>Copyright 2023 Silicon Laboratories Inc. www.silabs.com</b>
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
/* Includes ------------------------------------------------------------------*/
#include <errno.h>
#include "assert.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_i2c.h"
#include "sl_sleep.h"
#include "sl_udelay.h"
#include "sl_i2c_drv.h"
#include "sl_system_config.h"

/* Defines -------------------------------------------------------------------*/
// Clock cycles and SCL hold time for bus recovery.
#ifndef SL_I2C_RECOVER_NUM_CLOCKS
// In release mode reset should not happen during transfer
  #if DEBUG
    #define SL_I2C_RECOVER_NUM_CLOCKS 10
  #else
    #define SL_I2C_RECOVER_NUM_CLOCKS 0
  #endif
#endif
#ifndef SL_I2C_HOLD_TIME_US
  #define SL_I2C_HOLD_TIME_US 100
#endif

#define SLI_I2C_GET_IRQN(i2c_number) \
  (I2C0_IRQn + (i2c_number))

#define sli_i2c_get_clock_hi_lo_ratio(frequency)              \
  ((frequency <= I2C_FREQ_STANDARD_MAX) ? i2cClockHLRStandard \
   : (frequency <= I2C_FREQ_FAST_MAX) ? i2cClockHLRAsymetric : i2cClockHLRFast)

/* Type definitions ----------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static const uint16_t sli_i2c_operation_flag_conversion[] = {
  [SL_I2C_OPERATION_WRITE] = I2C_FLAG_WRITE,
  [SL_I2C_OPERATION_READ] = I2C_FLAG_READ,
  [SL_I2C_OPERATION_WRITE_READ] = I2C_FLAG_WRITE_READ,
  [SL_I2C_OPERATION_WRITE_WRITE] = I2C_FLAG_WRITE_WRITE,
};

/* Private function prototypes -----------------------------------------------*/
static int sli_i2c_transfer_init(uint8_t i2c_number, uint8_t device_address, sl_i2c_operation_flag_t operation_flag,
                                 void *buf0, size_t buf0_len, void *buf1, size_t buf1_len);
static inline I2C_TransferReturn_TypeDef sli_i2c_transfer_and_irq_handler(I2C_TypeDef *i2c_peripheral);

/* Function definitions ------------------------------------------------------*/
int sl_i2c_init(uint8_t i2c_number, uint32_t frequency_hz, uint32_t scl_port_pin, uint32_t sda_port_pin)
{
  int sc = -EINVAL;
  GPIO_Port_TypeDef scl_port = SYS_CNF_PORT_PIN_GET_PORT(scl_port_pin);
  uint8_t scl_pin = SYS_CNF_PORT_PIN_GET_PIN(scl_port_pin);
  GPIO_Port_TypeDef sda_port = SYS_CNF_PORT_PIN_GET_PORT(sda_port_pin);
  uint8_t sda_pin = SYS_CNF_PORT_PIN_GET_PIN(sda_port_pin);

  if ((i2c_number < I2C_COUNT)
      && (frequency_hz <= I2C_FREQ_FAST_MAX)
      && GPIO_PORT_PIN_VALID(scl_port, scl_pin)
      && GPIO_PORT_PIN_VALID(sda_port, sda_pin)) {
    sc = 0;
    CMU_Clock_TypeDef i2c_clock = i2c_number == 0 ? cmuClock_I2C0 : cmuClock_I2C1;
    SL_STATIC_ASSERT(I2C_COUNT <= 2, "This driver supports only two I2C instances.");

    CMU_ClockEnable(i2c_clock, true);
    CMU_ClockEnable(cmuClock_GPIO, true);
    GPIO_PinModeSet(scl_port, scl_pin, gpioModeWiredAnd, 1);
    GPIO_PinModeSet(sda_port, sda_pin, gpioModeWiredAnd, 1);

    // In some situations, after a reset during an I2C transfer, the slave device may be
    // left in an unknown state. Send 9 clock pulses to set slave in a defined state.
    for (int clk_recovery = 0; clk_recovery < SL_I2C_RECOVER_NUM_CLOCKS; clk_recovery++) {
      GPIO_PinOutClear(scl_port, scl_pin);
      sl_udelay_wait(SL_I2C_HOLD_TIME_US);
      GPIO_PinOutSet(scl_port, scl_pin);
      sl_udelay_wait(SL_I2C_HOLD_TIME_US);
    }
    GPIO->I2CROUTE[i2c_number].ROUTEEN = GPIO_I2C_ROUTEEN_SDAPEN | GPIO_I2C_ROUTEEN_SCLPEN;
    GPIO->I2CROUTE[i2c_number].SCLROUTE = (uint32_t)(  (scl_pin << _GPIO_I2C_SCLROUTE_PIN_SHIFT)
                                                       | (scl_port << _GPIO_I2C_SCLROUTE_PORT_SHIFT));
    GPIO->I2CROUTE[i2c_number].SDAROUTE = (uint32_t)(  (sda_pin << _GPIO_I2C_SDAROUTE_PIN_SHIFT)
                                                       | (sda_port << _GPIO_I2C_SDAROUTE_PORT_SHIFT));

    // Initialize I2C module in Master Mode.
    I2C_Init_TypeDef i2c_init = {
      .enable = true,
      .master = true,
      .freq = frequency_hz,
      .refFreq = 0,
      .clhr = sli_i2c_get_clock_hi_lo_ratio(frequency_hz),
    };
    I2C_Init(I2C(i2c_number), &i2c_init);
  }
  return sc;
}

int sl_i2c_read(uint8_t i2c_number, uint8_t device_address, void *buf, size_t buf_len)
{
  return sl_i2c_transfer_blocking(i2c_number, device_address, SL_I2C_OPERATION_READ, buf, buf_len, NULL, 0);
}

int sl_i2c_write(uint8_t i2c_number, uint8_t device_address, const void *buf, size_t buf_len)
{
  return sl_i2c_transfer_blocking(i2c_number, device_address, SL_I2C_OPERATION_WRITE, (void *)buf, buf_len, NULL, 0);
}

int sl_i2c_read_register(uint8_t i2c_number, uint8_t device_address, uint8_t register_address,
                         void *buf, size_t buf_len)
{
  return sl_i2c_transfer_blocking(i2c_number, device_address, SL_I2C_OPERATION_WRITE_READ,
                                  &register_address, 1, buf, buf_len);
}

int sl_i2c_write_register(uint8_t i2c_number, uint8_t device_address, uint8_t register_address,
                          const void *buf, size_t buf_len)
{
  return sl_i2c_transfer_blocking(i2c_number, device_address, SL_I2C_OPERATION_WRITE_WRITE,
                                  &register_address, 1, (void *)buf, buf_len);
}

int sl_i2c_transfer_blocking(uint8_t i2c_number, uint8_t device_address, sl_i2c_operation_flag_t operation_flag,
                             void *buf0, size_t buf0_len, void *buf1, size_t buf1_len)
{
  int ret = sli_i2c_transfer_init(i2c_number, device_address, operation_flag, buf0, buf0_len, buf1, buf1_len);
  while (i2cTransferInProgress == ret) {
    ret = I2C_Transfer(I2C(i2c_number));
  }
  return ret;
}

int sl_i2c_transfer_async(uint8_t i2c_number, uint8_t device_address, sl_i2c_operation_flag_t operation_flag,
                          void *buf0, size_t buf0_len, void *buf1, size_t buf1_len)
{
  sl_sleep_deep_enable(false);
  I2C_IntClear(I2C(i2c_number), _I2C_IF_MASK);
  NVIC_ClearPendingIRQ(SLI_I2C_GET_IRQN(i2c_number));
  NVIC_EnableIRQ(SLI_I2C_GET_IRQN(i2c_number));
  int ret = sli_i2c_transfer_init(i2c_number, device_address, operation_flag, buf0, buf0_len, buf1, buf1_len);
  if (ret != i2cTransferInProgress) {
    sl_sleep_deep_enable(true);
    NVIC_DisableIRQ(SLI_I2C_GET_IRQN(i2c_number));
  }
  return ret;
}

static int sli_i2c_transfer_init(uint8_t i2c_number, uint8_t device_address, sl_i2c_operation_flag_t operation_flag,
                                 void *buf0, size_t buf0_len, void *buf1, size_t buf1_len)
{
  static I2C_TransferSeq_TypeDef seq[I2C_COUNT]; //Must be static because of the async transfers

  int ret = -EINVAL;
  if (i2c_number < I2C_COUNT && buf0_len < UINT16_MAX && buf1_len < UINT16_MAX && operation_flag < SL_I2C_OPERATION_MAX) {
    seq[i2c_number].addr = device_address;
    seq[i2c_number].flags = sli_i2c_operation_flag_conversion[operation_flag];
    seq[i2c_number].buf[0].data = buf0;
    seq[i2c_number].buf[0].len = (uint16_t)buf0_len;
    seq[i2c_number].buf[1].data = buf1;
    seq[i2c_number].buf[1].len = (uint16_t)buf1_len;
    ret = I2C_TransferInit(I2C(i2c_number), &seq[i2c_number]);
  }
  return ret;
}

void I2C0_IRQHandler(void)
{
  sli_i2c_transfer_and_irq_handler(I2C0);
}
void I2C1_IRQHandler(void)
{
#if defined(I2C1)
  sli_i2c_transfer_and_irq_handler(I2C1);
#endif
}
void I2C2_IRQHandler(void)
{
#if defined(I2C2)
  sli_i2c_transfer_and_irq_handler(I2C2);
#endif
}
SL_STATIC_ASSERT(I2C_COUNT <= 3, "This driver supports only 3 I2C instances.");

static inline I2C_TransferReturn_TypeDef sli_i2c_transfer_and_irq_handler(I2C_TypeDef *i2c_peripheral)
{
  I2C_TransferReturn_TypeDef ret = I2C_Transfer(i2c_peripheral);
  if (ret != i2cTransferInProgress) {
    uint8_t i2c_number = I2C_NUM(i2c_peripheral);
    sl_i2c_transfer_complete(i2c_number, ret);
    NVIC_DisableIRQ(SLI_I2C_GET_IRQN(i2c_number));
    sl_sleep_deep_enable(true);
  }
  return ret;
}

SL_WEAK void sl_i2c_transfer_complete(uint8_t i2c_number, int result)
{
  (void)i2c_number;
  (void)result;
}
