/***************************************************************************//**
 * @file sl_battery_meas.c
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
#include "sl_battery_meas.h"
#include "assert.h"
#include "em_iadc.h"
#include "em_cmu.h"
#include "sl_common.h"
#include "sl_sleep.h"
#include "sl_sleeptimer.h"
#include "sl_pwr_ctrl.h"
#include "sl_system_config.h"

// Macros ----------------------------------------------------------------------
#define BATT_MEAS_ADC_REF_VOLTAGE_MV  1210
#define BATT_MEAS_ADC_RESOLUTION_BITS 12
//
#define BATT_MEAS_DATA_MULTIPLIER   (1000ULL * HW_CNF_BATTERY_MEAS_CNV_MULT * BATT_MEAS_ADC_REF_VOLTAGE_MV)
#define BATT_MEAS_DATA_DIVIDER      (HW_CNF_BATTERY_MEAS_CNV_DIV * ((1ULL << BATT_MEAS_ADC_RESOLUTION_BITS) - 1ULL))
//
#define BATT_MEAS_ADC_MIN_PERIOD_MS   500UL
SL_STATIC_ASSERT(HW_CNF_BATTERY_MEAS_CNV_DELAY_MS < BATT_MEAS_ADC_MIN_PERIOD_MS, "Battery measurement conversion delay must be less than minimum period!");
SL_STATIC_ASSERT(BATT_MEAS_ADC_MIN_PERIOD_MS < SYS_CNF_BATTERY_MEAS_DEF_PERIOD_MS, "Battery measurement period must be greater than minimum period!");

#define BATT_MEAS_ADC_INIT  {                           \
    .iadcClkSuspend0 = false,                           \
    .iadcClkSuspend1 = false,                           \
    .debugHalt = false,                                 \
    .warmup = iadcWarmupNormal,                         \
    .timebase = 0,                                      \
    .srcClkPrescale = 0,                                \
    .timerCycles = _IADC_TIMER_TIMER_DEFAULT,           \
    .greaterThanEqualThres = _IADC_CMPTHR_ADGT_DEFAULT, \
    .lessThanEqualThres =  _IADC_CMPTHR_ADLT_DEFAULT    \
}
#define BATT_MEAS_ADC_INIT_SINGLE {            \
    .alignment      = iadcAlignRight12,        \
    .showId         = false,                   \
    .dataValidLevel = iadcFifoCfgDvl1,         \
    .fifoDmaWakeup  = false,                   \
    .triggerSelect  = iadcTriggerSelImmediate, \
    .triggerAction  = iadcTriggerActionOnce,   \
    .singleTailgate = false,                   \
    .start          = false                    \
}
#define BATT_MEAS_ADC_INIT_CONFIG_FILL_UTIL(ref_enum, vref_mV) { \
    .adcMode        = iadcCfgModeNormal,                         \
    .osrHighSpeed   = iadcCfgOsrHighSpeed64x,                    \
    .analogGain     = iadcCfgAnalogGain1x,                       \
    .reference      = ref_enum,                                  \
    .twosComplement = iadcCfgTwosCompAuto,                       \
    .adcClkPrescale = 0,                                         \
    .vRef           = vref_mV,                                   \
    .digAvg         = iadcDigitalAverage1                        \
}
#define BATT_MEAS_ADC_INIT_CONFIG_ALL {                                                          \
    {                                                                                            \
      BATT_MEAS_ADC_INIT_CONFIG_FILL_UTIL(iadcCfgReferenceInt1V2, BATT_MEAS_ADC_REF_VOLTAGE_MV), \
      BATT_MEAS_ADC_INIT_CONFIG_FILL_UTIL(iadcCfgReferenceInt1V2, BATT_MEAS_ADC_REF_VOLTAGE_MV)  \
    }                                                                                            \
}

// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
static void battery_meas_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data);
static void battery_start_measurement_with_delay(bool meas_start_trigger);

// Private variables -----------------------------------------------------------
static volatile bool battery_meas_done;
static uint32_t battery_voltage_uv;
static uint32_t battery_meas_period_ms = SYS_CNF_BATTERY_MEAS_DEF_PERIOD_MS;
static sl_sleeptimer_timer_handle_t battery_meas_timer_periodic;
#if HW_CNF_BATTERY_MEAS_CNV_DELAY_MS
static sl_sleeptimer_timer_handle_t battery_meas_timer_delay;
#endif

// Function definitions --------------------------------------------------------

void sl_battery_meas_init(void)
{
  CMU_ClockEnable(cmuClock_IADC0, true);
  CMU_ClockEnable(cmuClock_GPIO, true);
  HW_CNF_BATTERY_MEAS_ADC_BUS_ALLOC();
  sl_pwr_ctrl_init();

  // Initialize IADC
  const IADC_Init_t init = BATT_MEAS_ADC_INIT;
  const IADC_AllConfigs_t initAllConfigs = BATT_MEAS_ADC_INIT_CONFIG_ALL;
  const IADC_InitSingle_t initSingle = BATT_MEAS_ADC_INIT_SINGLE;
  const IADC_SingleInput_t initSingleInput = {
    .negInput = iadcNegInputGnd,
    .posInput = HW_CNF_BATTERY_MEAS_ADC_IN,
    .configId = 0,
    .compare = false
  };
  IADC_init(IADC0, &init, &initAllConfigs);
  IADC_initSingle(IADC0, &initSingle, &initSingleInput);

  // Enable ADC interrupts
  NVIC_ClearPendingIRQ(IADC_IRQn);
  NVIC_EnableIRQ(IADC_IRQn);
  IADC_enableInt(IADC0, IADC_IEN_SINGLEDONE);

  // Start the battery measurement
  sl_status_t sc = sl_sleeptimer_restart_periodic_timer_ms(&battery_meas_timer_periodic, battery_meas_period_ms, battery_meas_timer_callback, NULL, 0xFF, SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
  SL_ASSERT(sc == SL_STATUS_OK, "Failed to start battery measurement timer");
  battery_start_measurement_with_delay(true);
}

void sl_battery_meas_cyclic(void)
{
  if (battery_meas_done) {
    battery_meas_done = false;
    IADC_Result_t result = IADC_pullSingleFifoResult(IADC0);
    battery_voltage_uv = ((BATT_MEAS_DATA_MULTIPLIER * result.data) / BATT_MEAS_DATA_DIVIDER);
    sl_battery_meas_on_complete(battery_voltage_uv);
    sl_sleep_deep_enable(true);
  }
}

uint32_t sl_battery_meas_get_voltage_uv(void)
{
  return battery_voltage_uv;
}

void sl_battery_meas_set_period_time_ms(uint32_t period_ms)
{
  sl_status_t sc;

  if (period_ms == 0) {
    battery_meas_period_ms = 0;
    sc = sl_sleeptimer_stop_timer(&battery_meas_timer_periodic);
  } else {
    battery_meas_period_ms = SL_MAX(BATT_MEAS_ADC_MIN_PERIOD_MS, period_ms);
    sc = sl_sleeptimer_restart_periodic_timer_ms(&battery_meas_timer_periodic, battery_meas_period_ms, battery_meas_timer_callback, NULL, 0xFF, SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
  }
  SL_ASSERT(sc == SL_STATUS_OK, "Failed to configure battery measurement timer: %d", sc);
}

uint32_t sl_battery_meas_get_period_time_ms(void)
{
  return battery_meas_period_ms;
}

static void battery_meas_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void)data;
  battery_start_measurement_with_delay(handle == &battery_meas_timer_periodic);
}

static void battery_start_measurement_with_delay(bool meas_start_trigger)
{
#if HW_CNF_BATTERY_MEAS_CNV_DELAY_MS
  if (meas_start_trigger) {
    sl_pwr_ctrl_enable(true, SL_PWR_CTRL_DOMAIN_BATTERY);
    sl_status_t sc = sl_sleeptimer_start_timer_ms(&battery_meas_timer_delay, HW_CNF_BATTERY_MEAS_CNV_DELAY_MS, battery_meas_timer_callback, NULL, 0xFF, SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
    SL_ASSERT(sc == SL_STATUS_OK, "Failed to start battery measurement delay timer");
  } else {
#else
  (void)meas_start_trigger; {
#endif
    sl_sleep_deep_enable(false);
    IADC_command(IADC0, iadcCmdStartSingle);
  }
}

void IADC_IRQHandler(void)
{
#if HW_CNF_BATTERY_MEAS_CNV_DELAY_MS
  sl_pwr_ctrl_enable(false, SL_PWR_CTRL_DOMAIN_BATTERY);
#endif
  IADC_clearInt(IADC0, _IADC_IF_MASK);
  battery_meas_done = true;
}

SL_WEAK void sl_battery_meas_on_complete(uint32_t microvolt)
{
  (void)microvolt;
}
