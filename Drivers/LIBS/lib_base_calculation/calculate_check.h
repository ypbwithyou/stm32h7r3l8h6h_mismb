/******************************************************************************
  Copyright (c) 2023-2025  (Xi'an) Testing & Control Technologies Co., Ltd.
  All rights reserved.

  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to 
    any third party.
  ------------------------------------------------------------------------

  *  Filename:      calculate_check.h
  *
  *  Description:   
  * 
  *  Author:        iris.zang
  *
  *  $Revision:     1.0 $
  *
*******************************************************************************/
#ifndef MCU_LIB_CALCULATE_CHECK_H 
#define MCU_LIB_CALCULATE_CHECK_H 

/******************************************************************************
* Include files.
******************************************************************************/
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/******************************************************************************
* Macros.
******************************************************************************/

/******************************************************************************
* Variables.
******************************************************************************/

/******************************************************************************
* Functions.
******************************************************************************/
unsigned int make_crc32(unsigned char * buf, unsigned int len);
unsigned short CheckCrc16_HL(unsigned char * buf, unsigned short len);

unsigned char CheckSum8(const char* buf, unsigned int len);
unsigned short CheckSum16(const char* buf, unsigned int len);
unsigned int CheckSum32(const char* buf, unsigned int len);

unsigned int CheckOddParity(const char* buf, unsigned int len);
unsigned int CheckEvenParity(const char* buf, unsigned int len);

#ifdef __cplusplus
}
#endif

#endif // MCU_LIB_CALCULATE_CHECK_H