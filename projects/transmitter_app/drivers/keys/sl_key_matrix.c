/***************************************************************************//**
 * @file sl_key_matrix.c
 * @brief Key Matrix Driver for 6x8 matrix with extra button
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
#include <stddef.h>
#include "assert.h"
#include "sl_key_matrix.h"
#include "em_cmu.h"
#include "em_letimer.h"
#include "em_gpio.h"
#include "sl_gpio.h" //only used for the GPIO callback, otherwise do NOT use this!
#include "sl_sleeptimer.h"
#include "sl_atomic.h"
#include "../sl_system_config.h"

// Macros ----------------------------------------------------------------------
#define KEY_MATRIX_TIMER_FREQ_HZ                     32768ULL
#define KEY_MATRIX_TIMER_TO_TICKS(us)                ((us) * KEY_MATRIX_TIMER_FREQ_HZ / 1000000ULL)
#define KEY_MATRIX_TIMER_LETIMER_MAX_TICKS           _LETIMER_CNT_MASK
#define KEY_MATRIX_TIMER_TIME_PERIOD_TIME_US         (SYS_CNF_KEY_MATRIX_CYCLE_TIME_MS * 1000)
#define KEY_MATRIX_TIMER_TIME_OUT_CHANGE_SETTLE_US   SYS_CNF_KEY_MATRIX_TIMER_TIME_OUT_CHANGE_SETTLE_US
SL_STATIC_ASSERT(KEY_MATRIX_TIMER_TO_TICKS(KEY_MATRIX_TIMER_TIME_OUT_CHANGE_SETTLE_US) >= 1, "Key matrix timer timeout change settle time must be at least 1 tick");
//
#define KEY_MATRIX_MASK_ALL       0xFFFFFFFFUL
#define KEY_MATRIX_MASK_ALL_HI    0xFFFFFFFFUL
#define KEY_MATRIX_MASK_ALL_LO    0x00000000UL
//
#define KEY_MATRIX_BUTTON_COUNT   (ARR_LEN(key_matrix_rows) * ARR_LEN(key_matrix_cols))
//
#define KEY_MATRIX_PRESS_POLARITY 0 /* 0: Active low, 1: Active high */
//
#define ARR_LEN(x) (sizeof(x) / sizeof(x[0]))
//
#if SYS_CNF_TEST_EN_KEYS
#define key_matrix_test_inject_is_on() (0xFF != key_matrix_test_data.key_id)
#define key_matrix_test_inject_key_press(key_id, duration_ms)                                                                 \
  do {                                                                                                                        \
    if ((key_id) < KEY_MATRIX_BUTTON_COUNT && (duration_ms) > 0) {                                                            \
      key_matrix_test_data.key_id = (key_id);                                                                                 \
      key_matrix_test_data.cycle_count = (duration_ms + SYS_CNF_KEY_MATRIX_CYCLE_TIME_MS) / SYS_CNF_KEY_MATRIX_CYCLE_TIME_MS; \
      GPIO_IntDisable(key_matrix_int_mask);                                                                                   \
      key_matrix_start_timer(KEY_MATRIX_TIMER_TIME_PERIOD_TIME_US);                                                           \
      key_matrix_state = KEY_MATRIX_STATE_IDLE;                                                                               \
      key_matrix_set_out(KEY_MATRIX_MASK_ALL, KEY_MATRIX_MASK_ALL_LO, key_matrix_cols, ARR_LEN(key_matrix_cols));             \
    } else {                                                                                                                  \
      key_matrix_test_data.cycle_count = 0;                                                                                   \
    }                                                                                                                         \
  } while (0)
#define key_matrix_test_inject_on_irq()                             \
  do {                                                              \
    if (key_matrix_test_data.cycle_count > 0) {                     \
      key_matrix_test_data.cycle_count--;                           \
      sl_key_matrix_on_event(1ULL << key_matrix_test_data.key_id);  \
      key_matrix_start_timer(KEY_MATRIX_TIMER_TIME_PERIOD_TIME_US); \
    } else {                                                        \
      key_matrix_test_data.key_id = 0xFF;                           \
      sl_key_matrix_on_event(0);                                    \
      GPIO_IntClear(key_matrix_int_mask);                           \
      GPIO_IntEnable(key_matrix_int_mask);                          \
    }                                                               \
  } while (0)
#else
#define key_matrix_test_inject_is_on() (false)
#define key_matrix_test_inject_key_press(key_id, duration_ms) \
  do {                                                        \
    (void)(key_id);                                           \
    (void)(duration_ms);                                      \
  } while (0)
