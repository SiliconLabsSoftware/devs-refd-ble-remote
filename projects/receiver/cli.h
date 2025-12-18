/***************************************************************************//**
 * @file cli.h
 * @brief Header file for the Command Line Interface (CLI) module.
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
#ifndef _APP_CLI_H_
#define _APP_CLI_H_
#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************//**
 * @brief Initializes the CLI module.
 *
 * This function sets up the CLI command groups and registers them with the
 * CLI instance. It should be called during system initialization.
 ******************************************************************************/
void sl_cli_init(void);

#ifdef __cplusplus
}
#endif
#endif /* _APP_CLI_H_ */
