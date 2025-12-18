/***************************************************************************//**
 * @file  cli.c
 * @brief Command line interface (CLI) for the application.
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
/* Includes ------------------------------------------------------------------*/
#include "em_chip.h"
#include "sl_cli_config.h"
#include "sl_cli_command.h"
#include "sl_cli_arguments.h"
#include "sl_cli.h"
#include "sl_log.h"
#include "sl_ble.h"
#include "voice.h"

#define cli_printf(...) sl_log_debug(__VA_ARGS__)
#define CLI_EOL         SL_LOG_EOL

// Function prototypes for BLE CLI handlers
static void cli_ble_read(sl_cli_command_arg_t *arguments);
static void cli_ble_read_callback(sl_ble_characteristic_id_t characteristic, const void *data, size_t size);
static void cli_ble_write(sl_cli_command_arg_t *arguments);
static void cli_ble_write_callback(sl_ble_characteristic_id_t characteristic, int result);
static void cli_ble_notify(sl_cli_command_arg_t *arguments);
static void cli_ble_notify_callback(sl_ble_characteristic_id_t characteristic, const void *data, size_t size);
static void cli_ble_filter_address(sl_cli_command_arg_t *arguments);

// Function prototypes for Voice CLI handlers
static void cli_voice_start_stream(sl_cli_command_arg_t *arguments);
static void cli_voice_stop_stream(sl_cli_command_arg_t *arguments);
static void cli_voice_config_sample_rate(sl_cli_command_arg_t *arguments);
static void cli_voice_config_filter(sl_cli_command_arg_t *arguments);
static void cli_voice_config_encoding(sl_cli_command_arg_t *arguments);
static void cli_voice_config_channel_number(sl_cli_command_arg_t *arguments);
static void cli_voice_config_mic_use_left(sl_cli_command_arg_t *arguments);
static void cli_voice_config_mic_pdm_delay(sl_cli_command_arg_t *arguments);
static void cli_voice_config_i2s_output(sl_cli_command_arg_t *arguments);
static void cli_voice_config_dac_output(sl_cli_command_arg_t *arguments);
static void cli_voice_config_log_output(sl_cli_command_arg_t *arguments);