#define key_matrix_test_inject_on_irq()
#endif

// Private type definitions ----------------------------------------------------
typedef uint16_t key_matrix_gpio_arr_t;
typedef enum {
  KEY_MATRIX_STATE_IDLE,
  KEY_MATRIX_STATE_SCAN,
  KEY_MATRIX_STATE_SET_INTERRUPT_ENABLE,
} key_matrix_state_t;

// Private function prototypes -------------------------------------------------
static int key_matrix_init_gpio(void);
static int key_matrix_init_timer(void);
static void key_matrix_init_check_already_pushed_buttons(void);
static void key_matrix_on_gpioint(uint8_t interrupt_number, void *context);
#if !SYS_CNF_KEY_MATRIX_USE_LETIMER
static void key_matrix_on_sleeptimer_irq(sl_sleeptimer_timer_handle_t *handle, void *data);
#endif
static inline void key_matrix_on_interrupt(void);
static inline void key_matrix_state_machine(void);
static inline void key_matrix_set_out(uint32_t mask, uint32_t state_mask, const key_matrix_gpio_arr_t *gpio_arr, size_t gpio_arr_size);
static inline void key_matrix_set_mode(uint32_t mask, uint32_t state_mask, GPIO_Mode_TypeDef mode, const key_matrix_gpio_arr_t *gpio_arr, size_t gpio_arr_size);
static inline uint64_t key_matrix_read(const key_matrix_gpio_arr_t *gpio_arr, size_t gpio_arr_size, uint32_t bit_spacing);
static inline void key_matrix_start_timer(uint32_t timeout_us);

// Private variables -----------------------------------------------------------
static uint32_t key_matrix_int_mask;
static key_matrix_state_t key_matrix_state;
#if SYS_CNF_TEST_EN_KEYS
static struct {
  uint8_t key_id;
  uint32_t cycle_count;
} key_matrix_test_data;
#endif

static const key_matrix_gpio_arr_t key_matrix_rows[] = SYS_CNF_KEY_MATRIX_ROWS;
static const key_matrix_gpio_arr_t key_matrix_cols[] = SYS_CNF_KEY_MATRIX_COLS;

// Function definitions --------------------------------------------------------

int sl_key_matrix_init(void)
{
  int sc = key_matrix_init_timer();
  sc |= key_matrix_init_gpio();
  SL_ATOMIC_SECTION(
    key_matrix_init_check_already_pushed_buttons();
    );
  return sc;
}

static int key_matrix_init_gpio(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);

  key_matrix_set_mode(KEY_MATRIX_MASK_ALL, KEY_MATRIX_MASK_ALL_LO, gpioModeWiredAnd, key_matrix_cols, ARR_LEN(key_matrix_cols));
  for (uint32_t i = 0; i < ARR_LEN(key_matrix_rows); i++) {
    const sl_gpio_t gpio = {
      .port = SYS_CNF_PORT_PIN_GET_PORT(key_matrix_rows[i]),
      .pin = SYS_CNF_PORT_PIN_GET_PIN(key_matrix_rows[i])
    };
    int32_t int_number = SL_GPIO_INTERRUPT_UNAVAILABLE;
    sl_gpio_configure_wakeup_em4_interrupt(&gpio, &int_number, false, key_matrix_on_gpioint, NULL);
  #if !SYS_CNF_KEY_MATRIX_USE_INTERNAL_PULL_UP
    GPIO_PinModeSet(gpio.port, gpio.pin, gpioModeInput, 0); //The API above set pull-up but it is not needed
  #endif
    key_matrix_int_mask |= (1UL << (int_number + _GPIO_EM4WUEN_EM4WUEN_SHIFT));
    SL_ASSERT(int_number != SL_GPIO_INTERRUPT_UNAVAILABLE, "Failed to configure a ROW %d interrupt pin!", i);
  }
  return SL_POPCOUNT32(key_matrix_int_mask) == ARR_LEN(key_matrix_rows) ? 0 : -EIO;
}

