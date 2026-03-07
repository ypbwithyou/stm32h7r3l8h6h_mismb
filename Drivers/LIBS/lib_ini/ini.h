/******************************************************************************
  Copyright (c) 2018-2021  (Xi'an) Testing & Control Technologies Co., Ltd.
  All rights reserved.
  
  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to any 
    third party.
  ------------------------------------------------------------------------
  文件名称: ini.h
  文件标识: 
  摘    要: 
  当前版本: 1.0
  作    者: Iris.Zang
  完成日期: 2025年12月25日
*******************************************************************************/
#ifndef MCU_LIB_INI_INCLUDE_INI_H_
#define MCU_LIB_INI_INCLUDE_INI_H_

#ifdef __cplusplus
extern "C" {
#endif

#define INI_RET_OK              (0)
#define INI_RET_ERROR           (-1)

char* StrTrim(char* str);

int LoadConfFile(const char* file);
int GetSecStr(const char* sector, const char* key, char* value);
int SetSecStr(const char* sector, const char* key, char* value);
int GetSecInt(const char* sector, const char* key, int* value);
int SetSecInt(const char* sector, const char* key, int* value);
int GetSecFloat(const char* sector, const char* key, float* value);
int SetSecFloat(const char* sector, const char* key, float* value);
int GetSecDouble(const char* sector, const char* key, double* value);
int SetSecDouble(const char* sector, const char* key, double* value);

int GetThresholdStr(const char* dir_name, const char* file_name, char* value);
int SetThresholdStr(const char* dir_name, const char* file_name, char* value);
int GetThresholdInt(const char* dir_name, const char* file_name, int* value);
int SetThresholdInt(const char* dir_name, const char* file_name, int* value);
int GetThresholdFloat(const char* dir_name, const char* file_name, float* value);
int SetThresholdFloat(const char* dir_name, const char* file_name, float* value);
int GetThresholdDouble(const char* dir_name, const char* file_name, double* value);
int SetThresholdDouble(const char* dir_name, const char* file_name, double* value);

#ifdef __cplusplus
}
#endif

#endif // MCU_LIB_INI_INCLUDE_INI_H_
