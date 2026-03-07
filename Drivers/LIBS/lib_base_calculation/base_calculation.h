/******************************************************************************
  Copyright (c) 2023-2025  (Xi'an) Testing & Control Technologies Co., Ltd.
  All rights reserved.

  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to 
    any third party.
  ------------------------------------------------------------------------

  *  Filename:      base_calculation.h
  *
  *  Description:   
  * 
  *  Author:        iris.zang
  *
  *  $Revision:     1.0 $
  *
*******************************************************************************/
#ifndef MCU_LIB_BASE_CALCULATION_H 
#define MCU_LIB_BASE_CALCULATION_H 

/******************************************************************************
* Include files.
******************************************************************************/

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
float float_array_max(int n, float array[]);
float float_array_min(int n, float array[]);
int int_array_max(int n, int array[]);
int int_array_min(int n, int array[]);
int count_one_bits(unsigned int value);

#ifdef __cplusplus
}
#endif

#endif // MCU_LIB_BASE_CALCULATION_H