/******************************************************************************
  Copyright (c) 2018-2021  (Xi'an) Testing & Control Technologies Co., Ltd.
  All rights reserved.
  
  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to any 
    third party.
  ------------------------------------------------------------------------
  文件名称：frame_analys.h
  文件标识：数据加密、解密、CRC校验
  摘    要：
  当前版本：1.0
  作    者：
  创建日期：2019年03月5日
  修改记录：
*******************************************************************************/
#ifndef LIB_PROTOCOL_INCLUDE_FRAME_ANALYS_H_
#define LIB_PROTOCOL_INCLUDE_FRAME_ANALYS_H_

/******************************************************************************
* Include files.
******************************************************************************/
#include <stdint.h>
#include "protocol.h"

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
int DataDecode(uint32_t type, ProtocolStr* data_in, ProtocolStr** data_out);
int DataEncode(uint32_t type, ProtocolStr* data_in, ProtocolStr** data_out);
int DataCrc(ProtocolStr* data_frame);

#ifdef __cplusplus
}
#endif

#endif // LIB_PROTOCOL_INCLUDE_FRAME_ANALYS_H_