static int key_matrix_init_timer(void)
{
  int sc = 0;

#if SYS_CNF_KEY_MATRIX_USE_LETIMER
  CMU_ClockEnable(cmuClock_LETIMER0, true);

  LETIMER_Init_TypeDef letimerInit = LETIMER_INIT_DEFAULT;
  LETIMER_Init(LETIMER0, &letimerInit);
  NVIC_EnableIRQ(LETIMER0_IRQn);
#else
  uint32_t freq = sl_sleeptimer_get_timer_frequency();
  sc = (freq == KEY_MATRIX_TIMER_FREQ_HZ) ? 0 : -EIO;
  SL_ASSERT(sc == 0, "Key matrix timer frequency mismatch: expected %lu Hz, got %lu Hz", KEY_MATRIX_TIMER_FREQ_HZ, freq);
#endif
  return sc;
}

static void key_matrix_init_check_already_pushed_buttons(void)
{
  if (KEY_MATRIX_STATE_IDLE == key_matrix_state
      && (key_matrix_read(key_matrix_rows, ARR_LEN(key_matrix_rows), 1))) {
    GPIO_IntSet(key_matrix_int_mask);
  }
}

int sl_key_matrix_test_inject_key_press(uint8_t key_id, uint32_t duration_ms)
{
  key_matrix_test_inject_key_press(key_id, duration_ms);
  return key_matrix_test_inject_is_on() ? 0 : -EIO;
}

static void key_matrix_on_gpioint(uint8_t interrupt_number, void *context)
{
  (void)context;
  (void)interrupt_number;
  GPIO_IntDisable(key_matrix_int_mask);
  key_matrix_on_interrupt();
}

#if SYS_CNF_KEY_MATRIX_USE_LETIMER
void LETIMER0_IRQHandler(void)
{
  LETIMER_IntDisable(LETIMER0, _LETIMER_IEN_MASK);
  key_matrix_on_interrupt();
}
#else
static void key_matrix_on_sleeptimer_irq(sl_sleeptimer_timer_handle_t *handle, void *data)
{
  (void)handle;
  (void)data;
  key_matrix_on_interrupt();
}
#endif

static inline void key_matrix_on_interrupt(void)
{
  if (!key_matrix_test_inject_is_on()) {
    key_matrix_state_machine();
  } else {
    key_matrix_test_inject_on_irq();
  }
}

static inline void key_matrix_state_machine(void)
{
  static uint32_t col_idx;
  static uint64_t keys_pressed;
  uint32_t delay_us = 0;

  switch (key_matrix_state) {
    case KEY_MATRIX_STATE_IDLE:
      key_matrix_set_out(KEY_MATRIX_MASK_ALL, ~(1UL << col_idx), key_matrix_cols, ARR_LEN(key_matrix_cols));
      delay_us = KEY_MATRIX_TIMER_TIME_OUT_CHANGE_SETTLE_US;
      key_matrix_state = KEY_MATRIX_STATE_SCAN;
      break;

    case KEY_MATRIX_STATE_SCAN:
      uint64_t rows = key_matrix_read(key_matrix_rows, ARR_LEN(key_matrix_rows), ARR_LEN(key_matrix_cols));
      keys_pressed |= (rows << col_idx);
      col_idx++;

      bool scan_ongoing = (col_idx < ARR_LEN(key_matrix_cols));
      if (scan_ongoing) {
        key_matrix_set_out(0b11 << (col_idx - 1), ~(1UL << col_idx), key_matrix_cols, ARR_LEN(key_matrix_cols));
        delay_us = KEY_MATRIX_TIMER_TIME_OUT_CHANGE_SETTLE_US;
      } else if (keys_pressed) {
        key_matrix_set_out((1UL << (col_idx - 1)), KEY_MATRIX_MASK_ALL_HI, key_matrix_cols, ARR_LEN(key_matrix_cols)); //Disable columns until the next scan to save power
        delay_us = KEY_MATRIX_TIMER_TIME_PERIOD_TIME_US;
        key_matrix_state = KEY_MATRIX_STATE_IDLE;
      } else {
        key_matrix_set_out(KEY_MATRIX_MASK_ALL, KEY_MATRIX_MASK_ALL_LO, key_matrix_cols, ARR_LEN(key_matrix_cols));
        delay_us = KEY_MATRIX_TIMER_TIME_OUT_CHANGE_SETTLE_US;
        key_matrix_state = KEY_MATRIX_STATE_SET_INTERRUPT_ENABLE;
      }

      if (!scan_ongoing) {
        sl_key_matrix_on_event(keys_pressed);
        keys_pressed = 0;
        col_idx = 0;
      }
      break;

    case KEY_MATRIX_STATE_SET_INTERRUPT_ENABLE:
      GPIO_IntClear(key_matrix_int_mask);
      GPIO_IntEnable(key_matrix_int_mask);
      key_matrix_state = KEY_MATRIX_STATE_IDLE; // Go back to idle state
      break;

    default:
      break;
  }

  if (delay_us > 0) {
    key_matrix_start_timer(delay_us);
  }
}

