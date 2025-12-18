/***************************************************************************//**
 * @file sl_log.h
 * @brief Logging utility that provides multi-level logging capabilities.
 *
 * This header file defines a logging system with five severity levels.
 * The logging functionality can be conditionally compiled based
 * on the defined log level, allowing for
 * different verbosity in debug vs. production builds.
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
#ifndef SL_LOG_H_
#define SL_LOG_H_
#ifdef __cplusplus
extern "C" {
#endif
#include <stdarg.h>

#define SL_LOG_EOL "\r\n"
#ifndef SL_LOG_MODULE_NAME
  #define SLI_LOG_MODULE_NAME ""
#else
  #define SLI_LOG_MODULE_NAME "[" SL_LOG_MODULE_NAME "]: "
#endif

#define SL_LOG_LEVEL_CRITICAL              1
#define SL_LOG_LEVEL_ERROR                 2
#define SL_LOG_LEVEL_WARNING               3
#define SL_LOG_LEVEL_INFO                  4
#define SL_LOG_LEVEL_DEBUG                 5

#if DEBUG && !defined(SL_LOG_LEVEL)
  #define SL_LOG_LEVEL                   SL_LOG_LEVEL_DEBUG
#elif !defined(SL_LOG_LEVEL)
  #define SL_LOG_LEVEL                   SL_LOG_LEVEL_WARNING
#endif

#define sli_log(...)                       sli_internal_log(SLI_LOG_MODULE_NAME __VA_ARGS__)
#define sli_log_status(sc, ...)            if (sc) sli_log(__VA_ARGS__)
#if SL_LOG_LEVEL >= SL_LOG_LEVEL_CRITICAL
  #define sl_log_critical(...)             sli_log(__VA_ARGS__)
  #define sl_log_status_critical(sc, ...)  sli_log_status(sc, __VA_ARGS__)
#else
  #define sl_log_critical(...)             sl_log_none(__VA_ARGS__)
  #define sl_log_status_critical(sc, ...)  (void)(sc); sl_log_none(__VA_ARGS__)
#endif

#if SL_LOG_LEVEL >= SL_LOG_LEVEL_ERROR
  #define sl_log_error(...)                sli_log(__VA_ARGS__)
  #define sl_log_status_error(sc, ...)     sli_log_status(sc, __VA_ARGS__)
#else
  #define sl_log_error(...)                sl_log_none(__VA_ARGS__)
  #define sl_log_status_error(sc, ...)     (void)(sc); sl_log_none(__VA_ARGS__)
#endif

#if SL_LOG_LEVEL >= SL_LOG_LEVEL_WARNING
  #define sl_log_warning(...)              sli_log(__VA_ARGS__)
  #define sl_log_status_warning(sc, ...)   sli_log_status(sc, __VA_ARGS__)
#else
  #define sl_log_warning(...)              sl_log_none(__VA_ARGS__)
  #define sl_log_status_warning(sc, ...)   (void)(sc); sl_log_none(__VA_ARGS__)
#endif

#if SL_LOG_LEVEL >= SL_LOG_LEVEL_INFO
  #define sl_log_info(...)                 sli_log(__VA_ARGS__)
  #define sl_log_status_info(sc, ...)      sli_log_status(sc, __VA_ARGS__)
#else
  #define sl_log_info(...)                 sl_log_none(__VA_ARGS__)
  #define sl_log_status_info(sc, ...)      (void)(sc); sl_log_none(__VA_ARGS__)
#endif

#if SL_LOG_LEVEL >= SL_LOG_LEVEL_DEBUG
  #define sl_log_debug(...)                sli_log(__VA_ARGS__)
  #define sl_log_status_debug(sc, ...)     sli_log_status(sc, __VA_ARGS__)
#else
  #define sl_log_debug(...)                sl_log_none(__VA_ARGS__)
  #define sl_log_status_debug(sc, ...)     (void)(sc); sl_log_none(__VA_ARGS__)
#endif

//Function to avoid unused variable warnings due to different log levels
static inline void sl_log_none(const char* format, ...)
{
  (void)format;
}

/**
 * @brief Internal logging function.
 *
 * @details This is an internal function intended for use only within this module.
 * It should not be called or accessed by other modules as it is not part of the public API.
 */
int sli_internal_log(const char* format, ...)
#ifdef __GNUC__
__attribute__ ((format(printf, 1, 2)))
#endif
;

#ifdef __cplusplus
}
#endif
#endif /* SL_LOG_H_ */
