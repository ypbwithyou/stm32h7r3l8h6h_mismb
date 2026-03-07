/******************************************************************************
  Copyright (c) 2019-2021  (Xi'an) Testing & Control Technologies Co., Ltd.
  All rights reserved.

  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to 
    any third party.
  ------------------------------------------------------------------------

  *  Filename:      base_calculation.c
  *
  *  Description:   
  * 
  *  Author:
  *
  *  $Revision:     1.0 $
  *
*******************************************************************************/


/******************************************************************************
* Include files.
******************************************************************************/
#include <stdio.h>

#include "base_calculation.h"

/****************************************************************************** 
* Macros. 
******************************************************************************/ 

/******************************************************************************
* Variables.
******************************************************************************/

/******************************************************************************
* Functions.
******************************************************************************/
// 获取float型数组最大值
float float_array_max(int n, float array[])
{
    int i;
    float max = array[0];
    for (i=0; i<n; i++) {
        if (array[i] > max) {
            max = array[i];
        }
    }
    return max;
}
// 获取float型数组最小值
float float_array_min(int n, float array[])
{
    int i;
    float min = array[0];
    for (i=0; i<n; i++) {
        if (array[i] < min) {
            min = array[i];
        }
    }
    return min;
}
// 获取int型数组最大值
int int_array_max(int n, int array[])
{
    int i;
    int max = array[0];
    for (i=0; i<n; i++) {
        if (array[i] > max) {
            max = array[i];
        }
    }
    return max;
}
// 获取int型数组最小值
int int_array_min(int n, int array[])
{
    int i;
    int min = array[0];
    for (i=0; i<n; i++) {
        if (array[i] < min) {
            min = array[i];
        }
    }
    return min;
}
// 计算int型数据中1的位数
int count_one_bits(unsigned int value)
{
	int count = 0;
	int i = 0;
	for (i = 0; i < 32; i++)
	{
		if (1 == (1 & (value>>i)))
			count++;
	}
	return count; // 返回1的位数 
}
