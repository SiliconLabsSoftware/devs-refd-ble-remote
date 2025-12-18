/***************************************************************************//**
 * @file sl_build_time.h
 * @brief
 * @version 1.0.0
 *******************************************************************************
 * # License
 * <b>Copyright 2022 Silicon Laboratories Inc. www.silabs.com</b>
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
#ifndef SL_BUILD_TIME_H_
#define SL_BUILD_TIME_H_
#ifdef __cplusplus
extern "C" {
#endif

//macros -----------------------------------------------------------------------
#define SL_BUILD_TIME_STRING __DATE__ " " __TIME__

#if (__GNUC__ >= 8 && __GNUC_MINOR__ > 1)
  #define SL_BUILD_TIME_YEAR ((__DATE__[7] - '0') * 1000 +  (__DATE__[8] - '0') * 100 + (__DATE__[9] - '0') * 10 + __DATE__[10] - '0')
// Very ugly but at least it will be evaluated at compile time.
// Note that the preprocessor cannot handle [] operator, so the obvious solution won't work
  #define SL_BUILD_TIME_MONTH                                         \
  (__DATE__[0] == 'J' && __DATE__[1] == 'a' && __DATE__[2] == 'n')    \
  ? 1                                                                 \
  : (__DATE__[0] == 'F' && __DATE__[1] == 'e' && __DATE__[2] == 'b')  \
  ? 2                                                                 \
  : (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'r')  \
  ? 3                                                                 \
  : (__DATE__[0] == 'A' && __DATE__[1] == 'p' && __DATE__[2] == 'r')  \
  ? 4                                                                 \
  : (__DATE__[0] == 'M' && __DATE__[1] == 'a' && __DATE__[2] == 'y')  \
  ? 5                                                                 \
  : (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'n')  \
  ? 6                                                                 \
  : (__DATE__[0] == 'J' && __DATE__[1] == 'u' && __DATE__[2] == 'l')  \
  ? 7                                                                 \
  : (__DATE__[0] == 'A' && __DATE__[1] == 'u' && __DATE__[2] == 'g')  \
  ? 8                                                                 \
  : (__DATE__[0] == 'S' && __DATE__[1] == 'e' && __DATE__[2] == 'p')  \
  ? 9                                                                 \
  :  (__DATE__[0] == 'O' && __DATE__[1] == 'c' && __DATE__[2] == 't') \
  ? 10                                                                \
  : (__DATE__[0] == 'N' && __DATE__[1] == 'o' && __DATE__[2] == 'v')  \
  ? 11                                                                \
  : 12

  #define SL_BUILD_TIME_DAY  (  (' ' == __DATE__[4])  \
                                ? (__DATE__[5] - '0') \
                                : (((__DATE__[4] - '0') * 10) + (__DATE__[5] - '0')) )
  #define SL_BUILD_TIME_HOUR ( ((__TIME__[0] - '0') * 10) + (__TIME__[1] - '0'))
  #define SL_BUILD_TIME_MIN  ( ((__TIME__[3] - '0') * 10) + (__TIME__[4] - '0'))
  #define SL_BUILD_TIME_SEC  ( ((__TIME__[6] - '0') * 10) + (__TIME__[7] - '0'))
#else
  #warning Update 'SL_BUILD_TIME*' macros manually!
  #define SL_BUILD_TIME_YEAR   2022
  #define SL_BUILD_TIME_MONTH  05
  #define SL_BUILD_TIME_DAY    17
  #define SL_BUILD_TIME_HOUR   0
  #define SL_BUILD_TIME_MIN    0
  #define SL_BUILD_TIME_SEC    0
#endif //(__GNUC__ >= 8 && __GNUC_MINOR__ > 1)

//type definitions -------------------------------------------------------------
//global variables -------------------------------------------------------------
//function prototypes ----------------------------------------------------------

#ifdef __cplusplus
}
#endif
#endif /* SL_BUILD_TIME_H_ */
