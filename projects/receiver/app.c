/***************************************************************************//**
 * @file
 * @brief Core application logic.
 *******************************************************************************
 * # License
 * <b>Copyright 2024 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/
#define SL_LOG_MODULE_NAME "Main"
#include "em_ldma.h"
#include "em_rmu.h"
#include "sl_power_manager.h"

#include "app.h"
#include "cli.h"
#include "app_properties_config.h"
#include "app/receiver.h"
#include "../../common/src/sl_build_time.h"
#include "../../common/src/drivers/sl_log.h"

static void app_init_log_reset_information(void);

void app_init(void)
{
  LDMA_Init_t dma_init = LDMA_INIT_DEFAULT;
  LDMA_Init(&dma_init);

  app_init_log_reset_information();
  sl_cli_init();
  sl_receiver_app_init();

  //This application does not need to be battery powered,
  //so keep the device always in EM1 mode (to avoid any issue).
  sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
}

void app_process_action(void)
{
  sl_receiver_app_cyclic();
}

static void app_init_log_reset_information(void)
{
  uint32_t reset_reason = RMU_ResetCauseGet();
  RMU_ResetCauseClear();

  sl_log_warning("Receiver application boot!" SL_LOG_EOL
                 " Reset reason: 0x%lX" SL_LOG_EOL
                 " Application version: 0x%X" SL_LOG_EOL
                 " Build time: " SL_BUILD_TIME_STRING SL_LOG_EOL,
                 reset_reason, SL_APPLICATION_VERSION);
}