static inline void key_matrix_set_out(uint32_t mask, uint32_t state_mask, const key_matrix_gpio_arr_t *gpio_arr, size_t gpio_arr_size)
{
  mask &= ((1UL << gpio_arr_size) - 1);

  while (mask) {
    uint32_t idx = SL_CTZ(mask);
    mask &= ~(1UL << idx);

    if ((state_mask >> idx) & 0x01) {
      GPIO_PinOutSet(SYS_CNF_PORT_PIN_GET_PORT(gpio_arr[idx]), SYS_CNF_PORT_PIN_GET_PIN(gpio_arr[idx]));
    } else {
      GPIO_PinOutClear(SYS_CNF_PORT_PIN_GET_PORT(gpio_arr[idx]), SYS_CNF_PORT_PIN_GET_PIN(gpio_arr[idx]));
    }
  }
}

static inline void key_matrix_set_mode(uint32_t mask, uint32_t state_mask, GPIO_Mode_TypeDef mode, const key_matrix_gpio_arr_t *gpio_arr, size_t gpio_arr_size)
{
  mask &= ((1UL << gpio_arr_size) - 1);

  while (mask) {
    uint32_t idx = SL_CTZ(mask);
    mask &= ~(1UL << idx);

    uint8_t port = SYS_CNF_PORT_PIN_GET_PORT(gpio_arr[idx]);
    uint8_t pin = SYS_CNF_PORT_PIN_GET_PIN(gpio_arr[idx]);
    if ((state_mask >> idx) & 0x01) {
      GPIO_PinOutSet(port, pin);
    } else {
      GPIO_PinOutClear(port, pin);
    }
    volatile uint32_t *reg = (pin < 8) ? &(GPIO->P[port].MODEL) : &(GPIO->P[port].MODEH);
    const uint32_t shift = (pin % 8) * 4;
    *reg = (*reg & ~(0xFU << shift)) | (mode << shift);
  }
}

static inline uint64_t key_matrix_read(const key_matrix_gpio_arr_t *gpio_arr, size_t gpio_arr_size, uint32_t bit_spacing)
{
  uint64_t bitmask = 0;

  for (size_t i = 0; i < gpio_arr_size; i++) {
    if (KEY_MATRIX_PRESS_POLARITY == GPIO_PinInGet(SYS_CNF_PORT_PIN_GET_PORT(gpio_arr[i]), SYS_CNF_PORT_PIN_GET_PIN(gpio_arr[i]))) {
      bitmask |= (1ULL << (i * bit_spacing));
    }
  }
  return bitmask;
}

static inline void key_matrix_start_timer(uint32_t timeout_us)
{
#if SYS_CNF_KEY_MATRIX_USE_LETIMER
  uint32_t new_comp;
  uint32_t counter = LETIMER0->CNT;
  if (counter >= KEY_MATRIX_TIMER_TO_TICKS(timeout_us)) {
    new_comp = counter - KEY_MATRIX_TIMER_TO_TICKS(timeout_us);
  } else {
    new_comp = KEY_MATRIX_TIMER_LETIMER_MAX_TICKS - (KEY_MATRIX_TIMER_TO_TICKS(timeout_us) - counter);
  }
  LETIMER0->COMP0 = new_comp;
  LETIMER_IntClear(LETIMER0, _LETIMER_IF_MASK);
  LETIMER_IntEnable(LETIMER0, LETIMER_IEN_COMP0);
#else
  static sl_sleeptimer_timer_handle_t timer_handle;
  sl_status_t status = sl_sleeptimer_start_timer(&timer_handle,
                                                 KEY_MATRIX_TIMER_TO_TICKS(timeout_us),
                                                 key_matrix_on_sleeptimer_irq,
                                                 NULL,
                                                 0x07,
                                                 SL_SLEEPTIMER_NO_HIGH_PRECISION_HF_CLOCKS_REQUIRED_FLAG);
  SL_ASSERT(status == SL_STATUS_OK, "Failed to start key matrix timer: %d", status);
#endif
}

// Weakly defined callback - user can/will override this function
SL_WEAK void sl_key_matrix_on_event(uint64_t keys_pressed)
{
  (void)keys_pressed;
}