static const sl_cli_command_info_t cli_cmd_ble_read =
  SL_CLI_COMMAND(cli_ble_read,
                 "Read BLE characteristic. Args: <char_id>",
                 "Characteristic ID",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_ble_write =
  SL_CLI_COMMAND(cli_ble_write,
                 "Write BLE characteristic. Args: <char_id> <hex_bytes...>",
                 "Characteristic ID" SL_CLI_UNIT_SEPARATOR "Hexadecimal bytes format is {0xAA 0xBB 0xCC}",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_HEX, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_ble_notify =
  SL_CLI_COMMAND(cli_ble_notify,
                 "Enable/disable BLE notify. Args: <char_id> <0|1>",
                 "Characteristic ID" SL_CLI_UNIT_SEPARATOR "Disable or Enable <0|1>",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_ble_filter_address =
  SL_CLI_COMMAND(cli_ble_filter_address,
                 "Filter BLE devices by address. Args: <address>",
                 "BLE address (hexadecimal) 0-6 bytes (0 bytes to disable)",
                 { SL_CLI_ARG_HEX, SL_CLI_ARG_END, });

static const sl_cli_command_entry_t cli_group_table_entry_ble[] = {
  { "read", &cli_cmd_ble_read, false },
  { "write", &cli_cmd_ble_write, false },
  { "notify", &cli_cmd_ble_notify, false },
  { "filter", &cli_cmd_ble_filter_address, false },
  { NULL, NULL, false },
};

static const sl_cli_command_info_t cli_group_table_info_ble =
  SL_CLI_COMMAND_GROUP(cli_group_table_entry_ble, "BLE commands");

static const sl_cli_command_info_t cli_cmd_voice_start_stream =
  SL_CLI_COMMAND(cli_voice_start_stream,
                 "Start the voice audio stream.",
                 "",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_voice_stop_stream =
  SL_CLI_COMMAND(cli_voice_stop_stream,
                 "Stop the voice audio stream.",
                 "",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_voice_config_sample_rate =
  SL_CLI_COMMAND(cli_voice_config_sample_rate,
                 "Set voice sample rate in kHz (8 or 16). Args: <rate_khz>",
                 "Sample rate (kHz)",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_voice_config_filter =
  SL_CLI_COMMAND(cli_voice_config_filter,
                 "Enable/disable voice filter. Args: <0|1>",
                 "0=disable, 1=enable",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_voice_config_encoding =
  SL_CLI_COMMAND(cli_voice_config_encoding,
                 "Enable/disable voice encoding. Args: <0|1>",
                 "0=disable, 1=enable",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_voice_config_channel_number =
  SL_CLI_COMMAND(cli_voice_config_channel_number,
                 "Set number of audio channels. Args: <channels>",
                 "Channels",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_voice_config_mic_use_left =
  SL_CLI_COMMAND(cli_voice_config_mic_use_left,
                 "Use left mic. Args: <0|1>",
                 "0=disable, 1=enable",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_voice_config_mic_pdm_delay =
  SL_CLI_COMMAND(cli_voice_config_mic_pdm_delay,
                 "Set mic PDM delay. Args: <delay>",
                 "Delay",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_voice_config_i2s_output =
  SL_CLI_COMMAND(cli_voice_config_i2s_output,
                 "Enable/disable I2S output. Args: <0|1>",
                 "0=disable, 1=enable",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_voice_config_dac_output =
  SL_CLI_COMMAND(cli_voice_config_dac_output,
                 "Enable/disable DAC output. Args: <0|1>",
                 "0=disable, 1=enable",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_voice_config_log_output =
  SL_CLI_COMMAND(cli_voice_config_log_output,
                 "Enable/disable voice frame log output. Args: <0|1>",
                 "0=disable, 1=enable",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_entry_t cli_group_table_entry_voice[] = {
  { "start", &cli_cmd_voice_start_stream, false },
  { "stop", &cli_cmd_voice_stop_stream, false },
  { "rate", &cli_cmd_voice_config_sample_rate, false },
  { "filter", &cli_cmd_voice_config_filter, false },
  { "encoding", &cli_cmd_voice_config_encoding, false },
  { "channels", &cli_cmd_voice_config_channel_number, false },
  { "micleft", &cli_cmd_voice_config_mic_use_left, false },
  { "micpdm", &cli_cmd_voice_config_mic_pdm_delay, false },
  { "i2s", &cli_cmd_voice_config_i2s_output, false },
  { "dac", &cli_cmd_voice_config_dac_output, false },
  { "log", &cli_cmd_voice_config_log_output, false },
  { NULL, NULL, false },
};

static const sl_cli_command_info_t cli_group_table_info_voice =
  SL_CLI_COMMAND_GROUP(cli_group_table_entry_voice, "Voice commands. The configuration is applied when the remote device is connected!");

static const sl_cli_command_entry_t cli_command_table[] = {
  { "ble", &cli_group_table_info_ble, false },
  { "voice", &cli_group_table_info_voice, false },
  { NULL, NULL, false },
};

static sl_cli_command_group_t sl_cli_app_command_group =
{
  { NULL },
  false,
  cli_command_table
};

void sl_cli_init(void)
{
  sl_cli_command_add_command_group(sl_cli_default_handle, &sl_cli_app_command_group);
}

// -----------------------------------------------------------------------------
// BLE CLI command handler implementations

static void cli_ble_read(sl_cli_command_arg_t *arguments)
{
  sl_ble_characteristic_id_t cid = (sl_ble_characteristic_id_t)sl_cli_get_argument_uint8(arguments, 0);
  int ret = sl_ble_characteristic_read(cid, cli_ble_read_callback);
  cli_printf("BLE CLI READ issued: char=%u, ret=%d" CLI_EOL, cid, ret);
}

static void cli_ble_read_callback(sl_ble_characteristic_id_t characteristic, const void *data, size_t size)
{
  cli_printf("BLE CLI READ: char=%u, size=%u, data=[", characteristic, (unsigned)size);
  for (size_t i = 0; i < size; ++i) {
    cli_printf("%02X", ((const uint8_t*)data)[i]);
  }
  cli_printf("]" CLI_EOL);
}

static void cli_ble_write(sl_cli_command_arg_t *arguments)
{
  sl_ble_characteristic_id_t cid = (sl_ble_characteristic_id_t)sl_cli_get_argument_uint8(arguments, 0);
  size_t data_len = 0;
  const uint8_t *data = sl_cli_get_argument_hex(arguments, 1, &data_len);

  int ret = sl_ble_characteristic_write(cid, data, data_len, cli_ble_write_callback);
  cli_printf("BLE CLI WRITE: char=%u, len=%u, ret=%d, data=[", cid, (unsigned)data_len, ret);
  for (size_t i = 0; i < data_len; ++i) {
    cli_printf("%02X", data[i]);
  }
  cli_printf("]" CLI_EOL);
}

static void cli_ble_write_callback(sl_ble_characteristic_id_t characteristic, int result)
{
  cli_printf("BLE CLI WRITE CALLBACK: char=%d, result=%d" CLI_EOL, characteristic, result);
}

static void cli_ble_notify(sl_cli_command_arg_t *arguments)
{
  sl_ble_characteristic_id_t cid = (sl_ble_characteristic_id_t)sl_cli_get_argument_uint8(arguments, 0);
  bool enable = sl_cli_get_argument_uint8(arguments, 1);

  int ret = sl_ble_characteristic_notify(cid, enable ? &cli_ble_notify_callback : NULL);
  cli_printf("BLE CLI NOTIFY %s: char=%u, ret=%d" CLI_EOL, enable ? "EN" : "DIS", cid, ret);
}

static void cli_ble_notify_callback(sl_ble_characteristic_id_t characteristic, const void *data, size_t size)
{
  cli_printf("BLE CLI NOTIFY: char=%u, size=%u, data=[", characteristic, (unsigned)size);
  for (size_t i = 0; i < size; ++i) {
    cli_printf("%02X", ((const uint8_t*)data)[i]);
  }
  cli_printf("]" CLI_EOL);
}

static void cli_ble_filter_address(sl_cli_command_arg_t *arguments)
{
  size_t data_len = 0;
  const uint8_t *address = sl_cli_get_argument_hex(arguments, 0, &data_len);

  // Correct the function call to sl_ble_set_address_filter
  int sc = sl_ble_set_address_filter(address, data_len);
  cli_printf("BLE address filter %s for address [", data_len > 0 ? "enabled" : "disabled");
  for (size_t i = 0; i < data_len; ++i) {
    cli_printf("%02X", address[i]);
  }
  cli_printf("] ret=%d" CLI_EOL, sc);
}

// -----------------------------------------------------------------------------
// Voice CLI command handler implementations

static void cli_voice_start_stream(sl_cli_command_arg_t *arguments)
{
  (void)arguments;
  int res = sl_voice_trigger_start_stream_on_remote();
  cli_printf("Voice stream started: %d" CLI_EOL, res);
}

static void cli_voice_stop_stream(sl_cli_command_arg_t *arguments)
{
  (void)arguments;
  sl_voice_stop_stream();
  int res = sl_voice_trigger_stop_stream_on_remote();
  cli_printf("Voice stream stopped: %d" CLI_EOL, res);
}

static void cli_voice_config_sample_rate(sl_cli_command_arg_t *arguments)
{
  uint8_t rate = sl_cli_get_argument_uint8(arguments, 0);
  sl_voice_config_sample_rate(rate);
  cli_printf("Sample rate set to %u kHz" CLI_EOL, rate);
}

static void cli_voice_config_filter(sl_cli_command_arg_t *arguments)
{
  bool enable = sl_cli_get_argument_uint8(arguments, 0) ? true : false;
  sl_voice_config_filter(enable);
  cli_printf("Filter %s" CLI_EOL, enable ? "enabled" : "disabled");
}

static void cli_voice_config_encoding(sl_cli_command_arg_t *arguments)
{
  bool enable = sl_cli_get_argument_uint8(arguments, 0) ? true : false;
  sl_voice_config_encoding(enable);
  cli_printf("Encoding %s" CLI_EOL, enable ? "enabled" : "disabled");
}

static void cli_voice_config_channel_number(sl_cli_command_arg_t *arguments)
{
  uint8_t channels = sl_cli_get_argument_uint8(arguments, 0);
  sl_voice_config_channel_number(channels);
  cli_printf("Channel number set to %u" CLI_EOL, channels);
}

static void cli_voice_config_mic_use_left(sl_cli_command_arg_t *arguments)
{
  bool use_left = sl_cli_get_argument_uint8(arguments, 0) ? true : false;
  sl_voice_config_mic_use_left(use_left);
  cli_printf("Mic use left %s" CLI_EOL, use_left ? "enabled" : "disabled");
}

static void cli_voice_config_mic_pdm_delay(sl_cli_command_arg_t *arguments)
{
  uint8_t delay = sl_cli_get_argument_uint8(arguments, 0);
  sl_voice_config_mic_pdm_delay(delay);
  cli_printf("Mic PDM delay set to %u" CLI_EOL, delay);
}

static void cli_voice_config_i2s_output(sl_cli_command_arg_t *arguments)
{
  bool enable = sl_cli_get_argument_uint8(arguments, 0) ? true : false;
  sl_voice_config_i2s_output(enable);
  cli_printf("I2S output %s" CLI_EOL, enable ? "enabled" : "disabled");
}

static void cli_voice_config_dac_output(sl_cli_command_arg_t *arguments)
{
  bool enable = sl_cli_get_argument_uint8(arguments, 0) ? true : false;
  sl_voice_config_dac_output(enable);
  cli_printf("DAC output %s" CLI_EOL, enable ? "enabled" : "disabled");
}

static void cli_voice_config_log_output(sl_cli_command_arg_t *arguments)
{
  bool enable = sl_cli_get_argument_uint8(arguments, 0) ? true : false;
  sl_voice_config_log_output(enable);
  cli_printf("Voice frame log output %s" CLI_EOL, enable ? "enabled" : "disabled");
}
