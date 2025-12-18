/***************************************************************************//**
 * @file sl_acc.c
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
#include "sl_acc.h"
#include "sl_common.h"
#include "sl_sleeptimer.h"
#include "sl_system_config.h"

// Macros ----------------------------------------------------------------------
#if !defined(HW_CNF_ACC_SENSOR_TYPE)
  #if defined(__GNUC__)
    #pragma message("Accelerometer sensor is not enabled. Please define HW_CNF_ACC_SENSOR_TYPE.")
  #endif
#elif HW_CNF_ACC_SENSOR_TYPE == HW_CNF_ACC_SENSOR_TYPE_ICM
  #define ACC_SENSOR_TYPE icm_20648
#elif HW_CNF_ACC_SENSOR_TYPE == HW_CNF_ACC_SENSOR_TYPE_LIS2DE12
  #define ACC_SENSOR_TYPE lis2de12
#elif HW_CNF_ACC_SENSOR_TYPE == HW_CNF_ACC_SENSOR_TYPE_LIS2DW12
  #define ACC_SENSOR_TYPE lis2dw12
#elif HW_CNF_ACC_SENSOR_TYPE == HW_CNF_ACC_SENSOR_TYPE_ADXL367
  #define ACC_SENSOR_TYPE adxl367
#else
  #error "Unsupported accelerometer sensor type defined in sl_system_config.h"
#endif

#if defined(HW_CNF_ACC_SENSOR_TYPE)
  #define sli_acc_init_io_impl                    CAT2(sli_acc_init_io_, ACC_SENSOR_TYPE)
  #define sli_acc_init_device_impl                CAT2(sli_acc_init_device_, ACC_SENSOR_TYPE)
  #define sli_acc_deinit_impl                     CAT2(sli_acc_deinit_, ACC_SENSOR_TYPE)
  #define sli_acc_get_acceleration_impl           CAT2(sli_acc_get_acceleration_, ACC_SENSOR_TYPE)
  #define sli_acc_set_thresholds_impl             CAT2(sli_acc_set_thresholds_, ACC_SENSOR_TYPE)
  #define sli_acc_on_threshold_reached_event_impl CAT2(sli_acc_on_threshold_reached_event_, ACC_SENSOR_TYPE)
  #define sli_acc_interrupt_enable_impl           CAT2(sli_acc_interrupt_enable_, ACC_SENSOR_TYPE)
  #define sli_acc_interrupt_disable_impl          CAT2(sli_acc_interrupt_disable_, ACC_SENSOR_TYPE)
  #define sli_acc_interrupt_clear_impl            CAT2(sli_acc_interrupt_clear_, ACC_SENSOR_TYPE)
#else
  #define sli_acc_init_io_impl()
  #define sli_acc_init_device_impl()
  #define sli_acc_deinit_impl()
  #define sli_acc_get_acceleration_impl(x, y, z) ((x) || (y) || (z) ? -1 : -1)
  #define sli_acc_set_thresholds_impl(x, y, z) ((x) || (y) || (z) ? -1 : -1)
  #define sli_acc_on_threshold_reached_event_impl  sli_acc_dummy_unimplemented_callback
  #define sli_acc_interrupt_enable_impl()
  #define sli_acc_interrupt_disable_impl()
  #define sli_acc_interrupt_clear_impl()
#endif

// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
#if defined(HW_CNF_ACC_SENSOR_TYPE)
void sli_acc_init_io_impl(void);
void sli_acc_init_device_impl(void);
void sli_acc_deinit_impl(void);
int sli_acc_get_acceleration_impl(int32_t *x_ug, int32_t *y_ug, int32_t *z_ug);
int sli_acc_set_thresholds_impl(uint32_t x_ug, uint32_t y_ug, uint32_t z_ug);
void sli_acc_interrupt_enable_impl(void);
void sli_acc_interrupt_disable_impl(void);
void sli_acc_interrupt_clear_impl(void);
#endif

// Private variables -----------------------------------------------------------
// Function definitions --------------------------------------------------------

void sl_acc_init_io(void)
{
  sli_acc_init_io_impl();
}

void sl_acc_init_device(void)
{
  sli_acc_init_device_impl();
}

void sl_acc_deinit(void)
{
  sli_acc_deinit_impl();
}

int sl_acc_get_acceleration(int32_t *x_ug, int32_t *y_ug, int32_t *z_ug)
{
  return sli_acc_get_acceleration_impl(x_ug, y_ug, z_ug);
}

int sl_acc_set_thresholds(uint32_t x_ug, uint32_t y_ug, uint32_t z_ug)
{
  return sli_acc_set_thresholds_impl(x_ug, y_ug, z_ug);
}

void sl_acc_interrupt_enable(void)
{
  sli_acc_interrupt_enable_impl();
}

void sl_acc_interrupt_disable(void)
{
  sli_acc_interrupt_disable_impl();
}

void sl_acc_interrupt_clear(void)
{
  sli_acc_interrupt_clear_impl();
}

bool sl_acc_interrupt_clear_lazy(uint16_t timeout_ms)
{
  static uint32_t last_clear_tick;
  bool cleared = false;
  uint32_t current_tick = sl_sleeptimer_get_tick_count();

  if ((current_tick - last_clear_tick) >= sl_sleeptimer_ms_to_tick(timeout_ms)) {
    cleared = true;
    sli_acc_interrupt_clear_impl();
    last_clear_tick = current_tick;
  }
  return cleared;
}

void sli_acc_on_threshold_reached_event_impl(void)
{
  sl_acc_on_threshold_reached_event();
}

SL_WEAK void sl_acc_on_threshold_reached_event(void)
{
  // User overridable callback for threshold event
}
