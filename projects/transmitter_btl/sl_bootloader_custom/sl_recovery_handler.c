#include "sl_recovery_handler.h"
#include "sl_system_config.h"
#include "sl_gpio.h"
#include "assert.h"
#include "sl_dwt_handler.h"
#include "em_cmu.h"

// Macros ----------------------------------------------------------------------

// Global variables -------------------------------------------------------------
static const uint32_t key_matrix_rows[] = SYS_CNF_KEY_MATRIX_ROWS;
static const uint32_t key_matrix_cols[] = SYS_CNF_KEY_MATRIX_COLS;
static const uint32_t recovery_key_combination[] = SYS_CNF_RECOVERY_KEY_SW_COMBINATION;
static const uint32_t recovery_key_count = SYS_CNF_ARR_LEN(recovery_key_combination);

// Local function prototypes ---------------------------------------------------------
/**
 * @brief Scan the key matrix and return the current button state.
 *
 * This function scans all rows and columns of the key matrix and returns
 * a 64-bit value representing the current state of all keys.
 *
 * @return 64-bit value representing the pressed keys in the matrix.
 */
static uint64_t sl_recovery_scan_matrix(void);

/**
 * @brief Check if the required recovery key combination is pressed.
 *
 * This function checks if the specific key combination required for
 * recovery mode is currently pressed.
 *
 * @return true if the recovery key combination is pressed, false otherwise.
 */
static bool sl_recovery_check_key_combination(void);

// Local function definitions ---------------------------------------------------
static uint64_t sl_recovery_scan_matrix(void)
{
  uint64_t matrix_button_state = 0;
  bool button_state = false;

  for (uint32_t col_idx = 0; col_idx < SYS_CNF_ARR_LEN(key_matrix_cols); col_idx++) {
    const sl_gpio_t col_config = {
      .port = SYS_CNF_PORT_PIN_GET_PORT(key_matrix_cols[col_idx]),
      .pin = SYS_CNF_PORT_PIN_GET_PIN(key_matrix_cols[col_idx])
    };
    sl_gpio_set_pin(&col_config);
    // delay to allow the GPIO to settle
    sl_dwt_handler_blocking_delay_us(SYS_CNF_KEY_MATRIX_TIMER_TIME_OUT_CHANGE_SETTLE_US);
    for (uint32_t row_idx = 0; row_idx < SYS_CNF_ARR_LEN(key_matrix_rows); row_idx++) {
      const sl_gpio_t row_config = {
        .port = SYS_CNF_PORT_PIN_GET_PORT(key_matrix_rows[row_idx]),
        .pin = SYS_CNF_PORT_PIN_GET_PIN(key_matrix_rows[row_idx])
      };
      sl_gpio_get_pin_input(&row_config, &button_state);
      if (button_state) {
        // Set the bit for the pressed button in the matrix state
        matrix_button_state |= (1ULL << (col_idx * SYS_CNF_ARR_LEN(key_matrix_cols) + row_idx));
      }
    }
    sl_gpio_clear_pin(&col_config);
  }
  return matrix_button_state;
}

static bool sl_recovery_check_key_combination(void)
{
  uint64_t matrix_button_state = sl_recovery_scan_matrix();
  uint64_t recovery_key_state = 0;

  // Calculate recovery key position in the matrix
  for (uint32_t i = 0; i < recovery_key_count; i++) {
    uint32_t converted_recovery_key_combination = recovery_key_combination[i] - 1;
    uint8_t pos_r = (uint8_t)(converted_recovery_key_combination / SYS_CNF_ARR_LEN(key_matrix_cols));
    uint8_t pos_c = (uint8_t)(converted_recovery_key_combination % SYS_CNF_ARR_LEN(key_matrix_cols));

    const uint64_t recovery_key_bit = (1ULL << (pos_c * SYS_CNF_ARR_LEN(key_matrix_rows) + pos_r));
    if (!(recovery_key_bit & matrix_button_state)) {
      // If any of the recovery keys is not pressed, return false
      return false;
    } else {
      // Set the bit for the pressed recovery key in the recovery key state
      recovery_key_state |= recovery_key_bit;
    }
  }

  return (matrix_button_state == recovery_key_state);
}

// Function definitions --------------------------------------------------------
void sl_recovery_init(void)
{
  sl_status_t status;

  // Initialize the GPIO subsystem
  status = sl_gpio_init();
  SL_ASSERT(status == SL_STATUS_OK);

  for (uint8_t btn_idx = 0; btn_idx < SYS_CNF_ARR_LEN(key_matrix_rows); btn_idx++) {
    const sl_gpio_t row_config = {
      .port = SYS_CNF_PORT_PIN_GET_PORT(key_matrix_rows[btn_idx]),
      .pin = SYS_CNF_PORT_PIN_GET_PIN(key_matrix_rows[btn_idx])
    };
    sl_gpio_set_pin_mode(&row_config, SL_GPIO_MODE_INPUT_PULL, 0);
  }
  for (uint8_t btn_idx = 0; btn_idx < SYS_CNF_ARR_LEN(key_matrix_cols); btn_idx++) {
    const sl_gpio_t col_config = {
      .port = SYS_CNF_PORT_PIN_GET_PORT(key_matrix_cols[btn_idx]),
      .pin = SYS_CNF_PORT_PIN_GET_PIN(key_matrix_cols[btn_idx])
    };
    sl_gpio_set_pin_mode(&col_config, SL_GPIO_MODE_WIRED_OR, 0);
  }

  sl_dwt_handler_init();
}

void sl_recovery_deinit(void)
{
  for (uint8_t btn_idx = 0; btn_idx < SYS_CNF_ARR_LEN(key_matrix_rows); btn_idx++) {
    const sl_gpio_t row_config = {
      .port = SYS_CNF_PORT_PIN_GET_PORT(key_matrix_rows[btn_idx]),
      .pin = SYS_CNF_PORT_PIN_GET_PIN(key_matrix_rows[btn_idx])
    };
    sl_gpio_set_pin_mode(&row_config, SL_GPIO_MODE_DISABLED, 0);
  }
  for (uint8_t btn_idx = 0; btn_idx < SYS_CNF_ARR_LEN(key_matrix_cols); btn_idx++) {
    const sl_gpio_t col_config = {
      .port = SYS_CNF_PORT_PIN_GET_PORT(key_matrix_cols[btn_idx]),
      .pin = SYS_CNF_PORT_PIN_GET_PIN(key_matrix_cols[btn_idx])
    };
    sl_gpio_set_pin_mode(&col_config, SL_GPIO_MODE_DISABLED, 0);
  }
  CMU_ClockEnable(cmuClock_GPIO, false);

  sl_dwt_handler_deinit();
}

bool sl_recovery_verify_condition(void)
{
  const uint32_t recovery_enter_start_dwt = sl_dwt_handler_get_cycle_count();

  while (!sl_dwt_handler_is_elapsed_ms(recovery_enter_start_dwt, SYS_CNF_RECOVERY_ENTER_TIMEOUT_MS)) {
    // Wait until the timeout is reached
    if (!sl_recovery_check_key_combination()) {
      return false;
    }
  }
  // Timeout reached with recovery buttons pressed, enter recovery mode
  return true;
}
