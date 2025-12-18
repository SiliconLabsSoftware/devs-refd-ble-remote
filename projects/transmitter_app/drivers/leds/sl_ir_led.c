/***************************************************************************//**
 * @file sl_ir_led.c
 * @brief The NEC IR transmission protocol uses pulse distance encoding of the message bits.
 *        Each pulse burst (mark – RC transmitter ON) is 562.5µs in length,
 *        at a carrier frequency of 38kHz (26.3µs). Bits as follows:
 *         -	Logical '0' – a 562.5µs pulse burst followed by a 562.5µs space,
 *                         with a total transmit time of 1.125ms
 *         -	Logical '1' – a 562.5µs pulse burst followed by a 1.6875ms space,
 *                         with a total transmit time of 2.25ms
 *
 *        The message (least significant bit first) transmitted in order:
 *         -	a 9ms leading pulse burst
 *         -	a 4.5ms space
 *         -	the 8-bit address for the receiving device
 *         -	the 8-bit bitwise negation of the address
 *         -	the 8-bit command
 *         -	the 8-bit bitwise negation of the command
 *         -	a final 562.5µs pulse burst to signify the end of message transmission.
 *
 *        If the key on the remote controller is kept depressed, a repeat code will be issued,
 *        typically around 40ms after the pulse burst that signified the end of the message
 *        (i.e. 108 ms since the start of the previous message).
 *        The repeat code consists of the following, in order:
 *         -	a 9ms leading pulse burst
 *         -	a 2.25ms space
 *         -	a 562.5µs pulse burst to mark the end of the space.
 *
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
#include "../sl_system_config.h"
#include "sl_ir_led.h"
#include "em_cmu.h"
#include "em_gpio.h"
#include "em_timer.h"
#include "dmadrv.h"
#include "sl_sleep.h"
#include "sl_sleeptimer.h"

// Macros ----------------------------------------------------------------------
#ifndef CAT3
  #define _CAT3EXP(a, b, c)   a ## b ## c
  #define CAT3(a, b, c)      _CAT3EXP(a, b, c)
#endif
#ifndef CAT2
  #define CAT2(a, b)      CAT3(a, b, )
#endif
#ifndef ARRAY_LEN
  #define ARRAY_LEN(a)   (sizeof(a) / sizeof((a)[0]))
#endif

#define IR_LED_DMA_TRANSFER_CFG(signal, slot) { \
    .ldmaReqSel = (signal),                     \
    .ldmaCtrlSyncPrsClrOff = 0,                 \
    .ldmaCtrlSyncPrsClrOn = 0,                  \
    .ldmaCtrlSyncPrsSetOff = 0,                 \
    .ldmaCtrlSyncPrsSetOn = 0,                  \
    .ldmaReqDis = false,                        \
    .ldmaDbgHalt = false,                       \
    .ldmaCfgArbSlots = (slot),                  \
    .ldmaCfgSrcIncSign = ldmaCfgSrcIncSignPos,  \
    .ldmaCfgDstIncSign = ldmaCfgDstIncSignPos,  \
    .ldmaLoopCnt = 0                            \
}
#define IR_LED_DMA_DESCRIPTOR(src, dest, src_size, count, self_link) { \
    .xfer =  {                                                         \
      .structType   = ldmaCtrlStructTypeXfer,                          \
      .structReq    = 0,                                               \
      .xferCnt      = (count) - 1,                                     \
      .byteSwap     = 0,                                               \
      .blockSize    = ldmaCtrlBlockSizeUnit1,                          \
      .doneIfs      = 1,                                               \
      .reqMode      = ldmaCtrlReqModeBlock,                            \
      .decLoopCnt   = 0,                                               \
      .ignoreSrec   = true,                                            \
      .srcInc       = ldmaCtrlSrcIncOne,                               \
      .size         = src_size,                                        \
      .dstInc       = ldmaCtrlDstIncNone,                              \
      .srcAddrMode  = ldmaCtrlSrcAddrModeAbs,                          \
      .dstAddrMode  = ldmaCtrlDstAddrModeAbs,                          \
      .srcAddr      = (uint32_t)(src),                                 \
      .dstAddr      = (uint32_t)(dest),                                \
      .linkMode     = (self_link) ? ldmaLinkModeRel : 0,               \
      .link         = self_link,                                       \
      .linkAddr     = 0                                                \
    }                                                                  \
}

#define IR_LED_TIMER_CARRIER_NUM              SYS_CNF_IR_TIMER_CARRIER_NUM
#define IR_LED_TIMER_CARRIER                  CAT2(TIMER, IR_LED_TIMER_CARRIER_NUM)
#define IR_LED_TIMER_MODULATOR_NUM            SYS_CNF_IR_TIMER_MODULATOR_NUM
#define IR_LED_TIMER_MODULATOR                CAT2(TIMER, IR_LED_TIMER_MODULATOR_NUM)
#if (IR_LED_TIMER_CARRIER_NUM == 2 || IR_LED_TIMER_CARRIER_NUM == 4)
SL_STATIC_ASSERT((SYS_CNF_IR_LED_IO_PORT != SL_GPIO_PORT_C) && (SYS_CNF_IR_LED_IO_PORT != SL_GPIO_PORT_D),
                 "Modulator timer cannot be 2 or 4 when IR LED is on port C or D!");
#elif IR_LED_TIMER_CARRIER_NUM == 3
SL_STATIC_ASSERT((SYS_CNF_IR_LED_IO_PORT != SL_GPIO_PORT_A) && (SYS_CNF_IR_LED_IO_PORT != SL_GPIO_PORT_B),
                 "Modulator timer cannot be 3 when IR LED is on port A or B!");
#endif

#define IR_LED_TIMER_MODULATOR_REPEAT_TIME_MS 108
#define IR_LED_TIMER_MODULATOR_MAX_TIME_MS    IR_LED_TIMER_MODULATOR_REPEAT_TIME_MS //max time for modulator to be on/off
#define IR_LED_TIMER_MODULATOR_MIN_TIME_US    526 //pulse burst time
#define IR_LED_TIMER_MODULATOR_START_DELAY_US 0 //start delay for modulator

#define IR_LED_CARRIER_FREQ_KHZ              38
#if SYS_CNF_IR_TIMER_CASCADED_MODE
  #define IR_LED_MODULATOR_EXPECTED_FREQ_KHZ IR_LED_CARRIER_FREQ_KHZ
  #if 38 != IR_LED_MODULATOR_EXPECTED_FREQ_KHZ
    #error "Modulator timer frequency is not as expected!"
  #endif
  #if (IR_LED_TIMER_CARRIER_NUM + 1) != IR_LED_TIMER_MODULATOR_NUM
    #error "Timers are cascaded! Modulator timer must be 1 after carrier timer!"
  #endif
#else
  #define IR_LED_MODULATOR_EXPECTED_FREQ_KHZ 600
#endif

#define ir_led_timer_modulator_us_to_ticks(x) ((x) * IR_LED_MODULATOR_EXPECTED_FREQ_KHZ / 1000)
#define IR_LED_TIMINGS_LEADING_BURST          ir_led_timer_modulator_us_to_ticks(9000)
#define IR_LED_TIMINGS_REPEAT_CODE_SPACE      ir_led_timer_modulator_us_to_ticks(2250)
#define IR_LED_TIMINGS_GENERAL_CODE_SPACE     ir_led_timer_modulator_us_to_ticks(4500)
#define IR_LED_TIMINGS_GENERAL_CODE_0_ACTIVE  ir_led_timer_modulator_us_to_ticks(562)
#define IR_LED_TIMINGS_GENERAL_CODE_0_SPACE   IR_LED_TIMINGS_GENERAL_CODE_0_ACTIVE
#define IR_LED_TIMINGS_GENERAL_CODE_1_ACTIVE  IR_LED_TIMINGS_GENERAL_CODE_0_ACTIVE
#define IR_LED_TIMINGS_GENERAL_CODE_1_SPACE   ir_led_timer_modulator_us_to_ticks(1687)
#define IR_LED_TIMINGS_END_BURST              IR_LED_TIMINGS_GENERAL_CODE_0_ACTIVE
#define IR_LED_TIMINGS_END_TRANSFER_MARK      UINT16_MAX

// Private type definitions ----------------------------------------------------
// Private function prototypes -------------------------------------------------
static void ir_led_timer_init_carrier(void);
static void ir_led_timer_init_modulator(void);
static void ir_led_dma_init(void);
static bool ir_led_on_dma_transfer_complete(unsigned int channel, unsigned int sequence, void *user_param);
static void ir_led_on_repeat_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data);
static inline void ir_led_transfer_start(const LDMA_Descriptor_t *timing_descriptor, uint16_t init_value);

// Private variables -----------------------------------------------------------
static volatile bool ir_led_transfer_ongoing;
static uint32_t ir_led_transfer_repeat_count;

static unsigned int ir_led_dma_ch_route_en;
static unsigned int ir_led_dma_ch_top_val;
static sl_sleeptimer_timer_handle_t ir_led_repeat_timer_handle;

static uint16_t ir_led_dma_data_arr_data_code_timings[1 + 1 + (2 * 4 * 8) + 1 + 1]; //burst, space, (address, neg address, command, neg command), burst, end
static const uint32_t ir_led_dma_data_arr_route_en[] = { GPIO_TIMER_ROUTEEN_CC0PEN, 0 };
static const uint16_t ir_led_dma_data_arr_repeat_code_timings[] = {
  IR_LED_TIMINGS_LEADING_BURST,
  IR_LED_TIMINGS_REPEAT_CODE_SPACE,
  IR_LED_TIMINGS_END_BURST,
  IR_LED_TIMINGS_END_TRANSFER_MARK,
};

static const LDMA_TransferCfg_t ir_led_dma_transfer_cfg =
  IR_LED_DMA_TRANSFER_CFG(CAT3(ldmaPeripheralSignal_TIMER, IR_LED_TIMER_MODULATOR_NUM, _UFOF), ldmaCfgArbSlotsAs2);
static const LDMA_Descriptor_t ir_led_dma_descriptor_gen_code_data =
  IR_LED_DMA_DESCRIPTOR(&ir_led_dma_data_arr_data_code_timings[1],
                        &IR_LED_TIMER_MODULATOR->TOPB,
                        ldmaCtrlSizeHalf,
                        ARRAY_LEN(ir_led_dma_data_arr_data_code_timings),
                        false);
static const LDMA_Descriptor_t ir_led_dma_descriptor_repeat_code_data =
  IR_LED_DMA_DESCRIPTOR(&ir_led_dma_data_arr_repeat_code_timings[1],
                        &IR_LED_TIMER_MODULATOR->TOPB,
                        ldmaCtrlSizeHalf,
                        ARRAY_LEN(ir_led_dma_data_arr_repeat_code_timings),
                        false);
static const LDMA_Descriptor_t ir_led_dma_descriptor_route_en =
  IR_LED_DMA_DESCRIPTOR(ir_led_dma_data_arr_route_en,
                        &GPIO->TIMERROUTE[IR_LED_TIMER_CARRIER_NUM].ROUTEEN,
                        ldmaCtrlSizeWord,
                        ARRAY_LEN(ir_led_dma_data_arr_route_en),
                        true);

// Function definitions --------------------------------------------------------

void sl_ir_led_init(void)
{
  ir_led_timer_init_carrier();
  ir_led_timer_init_modulator();
  ir_led_dma_init();
}

static void ir_led_timer_init_carrier(void)
{
  // Enable the clock for TIMER
  CMU_ClockEnable(CAT2(cmuClock_TIMER, IR_LED_TIMER_CARRIER_NUM), true);

  // Configure TIMER for PWM mode
  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
  timerInit.prescale = timerPrescale1; // No prescaling
  timerInit.enable = false; // Do not start the timer yet
  TIMER_Init(IR_LED_TIMER_CARRIER, &timerInit);
  TIMER_InitCC_TypeDef timerCCInit = TIMER_INITCC_DEFAULT;
  timerCCInit.mode = timerCCModePWM; // PWM mode
  TIMER_InitCC(IR_LED_TIMER_CARRIER, 0, &timerCCInit);

  // Calculate the top value for 38 kHz frequency
  // Set the top value and configure PWM output
  uint32_t timer_freq = CMU_ClockFreqGet(CAT2(cmuClock_TIMER, IR_LED_TIMER_CARRIER_NUM));
  uint32_t top_val = (timer_freq / (IR_LED_CARRIER_FREQ_KHZ * 1000)) - 1;
  TIMER_TopSet(IR_LED_TIMER_CARRIER, top_val);
  TIMER_CompareSet(IR_LED_TIMER_CARRIER, 0, top_val / 2); // 50% duty cycle

  // Route the PWM signal to a GPIO pin
  GPIO_PinModeSet(SYS_CNF_IR_LED_IO_PORT, SYS_CNF_IR_LED_IO_PIN, gpioModePushPullAlternate, 0);
  GPIO->TIMERROUTE[IR_LED_TIMER_CARRIER_NUM].CC0ROUTE =
    (  (SYS_CNF_IR_LED_IO_PORT << _GPIO_TIMER_CC0ROUTE_PORT_SHIFT)
       | (SYS_CNF_IR_LED_IO_PIN << _GPIO_TIMER_CC0ROUTE_PIN_SHIFT));

  // Start the timer
  TIMER_Enable(IR_LED_TIMER_CARRIER, true);
}

static void ir_led_timer_init_modulator(void)
{
  TIMER_Init_TypeDef timerInit = TIMER_INIT_DEFAULT;
  CMU_ClockEnable(CAT2(cmuClock_TIMER, IR_LED_TIMER_MODULATOR_NUM), true);

#if !SYS_CNF_IR_TIMER_CASCADED_MODE
  uint32_t timer_freq = CMU_ClockFreqGet(CAT2(cmuClock_TIMER, IR_LED_TIMER_MODULATOR_NUM));
  uint32_t scaled_freq = 0;

  //find the divider for the 16 bit modulator timer which allows the timer to be on for the configured max time
  for (uint32_t i = 1; i < 2048 && !scaled_freq; i *= 2) {
    if (((timer_freq / i) * IR_LED_TIMER_MODULATOR_MAX_TIME_MS / 1000) < UINT16_MAX) {
      scaled_freq = (timer_freq / 1000) / i;
    }
  }
  SL_ASSERT(scaled_freq, "Modulator timer frequency not set!");
  SL_ASSERT(scaled_freq == IR_LED_MODULATOR_EXPECTED_FREQ_KHZ, "Modulator timer frequency is not as expected! %lu vs %lu", scaled_freq, IR_LED_MODULATOR_EXPECTED_FREQ_KHZ);
  SL_ASSERT(IR_LED_TIMER_MODULATOR_MAX_TIME_MS / scaled_freq < UINT16_MAX, "Modulator timer frequency is too large!");
  SL_ASSERT((100 * abs((int32_t)( ((ir_led_timer_modulator_us_to_ticks(IR_LED_TIMER_MODULATOR_MIN_TIME_US) * 1000 / scaled_freq))
                                  - IR_LED_TIMER_MODULATOR_MIN_TIME_US)) / IR_LED_TIMER_MODULATOR_MIN_TIME_US) < 10,
            "Modulator timer frequency error exceeds 10%%!");

  timerInit.prescale = (timer_freq / (scaled_freq * 1000) - 1);
#else
  timerInit.clkSel = timerClkSelCascade;
#endif
  timerInit.enable = false;
  TIMER_Init(IR_LED_TIMER_MODULATOR, &timerInit);
  TIMER_TopSet(IR_LED_TIMER_MODULATOR, ir_led_timer_modulator_us_to_ticks(IR_LED_TIMER_MODULATOR_START_DELAY_US));
}

static void ir_led_dma_init(void)
{
  Ecode_t sts = DMADRV_Init();
  SL_ASSERT(ECODE_OK == sts || ECODE_EMDRV_DMADRV_ALREADY_INITIALIZED == sts);

  sts = DMADRV_AllocateChannel(&ir_led_dma_ch_route_en, NULL);
  sts |= DMADRV_AllocateChannel(&ir_led_dma_ch_top_val, NULL);
  SL_ASSERT(ECODE_OK == sts);
}

int sl_ir_led_send(uint8_t address, uint8_t command, uint32_t repeat_limit)
{
  uint32_t raw_data = (address & 0xFF) | ((~address & 0xFF) << 8) | ((command & 0xFF) << 16) | ((~command & 0xFF) << 24);
  return sl_ir_led_send_raw(raw_data, repeat_limit);
}

int sl_ir_led_send_extended(uint16_t address, uint8_t command, uint32_t repeat_limit)
{
  uint32_t raw_data = address | ((command & 0xFF) << 16) | ((~command & 0xFF) << 24);
  return sl_ir_led_send_raw(raw_data, repeat_limit);
}

int sl_ir_led_send_raw(uint32_t raw_data, uint32_t repeat_limit)
{
  uint16_t *timing_ptr = ir_led_dma_data_arr_data_code_timings;
  SL_STATIC_ASSERT(ARRAY_LEN(ir_led_dma_data_arr_data_code_timings) == 1 + 1 + (2 * 32) + 1 + 1, "Data code timings array size mismatch!");

  if (ir_led_transfer_ongoing) {
    return -EBUSY; // Transfer already in progress
  }
  ir_led_transfer_ongoing = true;
  ir_led_transfer_repeat_count = repeat_limit;

  // Fill the data array with the bitcoded stream
  *timing_ptr++ = IR_LED_TIMINGS_LEADING_BURST;
  *timing_ptr++ = IR_LED_TIMINGS_GENERAL_CODE_SPACE;
  for (uint8_t i = 0; i < 32; i++) {
    if (raw_data & (1 << i)) { // Logical '1'
      *timing_ptr++ = IR_LED_TIMINGS_GENERAL_CODE_1_ACTIVE;
      *timing_ptr++ = IR_LED_TIMINGS_GENERAL_CODE_1_SPACE;
    } else { // Logical '0'
      *timing_ptr++ = IR_LED_TIMINGS_GENERAL_CODE_0_ACTIVE;
      *timing_ptr++ = IR_LED_TIMINGS_GENERAL_CODE_0_SPACE;
    }
  }
  *timing_ptr++ = IR_LED_TIMINGS_END_BURST;
  *timing_ptr = IR_LED_TIMINGS_END_TRANSFER_MARK;

  ir_led_transfer_start(&ir_led_dma_descriptor_gen_code_data, ir_led_dma_data_arr_data_code_timings[0]);
  sl_sleeptimer_start_periodic_timer_ms(&ir_led_repeat_timer_handle,
                                        IR_LED_TIMER_MODULATOR_REPEAT_TIME_MS,
                                        ir_led_on_repeat_timer_callback,
                                        NULL, 0, SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
  return 0;
}

void sl_ir_led_stop(void)
{
  ir_led_transfer_repeat_count = 0;
}

static void ir_led_on_repeat_timer_callback(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void)handle;
  (void)data;

  if (ir_led_transfer_repeat_count) {
    ir_led_transfer_repeat_count--;
    ir_led_transfer_start(&ir_led_dma_descriptor_repeat_code_data, ir_led_dma_data_arr_repeat_code_timings[0]);
  } else {
    ir_led_transfer_ongoing = false;
    sl_sleeptimer_stop_timer(&ir_led_repeat_timer_handle);
  }
}

static bool ir_led_on_dma_transfer_complete(unsigned int channel, unsigned int sequence, void *user_param)
{
  (void)channel;
  (void)sequence;
  (void)user_param;

  DMADRV_StopTransfer(ir_led_dma_ch_route_en);
  TIMER_Enable(IR_LED_TIMER_MODULATOR, false);
  TIMER_CounterSet(IR_LED_TIMER_MODULATOR, 0);
  TIMER_TopSet(IR_LED_TIMER_MODULATOR, ir_led_timer_modulator_us_to_ticks(IR_LED_TIMER_MODULATOR_START_DELAY_US));
  sl_sleep_deep_enable(true);
  GPIO->TIMERROUTE[IR_LED_TIMER_CARRIER_NUM].ROUTEEN = 0;
  return false;
}

static inline void ir_led_transfer_start(const LDMA_Descriptor_t *timing_descriptor, uint16_t init_value)
{
  sl_sleep_deep_enable(false);
  DMADRV_LdmaStartTransfer(ir_led_dma_ch_top_val, (void*)&ir_led_dma_transfer_cfg, (void*)timing_descriptor, ir_led_on_dma_transfer_complete, NULL);
  DMADRV_LdmaStartTransfer(ir_led_dma_ch_route_en, (void*)&ir_led_dma_transfer_cfg, (void*)&ir_led_dma_descriptor_route_en, NULL, NULL);

  TIMER_TopBufSet(IR_LED_TIMER_MODULATOR, init_value);
  TIMER_Enable(IR_LED_TIMER_MODULATOR, true);
}
