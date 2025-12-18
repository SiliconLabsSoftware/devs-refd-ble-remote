/***************************************************************************//**
 * @file sl_ir_nec_decoder.c
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
#define SL_LOG_MODULE_NAME "NecDecoder"
#include <stddef.h>
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_timer.h"
#include "assert.h"
#include "sl_system_config.h"
#include "sl_ir_nec_decoder.h"
#include "sl_common.h"
#include "sl_log.h"
#include "sl_activity_led.h"

// Macros ----------------------------------------------------------------------
#ifndef CAT3
  #define _CAT3(a, b, c) a##b##c
  #define CAT3(a, b, c) _CAT3(a, b, c)
#endif
#ifndef CAT2
  #define CAT2(a, b) CAT3(a, b, )
#endif

#define NEC_LEADING_PULSE_MIN_US 8500
#define NEC_LEADING_PULSE_MAX_US 9500
#define NEC_LEADING_SPACE_MIN_US 4000
#define NEC_LEADING_SPACE_MAX_US 5000
#define NEC_REPEAT_SPACE_MIN_US  2000
#define NEC_REPEAT_SPACE_MAX_US  2500
#define NEC_BIT_PULSE_MIN_US     300
#define NEC_BIT_PULSE_MAX_US     800
#define NEC_BIT_0_SPACE_MIN_US   NEC_BIT_PULSE_MIN_US
#define NEC_BIT_0_SPACE_MAX_US   NEC_BIT_PULSE_MAX_US
#define NEC_BIT_1_SPACE_MIN_US   1400
#define NEC_BIT_1_SPACE_MAX_US   2000
#define NEC_FINAL_PULSE_MIN_US   NEC_BIT_PULSE_MIN_US
#define NEC_FINAL_PULSE_MAX_US   NEC_BIT_PULSE_MAX_US
#define NEC_LONGEST_PULSE_US     NEC_LEADING_PULSE_MAX_US

#define NEC_TIMER                 CAT2(TIMER, SYS_CNF_IR_NEC_TIMER_NUMBER)
#define NEC_TIMER_RISING_EDGE_CH  0
#define NEC_TIMER_FALLING_EDGE_CH 1
#define nec_timer_irq_handler     CAT3(TIMER, SYS_CNF_IR_NEC_TIMER_NUMBER, _IRQHandler)
#define nec_timer_cc_fifo_empty(channel) \
  (CAT2(TIMER_STATUS_ICFEMPTY, channel) == (NEC_TIMER->STATUS & CAT2(TIMER_STATUS_ICFEMPTY, channel)))

// Private type definitions ----------------------------------------------------
typedef enum {
  NEC_STATE_IDLE,
  NEC_STATE_LEADING_PULSE,
  NEC_STATE_LEADING_SPACE,
  NEC_STATE_DATA,
  NEC_STATE_FINAL_PULSE,
  NEC_STATE_REPEAT_SPACE,
  NEC_STATE_ERROR
} nec_decoder_state_t;

// Private variables -----------------------------------------------------------
#if CAT3(TIMER, SYS_CNF_IR_NEC_TIMER_NUMBER, _CNTWIDTH) == 16
typedef uint16_t timestamp_t;
#elif CAT3(TIMER, SYS_CNF_IR_NEC_TIMER_NUMBER, _CNTWIDTH) == 32
typedef uint32_t timestamp_t;
#else
  #error "Unsupported TIMER CNTWIDTH for IR NEC decoder!"
#endif

static uint32_t nec_timer_freq_khz;
static struct {
  volatile uint32_t rd_idx;
  volatile uint32_t wr_idx;
  struct {
    timestamp_t is_rising;
    timestamp_t timestamp;
  } data[SYS_CNF_IR_NEC_FIFO_DEPTH];
} ir_nec_fifo;

// Private function prototypes -------------------------------------------------
static inline void sli_ir_nec_decoder_fifo_put(timestamp_t timestamp, bool is_rising);
static void sli_ir_nec_decoder_process(bool edge_rising, timestamp_t timestamp);
static nec_decoder_state_t sli_ir_nec_handle_idle_state(bool edge_rising);
static nec_decoder_state_t sli_ir_nec_handle_leading_pulse_state(bool edge_rising, uint32_t duration);
static nec_decoder_state_t sli_ir_nec_handle_leading_space_state(bool edge_rising, uint32_t duration, uint32_t *nec_data, uint8_t *nec_bit_index);
static nec_decoder_state_t sli_ir_nec_handle_data_state(bool edge_rising, uint32_t duration, uint32_t *nec_data, uint8_t *nec_bit_index, bool *ned_data_was_valid);
static nec_decoder_state_t sli_ir_nec_handle_final_repeat_state(bool edge_rising, uint32_t duration, nec_decoder_state_t current_state, uint32_t nec_data, bool ned_data_was_valid);
static inline bool sli_ir_nec_pulse_is_in_range(uint32_t value_us, uint32_t min_us, uint32_t max_us);

// Function definitions --------------------------------------------------------
void sl_ir_nec_decoder_init(void)
{
  CMU_ClockEnable(CAT2(cmuClock_TIMER, SYS_CNF_IR_NEC_TIMER_NUMBER), true);

  //initialize the IR NEC FIFO so when 2 edges are captured at init, the first one is identified correctly
  ir_nec_fifo.data[0].is_rising = SYS_CNF_IR_NEC_IO_PIN_INACTIVE_STATE;

  // Initialize the TIMER for capturing IR NEC signals
  TIMER_Init_TypeDef timer_init = TIMER_INIT_DEFAULT;
  timer_init.prescale = timerPrescale16;
  TIMER_InitCC_TypeDef cc_init = TIMER_INITCC_DEFAULT;
  cc_init.edge = SYS_CNF_IR_NEC_IO_PIN_INACTIVE_STATE ? timerEdgeFalling : timerEdgeRising;
  cc_init.mode = timerCCModeCapture;
  TIMER_InitCC(NEC_TIMER, NEC_TIMER_RISING_EDGE_CH, &cc_init);
  cc_init.edge = SYS_CNF_IR_NEC_IO_PIN_INACTIVE_STATE ? timerEdgeRising : timerEdgeFalling;
  TIMER_InitCC(NEC_TIMER, NEC_TIMER_FALLING_EDGE_CH, &cc_init);
  TIMER_TopSet(NEC_TIMER, TIMER_MaxCount(NEC_TIMER));
  TIMER_Init(NEC_TIMER, &timer_init);
  nec_timer_freq_khz = CMU_ClockFreqGet(CAT2(cmuClock_TIMER, SYS_CNF_IR_NEC_TIMER_NUMBER)) / (1000 * (timer_init.prescale + 1));
  SL_ASSERT((((uint64_t)((timestamp_t)-1)) * 1000ULL) / nec_timer_freq_khz >= NEC_LONGEST_PULSE_US,
            "NEC timer frequency is too high for the longest NEC pulse duration!");

  // Route the input pin to TIMER's capture channels
  GPIO_PinModeSet(SYS_CNF_IR_NEC_IO_PORT, SYS_CNF_IR_NEC_IO_PIN, gpioModeInput, 0);
  GPIO->TIMERROUTE[SYS_CNF_IR_NEC_TIMER_NUMBER].CAT3(CC, NEC_TIMER_RISING_EDGE_CH, ROUTE) =
    (SYS_CNF_IR_NEC_IO_PORT << CAT3(_GPIO_TIMER_CC, NEC_TIMER_RISING_EDGE_CH, ROUTE_PORT_SHIFT))
    | (SYS_CNF_IR_NEC_IO_PIN << CAT3(_GPIO_TIMER_CC, NEC_TIMER_RISING_EDGE_CH, ROUTE_PIN_SHIFT));
  GPIO->TIMERROUTE[SYS_CNF_IR_NEC_TIMER_NUMBER].CAT3(CC, NEC_TIMER_FALLING_EDGE_CH, ROUTE) =
    (SYS_CNF_IR_NEC_IO_PORT << CAT3(_GPIO_TIMER_CC, NEC_TIMER_FALLING_EDGE_CH, ROUTE_PORT_SHIFT))
    | (SYS_CNF_IR_NEC_IO_PIN << CAT3(_GPIO_TIMER_CC, NEC_TIMER_FALLING_EDGE_CH, ROUTE_PIN_SHIFT));
  GPIO->TIMERROUTE[SYS_CNF_IR_NEC_TIMER_NUMBER].ROUTEEN |=
    (CAT3(GPIO_TIMER_ROUTEEN_CC, NEC_TIMER_RISING_EDGE_CH, PEN) | CAT3(GPIO_TIMER_ROUTEEN_CC, NEC_TIMER_FALLING_EDGE_CH, PEN));

  // Enable interrupts for both channels
  TIMER_IntClear(NEC_TIMER, _TIMER_IF_MASK); // Clear any pending interrupts
  TIMER_IntEnable(NEC_TIMER, CAT2(TIMER_IEN_CC, NEC_TIMER_RISING_EDGE_CH));
  TIMER_IntEnable(NEC_TIMER, CAT2(TIMER_IEN_CC, NEC_TIMER_FALLING_EDGE_CH));
  NVIC_EnableIRQ(CAT3(TIMER, SYS_CNF_IR_NEC_TIMER_NUMBER, _IRQn));
}

void nec_timer_irq_handler(void)
{
  static uint32_t last_wr_idx;
  TIMER_IntClear(NEC_TIMER, TIMER_IntGetEnabled(NEC_TIMER));

  while (!nec_timer_cc_fifo_empty(NEC_TIMER_RISING_EDGE_CH) || !nec_timer_cc_fifo_empty(NEC_TIMER_FALLING_EDGE_CH)) {
    if (!nec_timer_cc_fifo_empty(NEC_TIMER_RISING_EDGE_CH) && !nec_timer_cc_fifo_empty(NEC_TIMER_FALLING_EDGE_CH)) {
      if (ir_nec_fifo.data[last_wr_idx].is_rising) {
        sli_ir_nec_decoder_fifo_put(TIMER_CaptureGet(NEC_TIMER, NEC_TIMER_FALLING_EDGE_CH), false);
      } else {
        sli_ir_nec_decoder_fifo_put(TIMER_CaptureGet(NEC_TIMER, NEC_TIMER_RISING_EDGE_CH), true);
      }
    } else if (!nec_timer_cc_fifo_empty(NEC_TIMER_RISING_EDGE_CH)) {
      sli_ir_nec_decoder_fifo_put(TIMER_CaptureGet(NEC_TIMER, NEC_TIMER_RISING_EDGE_CH), true);
    } else if (!nec_timer_cc_fifo_empty(NEC_TIMER_FALLING_EDGE_CH)) {
      sli_ir_nec_decoder_fifo_put(TIMER_CaptureGet(NEC_TIMER, NEC_TIMER_FALLING_EDGE_CH), false);
    }
  }
  last_wr_idx = ir_nec_fifo.wr_idx;
  SL_ASSERT(0 == (TIMER_IntGet(NEC_TIMER) & (CAT2(TIMER_IF_ICFOF, NEC_TIMER_RISING_EDGE_CH) | CAT2(TIMER_IF_ICFOF, NEC_TIMER_FALLING_EDGE_CH))),
            "TIMER FIFO Overflow!" SL_LOG_EOL);
}

static inline void sli_ir_nec_decoder_fifo_put(timestamp_t timestamp, bool is_rising)
{
  uint32_t new_wr_idx = (ir_nec_fifo.wr_idx + 1) % SYS_CNF_IR_NEC_FIFO_DEPTH;

  if (new_wr_idx != ir_nec_fifo.rd_idx) {
    ir_nec_fifo.data[ir_nec_fifo.wr_idx].timestamp = timestamp;
    ir_nec_fifo.data[ir_nec_fifo.wr_idx].is_rising = is_rising;
    ir_nec_fifo.wr_idx = new_wr_idx;
  } else {
    SL_ASSERT(0, "IR NEC FIFO overflow!" SL_LOG_EOL);
  }
}

void sl_ir_nec_decoder_cyclic(void)
{
  while (ir_nec_fifo.rd_idx != ir_nec_fifo.wr_idx) {
    sli_ir_nec_decoder_process(ir_nec_fifo.data[ir_nec_fifo.rd_idx].is_rising,
                               ir_nec_fifo.data[ir_nec_fifo.rd_idx].timestamp);
    ir_nec_fifo.rd_idx = (ir_nec_fifo.rd_idx + 1) % SYS_CNF_IR_NEC_FIFO_DEPTH;
  }
}

static void sli_ir_nec_decoder_process(bool edge_rising, timestamp_t timestamp)
{
  static nec_decoder_state_t nec_state;
  static uint32_t nec_data;
  static uint8_t nec_bit_index;
  static bool ned_data_was_valid;
  static timestamp_t nec_last_timestamp;
  nec_decoder_state_t new_state = nec_state;
  timestamp_t tick_diff = timestamp - nec_last_timestamp;
  uint32_t duration = (1000 * tick_diff) / nec_timer_freq_khz;

  nec_last_timestamp = timestamp;
  switch (nec_state) {
    case NEC_STATE_IDLE:
      new_state = sli_ir_nec_handle_idle_state(edge_rising);
      break;
    case NEC_STATE_LEADING_PULSE:
      new_state = sli_ir_nec_handle_leading_pulse_state(edge_rising, duration);
      break;
    case NEC_STATE_LEADING_SPACE:
      new_state = sli_ir_nec_handle_leading_space_state(edge_rising, duration, &nec_data, &nec_bit_index);
      break;
    case NEC_STATE_DATA:
      new_state = sli_ir_nec_handle_data_state(edge_rising, duration, &nec_data, &nec_bit_index, &ned_data_was_valid);
      break;
    case NEC_STATE_REPEAT_SPACE: //Fallthrough
    case NEC_STATE_FINAL_PULSE:
      new_state = sli_ir_nec_handle_final_repeat_state(edge_rising, duration, nec_state, nec_data, ned_data_was_valid);
      break;
    default:
      new_state = NEC_STATE_IDLE;
      sl_log_error("Unexpected state: %d!" SL_LOG_EOL, nec_state);
      break;
  }

  if (new_state == NEC_STATE_ERROR) { //just a temporary state to indicate error
    new_state = NEC_STATE_IDLE;
    ned_data_was_valid = false;
    sl_log_error("Parsing error from state: %d!" SL_LOG_EOL, nec_state);
  }
  nec_state = new_state;
}

static nec_decoder_state_t sli_ir_nec_handle_idle_state(bool edge_rising)
{
  return edge_rising ? NEC_STATE_LEADING_PULSE : NEC_STATE_ERROR;
}

static nec_decoder_state_t sli_ir_nec_handle_leading_pulse_state(bool edge_rising, uint32_t duration)
{
  if (!edge_rising && sli_ir_nec_pulse_is_in_range(duration, NEC_LEADING_PULSE_MIN_US, NEC_LEADING_PULSE_MAX_US)) {
    return NEC_STATE_LEADING_SPACE;
  } else {
    sl_log_debug("Leading pulse width issue: %lu us (expected %u - %u us)" SL_LOG_EOL,
                 duration, NEC_LEADING_PULSE_MIN_US, NEC_LEADING_PULSE_MAX_US);
    return NEC_STATE_ERROR;
  }
}

static nec_decoder_state_t sli_ir_nec_handle_leading_space_state(bool edge_rising, uint32_t duration, uint32_t *nec_data, uint8_t *nec_bit_index)
{
  if (edge_rising) {
    if (sli_ir_nec_pulse_is_in_range(duration, NEC_LEADING_SPACE_MIN_US, NEC_LEADING_SPACE_MAX_US)) {
      *nec_data = 0;
      *nec_bit_index = 0;
      return NEC_STATE_DATA;
    } else if (sli_ir_nec_pulse_is_in_range(duration, NEC_REPEAT_SPACE_MIN_US, NEC_REPEAT_SPACE_MAX_US)) {
      return NEC_STATE_REPEAT_SPACE;
    } else {
      sl_log_debug("Leading space width issue: %lu us (expected %u - %u us)" SL_LOG_EOL,
                   duration, NEC_LEADING_SPACE_MIN_US, NEC_LEADING_SPACE_MAX_US);
      return NEC_STATE_ERROR;
    }
  }
  return NEC_STATE_LEADING_SPACE; // No state change
}

static nec_decoder_state_t sli_ir_nec_handle_data_state(bool edge_rising, uint32_t duration, uint32_t *nec_data, uint8_t *nec_bit_index, bool *ned_data_was_valid)
{
  nec_decoder_state_t new_state = NEC_STATE_DATA;

  if (!edge_rising) {
    if (!sli_ir_nec_pulse_is_in_range(duration, NEC_BIT_PULSE_MIN_US, NEC_BIT_PULSE_MAX_US)) {
      new_state = NEC_STATE_ERROR;
    }
  } else {
    if (sli_ir_nec_pulse_is_in_range(duration, NEC_BIT_0_SPACE_MIN_US, NEC_BIT_0_SPACE_MAX_US)) {
      *nec_data |= (0 << (*nec_bit_index)++);
    } else if (sli_ir_nec_pulse_is_in_range(duration, NEC_BIT_1_SPACE_MIN_US, NEC_BIT_1_SPACE_MAX_US)) {
      *nec_data |= (1 << (*nec_bit_index)++);
    } else {
      new_state = NEC_STATE_ERROR;
    }

    if (*nec_bit_index == (sizeof(*nec_data) * 8)) {
      *ned_data_was_valid = true;
      new_state = NEC_STATE_FINAL_PULSE;
    }
  }

  if (NEC_STATE_ERROR == new_state) {
    sl_log_debug("Data pulse width issue: %lu us (expected %u - %u us or %u - %u us)" SL_LOG_EOL,
                 duration, NEC_BIT_0_SPACE_MIN_US, NEC_BIT_0_SPACE_MAX_US, NEC_BIT_1_SPACE_MIN_US, NEC_BIT_1_SPACE_MAX_US);
  }
  return new_state;
}

static nec_decoder_state_t sli_ir_nec_handle_final_repeat_state(bool edge_rising, uint32_t duration, nec_decoder_state_t current_state, uint32_t nec_data, bool ned_data_was_valid)
{
  if (!edge_rising && sli_ir_nec_pulse_is_in_range(duration, NEC_FINAL_PULSE_MIN_US, NEC_FINAL_PULSE_MAX_US)) {
    if (ned_data_was_valid) {
      sl_ir_nec_decoder_on_data(nec_data, current_state == NEC_STATE_REPEAT_SPACE);
      sl_activity_led_set(SL_ACTIVITY_LED_INST_IR);
    } else {
      sl_log_warning("Repeat code received without valid data!" SL_LOG_EOL);
    }
    return NEC_STATE_IDLE;
  } else if (!edge_rising || !sli_ir_nec_pulse_is_in_range(duration, NEC_BIT_0_SPACE_MIN_US, NEC_BIT_1_SPACE_MAX_US)) {
    sl_log_debug("Final pulse width issue: %lu us (expected %u - %u us)" SL_LOG_EOL,
                 duration, NEC_FINAL_PULSE_MIN_US, NEC_FINAL_PULSE_MAX_US);
    return NEC_STATE_ERROR;
  }
  return current_state; // No state change
}

static inline bool sli_ir_nec_pulse_is_in_range(uint32_t value_us, uint32_t min_us, uint32_t max_us)
{
  return (value_us >= min_us && value_us <= max_us);
}

SL_WEAK void sl_ir_nec_decoder_on_data(uint32_t data, bool is_repeat)
{
  if (is_repeat) {
    sl_log_info("Repeat code received!" SL_LOG_EOL);
  } else {
    bool addr_ok = ((SL_IR_NEC_DEC_ADDR(data) ^ SL_IR_NEC_DEC_ADDR_INV(data)) == 0xFF);
    bool cmd_ok  = ((SL_IR_NEC_DEC_CMD(data) ^ SL_IR_NEC_DEC_CMD_INV(data)) == 0xFF);

    sl_log_info("Decoded data:"SL_LOG_EOL
                " - RAW: 0x%08lX" SL_LOG_EOL
                " - STANDARD (%s): Address: 0x%02lX, Command: 0x%02lX" SL_LOG_EOL
                " - EXTENDED (%s): Address: 0x%04lX, Command: 0x%02lX" SL_LOG_EOL,
                data,
                addr_ok & cmd_ok ? "OK" : "ERROR", SL_IR_NEC_DEC_ADDR(data), SL_IR_NEC_DEC_CMD(data),
                cmd_ok ? "OK" : "ERROR", SL_IR_NEC_DEC_ADDR_U16(data), SL_IR_NEC_DEC_CMD(data));
  }
}
