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
#include "sl_cli_config.h"
#include "sl_cli_command.h"
#include "sl_cli_arguments.h"
#include "sl_cli.h"
#include "sl_log.h"

#include "app/user_interface/voice.h"
#include "app/user_interface/key_handler.h"
#include "drivers/leds/sl_ir_led.h"

static void cli_voice_sample_rate(sl_cli_command_arg_t *arguments);
static void cli_voice_channels(sl_cli_command_arg_t *arguments);
static void cli_voice_filter(sl_cli_command_arg_t *arguments);
static void cli_voice_encoding(sl_cli_command_arg_t *arguments);
static void cli_voice_start_stop(sl_cli_command_arg_t *arguments);
static void cli_ir_send_code(sl_cli_command_arg_t *arguments);
static void cli_ir_send_code_ext(sl_cli_command_arg_t *arguments);
static void cli_ir_send_code_raw(sl_cli_command_arg_t *arguments);
static void cli_ir_stop_send(sl_cli_command_arg_t *arguments);
static void cli_key_handler_set_allowed_key_bitfield_ble(sl_cli_command_arg_t *args);
static void cli_key_handler_set_allowed_key_bitfield_ir(sl_cli_command_arg_t *args);
static void cli_key_handler_set_ir_address(sl_cli_command_arg_t *args);
static void cli_key_handler_set_ir_repeat_limit(sl_cli_command_arg_t *args);
static void cli_key_handler_set_ir_command_table(sl_cli_command_arg_t *args);

