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
#include "em_emu.h"
#include "em_cmu.h"
#include "em_rmu.h"
#include "em_wdog.h"
#include "btl_interface.h"
#include "app_properties_config.h"
#include "sl_device_init_dcdc_config.h"

#include "app.h"
#include "cli.h"
#include "app/remote.h"
#include "../sl_system_config.h"
#include "../../common/src/drivers/sl_log.h"
#include "../../common/src/sl_build_time.h"

#if DEBUG
  #define APP_TYPE "DEBUG"
#else
  #define APP_TYPE "RELEASE"
#endif

static void app_init_watchdog(void);
static void app_init_log_reset_information(uint32_t reset_reason);

void app_init(void)
{
  uint32_t reset_reason = RMU_ResetCauseGet();
  RMU_ResetCauseClear();
  app_init_watchdog();
  app_init_log_reset_information(reset_reason);

#if SL_DEVICE_INIT_DCDC_ENABLE && SL_DEVICE_INIT_DCDC_BYPASS != SYS_CNF_DCDC_BYPASS_ENABLE
  EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_DEFAULT;
  dcdcInit.mode = SYS_CNF_DCDC_BYPASS_ENABLE ? emuDcdcMode_Bypass : emuDcdcMode_Regulation;
  EMU_DCDCInit(&dcdcInit);
#endif

#if DEBUG
  sl_cli_init();
#endif
  sl_remote_app_init(EMU_RSTCAUSE_EM4 != reset_reason);
}

void app_process_action(void)
{
  WDOGn_Feed(DEFAULT_WDOG);
  sl_remote_app_cyclic();
}

static void app_init_watchdog(void)
{
  const WDOG_Init_TypeDef wdog_init = WDOG_INIT_DEFAULT;
  CMU_ClockEnable(cmuClock_WDOG0, true);
  WDOGn_Init(DEFAULT_WDOG, &wdog_init);
}

static void app_init_log_reset_information(uint32_t reset_reason)
{
  BootloaderInformation_t btl_info = { 0 };

  bootloader_getInfo(&btl_info);
  sl_log_warning("Transmitter application boot!" SL_LOG_EOL
                 " Reset reason: 0x%lX" SL_LOG_EOL
                 " Bootloader version: 0x%lX" SL_LOG_EOL
                 " Application version: 0x%X" SL_LOG_EOL
                 " Application type: " APP_TYPE SL_LOG_EOL
                 " Build time: " SL_BUILD_TIME_STRING SL_LOG_EOL,
                 reset_reason, btl_info.version, SL_APPLICATION_VERSION);
}
