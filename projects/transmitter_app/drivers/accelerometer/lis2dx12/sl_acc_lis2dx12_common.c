/***************************************************************************//**
 * @file sl_acc_lis2dx12_common.c
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
#include "em_cmu.h"
#include "em_i2c.h"
#include "em_gpio.h"
#include "sl_gpio.h"
#include "sl_i2c_drv.h"
#include "sl_acc_lis2dx12_common.h"
#include "sl_system_config.h"

// Macros ----------------------------------------------------------------------
// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
static void lis2dx12_on_gpioint(uint8_t interrupt_number, void *context);

// Private variables -----------------------------------------------------------
static void (*lis2dx12_irq_callback)(void);
static int32_t lis2dx12_int_number = SL_GPIO_INTERRUPT_UNAVAILABLE;

// Function definitions --------------------------------------------------------
void sli_acc_init_i2c_lis2dx12(void)
{
  int sc = sl_i2c_init(SYS_CNF_ACC_LIS2Dx12_I2C_NUM,
                       SYS_CNF_ACC_LIS2Dx12_I2C_FREQ,
                       HW_CNF_PORT_PIN_SET(SYS_CNF_ACC_LIS2Dx12_I2C_SCL_PORT, SYS_CNF_ACC_LIS2Dx12_I2C_SCL_PIN),
                       HW_CNF_PORT_PIN_SET(SYS_CNF_ACC_LIS2Dx12_I2C_SDA_PORT, SYS_CNF_ACC_LIS2Dx12_I2C_SDA_PIN));
  SL_ASSERT(sc == 0, "I2C init failure %d!", sc);
}

void sli_acc_init_gpio_lis2dx12(void (*irq_callback)(void))
{
  CMU_ClockEnable(cmuClock_GPIO, true);

  const sl_gpio_t gpio = {
    .port = SYS_CNF_ACC_LIS2Dx12_INT_PORT,
    .pin = SYS_CNF_ACC_LIS2Dx12_INT_PIN
  };
  lis2dx12_irq_callback = irq_callback;
  sl_gpio_configure_wakeup_em4_interrupt(&gpio, &lis2dx12_int_number, true, lis2dx12_on_gpioint, NULL); //default interrupt polarity is active high
  GPIO_PinModeSet(SYS_CNF_ACC_LIS2Dx12_INT_PORT, SYS_CNF_ACC_LIS2Dx12_INT_PIN, gpioModeInput, 0); //The API above set pull-up but it is not needed
  SL_ASSERT(lis2dx12_int_number != SL_GPIO_INTERRUPT_UNAVAILABLE, "Failed to configure LIS2Dx12 interrupt pin!");
  lis2dx12_int_number += _GPIO_EM4WUEN_EM4WUEN_SHIFT;
}

void sli_acc_interrupt_enable_lis2dx12(void)
{
  GPIO_IntClear(1 << lis2dx12_int_number);
  GPIO_IntEnable(1 << lis2dx12_int_number);
}

void sli_acc_interrupt_disable_lis2dx12(void)
{
  GPIO_IntDisable(1 << lis2dx12_int_number);
}

static void lis2dx12_on_gpioint(uint8_t interrupt_number, void *context)
{
  (void)interrupt_number;
  (void)context;
  SL_ASSERT(lis2dx12_irq_callback != NULL, "LIS2Dx12 IRQ callback is not set!");
  lis2dx12_irq_callback();
}

int32_t sli_acc_register_write_lis2dx12(void *handle_as_address, uint8_t reg, const uint8_t *data, uint16_t len)
{
  return sl_i2c_write_register(SYS_CNF_ACC_LIS2Dx12_I2C_NUM, (uint32_t)handle_as_address, reg, data, len);
}

int32_t sli_acc_register_read_lis2dx12(void *handle_as_address, uint8_t reg, uint8_t *data, uint16_t len)
{
  return sl_i2c_read_register(SYS_CNF_ACC_LIS2Dx12_I2C_NUM, (uint32_t)handle_as_address, reg, data, len);
}
