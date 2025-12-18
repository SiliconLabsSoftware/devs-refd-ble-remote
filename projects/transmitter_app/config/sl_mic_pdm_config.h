/***************************************************************************//**
 * @file
 * @brief MIC_PDM config
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
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

#ifndef SL_MIC_PDM_CONFIG_H
#define SL_MIC_PDM_CONFIG_H

// <<< Use Configuration Wizard in Context Menu
// <h> MIC PDM config
// <o> PDM down sampling rate <3-73>
// <i> Defines the ratio between PDM reference clock and the sampling frequency
#define SL_MIC_PDM_DSR           64
// </h> end MIC_PDM config
// <<< end of configuration section >>>

// <<< sl:start pin_tool >>>

// <pdm signal=DAT0,CLK> SL_MIC_PDM
// $[PDM_SL_MIC_PDM]
#ifndef SL_MIC_PDM_PERIPHERAL                   
#define SL_MIC_PDM_PERIPHERAL                    PDM
#endif

// PDM DAT0 on PC07
#ifndef SL_MIC_PDM_DAT0_PORT                    
#define SL_MIC_PDM_DAT0_PORT                     SL_GPIO_PORT_C
#endif
#ifndef SL_MIC_PDM_DAT0_PIN                     
#define SL_MIC_PDM_DAT0_PIN                      4
#endif

// PDM CLK on PC06
#ifndef SL_MIC_PDM_CLK_PORT                     
#define SL_MIC_PDM_CLK_PORT                      SL_GPIO_PORT_C
#endif
#ifndef SL_MIC_PDM_CLK_PIN                      
#define SL_MIC_PDM_CLK_PIN                       6
#endif
// [PDM_SL_MIC_PDM]$
// <<< sl:end pin_tool >>>

// FIXME
// To support any or multiple configurations, we need to define the macros this way.
// If these are removed after a Simplicity configuration it's still not an issue because
// sl_drv_mic.c will verify the macro correctness.
#include "sl_system_config.h"
#undef SL_MIC_PDM_DAT0_PORT
#define SL_MIC_PDM_DAT0_PORT SYS_CNF_MIC_IO_PDM_DATA_PORT
#undef SL_MIC_PDM_DAT0_PIN
#define SL_MIC_PDM_DAT0_PIN  SYS_CNF_MIC_IO_PDM_DATA_PIN
#undef SL_MIC_PDM_CLK_PORT
#define SL_MIC_PDM_CLK_PORT  SYS_CNF_MIC_IO_PDM_CLK_PORT
#undef SL_MIC_PDM_CLK_PIN
#define SL_MIC_PDM_CLK_PIN   SYS_CNF_MIC_IO_PDM_CLK_PIN

#endif // SL_MIC_PDM_CONFIG_H
