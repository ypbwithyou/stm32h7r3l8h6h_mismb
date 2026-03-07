/******************************************************************************
  Copyright (c) 2018-2021  (Xi'an) Testing & Control Technologies Co., Ltd.
  All rights reserved.
  
  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to any 
    third party.
  ------------------------------------------------------------------------
  文件名称：proctocol_str.h
  文件标识：协议处理
  摘    要：
  当前版本：1.0
  作    者：
  创建日期：2019年02月23日
  修改记录：
*******************************************************************************/
#ifndef LIB_PROTOCOL_INCLUDE_PROTOCOL_STR_H_
#define LIB_PROTOCOL_INCLUDE_PROTOCOL_STR_H_

/******************************************************************************
* Include files.
******************************************************************************/
#include "protocol.h"
#include "./LIBS/lib_link_list/link_list.h"

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
ProtocolStr* CreatProtocolStr(int len);
int AppendProtocolStr(ProtocolStr* data_ptr, char* append_data, int append_len);
void FreeProtocolStr(ProtocolStr** data_ptr);
ProtocolStr* GetProtocolStrFromLinkList(LinkList* l_list);
//void FreeProtocolStr(ProtocolStr** ptr);

ProtocolStr* GetStringFromList(LinkList *data_list, int offset_bytes, int len_bytes);
void PopStringFromList(LinkList *data, int data_len);

#ifdef __cplusplus
}
#endif

#endif // LIB_PROTOCOL_INCLUDE_PROTOCOL_STR_H_