static const sl_cli_command_info_t cli_cmd_voice_sample_rate = \
  SL_CLI_COMMAND(cli_voice_sample_rate,
                 "Set the sample rate for voice.",
                 "Sample rate",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_voice_channels = \
  SL_CLI_COMMAND(cli_voice_channels,
                 "Set the number of channels for voice.",
                 "Channels",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_voice_filter = \
  SL_CLI_COMMAND(cli_voice_filter,
                 "Enable or disable the voice filter.",
                 "Filter status (bool)",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_voice_encoding = \
  SL_CLI_COMMAND(cli_voice_encoding,
                 "Enable or disable voice encoding.",
                 "Encoding status (bool)",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_voice_start_stop = \
  SL_CLI_COMMAND(cli_voice_start_stop,
                 "Start or stop voice functionality.",
                 "Start (1) or Stop (0) (bool)",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_ir_send_code = \
  SL_CLI_COMMAND(cli_ir_send_code,
                 "Send a standard IR frame.",
                 "Address" SL_CLI_UNIT_SEPARATOR "Code" SL_CLI_UNIT_SEPARATOR "Repeat count",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_UINT8, SL_CLI_ARG_UINT32, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_ir_send_code_ext = \
  SL_CLI_COMMAND(cli_ir_send_code_ext,
                 "Send an extended IR code using u16 address and key.",
                 "Address" SL_CLI_UNIT_SEPARATOR "Key" SL_CLI_UNIT_SEPARATOR "Repeat count",
                 { SL_CLI_ARG_UINT16, SL_CLI_ARG_UINT8, SL_CLI_ARG_UINT32, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_ir_send_code_raw = \
  SL_CLI_COMMAND(cli_ir_send_code_raw,
                 "Send a raw IR code using u32 data.",
                 "Raw data" SL_CLI_UNIT_SEPARATOR "Repeat count",
                 { SL_CLI_ARG_UINT32, SL_CLI_ARG_UINT32, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_ir_stop_send = \
  SL_CLI_COMMAND(cli_ir_stop_send,
                 "Stop sending an IR code.",
                 "",
                 { SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_key_handler_set_allowed_key_bitfield_ble = \
  SL_CLI_COMMAND(cli_key_handler_set_allowed_key_bitfield_ble,
                 "Set allowed key bitfield for BLE.",
                 "UINT64 in LSB hex format",
                 { SL_CLI_ARG_HEX, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_key_handler_set_allowed_key_bitfield_ir = \
  SL_CLI_COMMAND(cli_key_handler_set_allowed_key_bitfield_ir,
                 "Set allowed key bitfield for IR.",
                 "UINT64 in LSB hex format",
                 { SL_CLI_ARG_HEX, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_key_handler_set_ir_address = \
  SL_CLI_COMMAND(cli_key_handler_set_ir_address,
                 "Set the IR address.",
                 "Address (if address < 0xFF, standard IR code is sent, otherwise extended IR code is sent)",
                 { SL_CLI_ARG_UINT16, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_key_handler_set_ir_repeat_limit = \
  SL_CLI_COMMAND(cli_key_handler_set_ir_repeat_limit,
                 "Set the IR repeat limit.",
                 "Maximum number of repeat codes to be sent.",
                 { SL_CLI_ARG_UINT32, SL_CLI_ARG_END, });

static const sl_cli_command_info_t cli_cmd_key_handler_set_ir_command_table = \
  SL_CLI_COMMAND(cli_key_handler_set_ir_command_table,
                 "Set a single IR command in the command table.",
                 "Key ID (0-63), zero based index (button ID on schematic minus 1) "  SL_CLI_UNIT_SEPARATOR "IR command (0-255)",
                 { SL_CLI_ARG_UINT8, SL_CLI_ARG_UINT8, SL_CLI_ARG_END, });

static const sl_cli_command_entry_t cli_group_table_entry_voice[] = {
  { "sample_rate", &cli_cmd_voice_sample_rate, false },
  { "channels", &cli_cmd_voice_channels, false },
  { "filter", &cli_cmd_voice_filter, false },
  { "encoding", &cli_cmd_voice_encoding, false },
  { "start_stop", &cli_cmd_voice_start_stop, false },
  { NULL, NULL, false },
};

static const sl_cli_command_entry_t cli_group_table_entry_ir[] = {
  { "send_code", &cli_cmd_ir_send_code, false },
  { "send_code_ext", &cli_cmd_ir_send_code_ext, false },
  { "send_code_raw", &cli_cmd_ir_send_code_raw, false },
  { "stop_send", &cli_cmd_ir_stop_send, false },
  { NULL, NULL, false },
};

static const sl_cli_command_entry_t cli_group_table_entry_key_handler[] = {
  { "set_allowed_keys_ble", &cli_cmd_key_handler_set_allowed_key_bitfield_ble, false },
  { "set_allowed_keys_ir", &cli_cmd_key_handler_set_allowed_key_bitfield_ir, false },
  { "set_ir_address", &cli_cmd_key_handler_set_ir_address, false },
  { "set_ir_repeat_limit", &cli_cmd_key_handler_set_ir_repeat_limit, false },
  { "set_ir_command_table", &cli_cmd_key_handler_set_ir_command_table, false },
  { NULL, NULL, false },
};

static const sl_cli_command_info_t cli_group_table_info_voice = \
  SL_CLI_COMMAND_GROUP(cli_group_table_entry_voice, "Voice commands");

static const sl_cli_command_info_t cli_group_table_info_ir = \
  SL_CLI_COMMAND_GROUP(cli_group_table_entry_ir, "IR LED commands");

static const sl_cli_command_info_t cli_group_table_info_key_handler = \
  SL_CLI_COMMAND_GROUP(cli_group_table_entry_key_handler, "Key Handler Commands");

static const sl_cli_command_entry_t cli_command_table[] = {
  { "voice", &cli_group_table_info_voice, false },
  { "ir", &cli_group_table_info_ir, false },
  { "keys", &cli_group_table_info_key_handler, false },
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

static void cli_voice_sample_rate(sl_cli_command_arg_t *arguments)
{
  sample_rate_t sample_rate = (sample_rate_t)sl_cli_get_argument_uint8(arguments, 0);
  if (sample_rate != sr_16k && sample_rate != sr_8k) {
    sli_log("Invalid sample rate: %d" SL_LOG_EOL, sample_rate);
  } else {
    sli_log("Setting sample rate to: %d" SL_LOG_EOL, sample_rate);
    voice_set_sample_rate(sample_rate);
  }
}

static void cli_voice_channels(sl_cli_command_arg_t *arguments)
{
  uint8_t channels = sl_cli_get_argument_uint8(arguments, 0);
  if (channels > 2) {
    sli_log("Invalid number of channels: %d" SL_LOG_EOL, channels);
  } else {
    sli_log("Setting channels to: %d" SL_LOG_EOL, channels);
    voice_set_channels(channels);
  }
}

static void cli_voice_filter(sl_cli_command_arg_t *arguments)
{
  bool status = (bool)sl_cli_get_argument_uint8(arguments, 0);
  sli_log("Setting filter enable to: %s" SL_LOG_EOL, status ? "Enabled" : "Disabled");
  voice_set_filter_enable(status);
}

static void cli_voice_encoding(sl_cli_command_arg_t *arguments)
{
  bool status = (bool)sl_cli_get_argument_uint8(arguments, 0);
  sli_log("Setting encoding enable to: %s" SL_LOG_EOL, status ? "Enabled" : "Disabled");
  voice_set_encoding_enable(status);
}

static void cli_voice_start_stop(sl_cli_command_arg_t *arguments)
{
  bool start = (bool)sl_cli_get_argument_uint8(arguments, 0);
  sli_log("Voice %s" SL_LOG_EOL, start ? "started" : "stopped");
  if (start) {
    voice_start();
  } else {
    voice_stop();
  }
}

static void cli_ir_send_code(sl_cli_command_arg_t *arguments)
{
  uint8_t address = sl_cli_get_argument_uint8(arguments, 0);
  uint8_t code = sl_cli_get_argument_uint8(arguments, 1);
  uint32_t repeat_count = sl_cli_get_argument_uint32(arguments, 2);

  int sc = sl_ir_led_send(address, code, repeat_count);
  sli_log("Sending IR code... Status: %d, Address: 0x%X, Key: 0x%X" SL_LOG_EOL, sc, address, code);
}

static void cli_ir_send_code_ext(sl_cli_command_arg_t *arguments)
{
  uint16_t address = sl_cli_get_argument_uint16(arguments, 0);
  uint8_t key = sl_cli_get_argument_uint8(arguments, 1);
  uint32_t repeat_count = sl_cli_get_argument_uint32(arguments, 2);

  int sc = sl_ir_led_send_extended(address, key, repeat_count);
  sli_log("Sending extended IR code... Status: %d, Address: 0x%X, Key: 0x%X" SL_LOG_EOL, sc, address, key);
}

static void cli_ir_send_code_raw(sl_cli_command_arg_t *arguments)
{
  uint32_t raw_data = sl_cli_get_argument_uint32(arguments, 0);
  uint32_t repeat_count = sl_cli_get_argument_uint32(arguments, 1);

  int sc = sl_ir_led_send_raw(raw_data, repeat_count);
  sli_log("Sending raw IR code... Status: %d, Raw Data: 0x%lX" SL_LOG_EOL, sc, raw_data);
}

static void cli_ir_stop_send(sl_cli_command_arg_t *arguments)
{
  (void)arguments; // Unused
  sli_log("Stopping IR send" SL_LOG_EOL);
  sl_ir_led_stop();
}

static void cli_key_handler_set_allowed_key_bitfield_ble(sl_cli_command_arg_t *args)
{
  size_t size = 0;
  uint64_t allowed_bitfield = 0;
  const void *data = sl_cli_get_argument_hex(args, 0, &size);

  int sc = key_handler_config_set(KEY_HANDLER_CONFIG_ID_ALLOWED_KEY_BITFIELD_BLE, data, size);
  size = sizeof(allowed_bitfield);
  sc |= key_handler_config_get(KEY_HANDLER_CONFIG_ID_ALLOWED_KEY_BITFIELD_BLE, &allowed_bitfield, &size);
  sli_log("Set allowed_key_bitfield_ble: 0x%llX (ret=%d)" SL_LOG_EOL, allowed_bitfield, sc);
}

static void cli_key_handler_set_allowed_key_bitfield_ir(sl_cli_command_arg_t *args)
{
  size_t size = 0;
  uint64_t allowed_bitfield = 0;
  const void *data = sl_cli_get_argument_hex(args, 0, &size);

  int sc = key_handler_config_set(KEY_HANDLER_CONFIG_ID_ALLOWED_KEY_BITFIELD_IR, data, size);
  size = sizeof(allowed_bitfield);
  sc |= key_handler_config_get(KEY_HANDLER_CONFIG_ID_ALLOWED_KEY_BITFIELD_IR, &allowed_bitfield, &size);
  sli_log("Set allowed_key_bitfield_ir: 0x%llX (ret=%d)" SL_LOG_EOL, allowed_bitfield, sc);
}

static void cli_key_handler_set_ir_address(sl_cli_command_arg_t *args)
{
  uint16_t value = sl_cli_get_argument_uint16(args, 0);
  int sc = key_handler_config_set(KEY_HANDLER_CONFIG_ID_IR_ADDRESS, &value, sizeof(value));
  sli_log("Set ir_address: 0x%X (ret=%d)" SL_LOG_EOL, value, sc);
}

static void cli_key_handler_set_ir_repeat_limit(sl_cli_command_arg_t *args)
{
  uint32_t value = sl_cli_get_argument_uint32(args, 0);
  int sc = key_handler_config_set(KEY_HANDLER_CONFIG_ID_IR_REPEAT_LIMIT, &value, sizeof(value));
  sli_log("Set ir_repeat_limit: %lu, (ret=%d)" SL_LOG_EOL, value, sc);
}

static void cli_key_handler_set_ir_command_table(sl_cli_command_arg_t *args)
{
  uint8_t key_id = sl_cli_get_argument_uint8(args, 0);
  uint8_t command = sl_cli_get_argument_uint8(args, 1);
  uint8_t key_table[64];
  size_t size = sizeof(key_table);

  if (key_id >= sizeof(key_table) ) {
    sli_log("Invalid key ID or command value! Key ID: %d, Command: %d" SL_LOG_EOL, key_id, command);
  } else if (key_handler_config_get(KEY_HANDLER_CONFIG_ID_IR_KEY_TO_CMD_TABLE, key_table, &size)) {
    sli_log("Failed to get IR key to command table!" SL_LOG_EOL);
  } else {
    key_table[key_id] = command;
    int sc = key_handler_config_set(KEY_HANDLER_CONFIG_ID_IR_KEY_TO_CMD_TABLE, key_table, sizeof(key_table));
    sli_log("Set IR command table single: Key ID %d, Command %d, Result: %d" SL_LOG_EOL, key_id, command, sc);
  }
}
