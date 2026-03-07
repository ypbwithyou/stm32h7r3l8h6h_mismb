/******************************************************************************
  Copyright (c) 2019-2021  (Xi'an) Testing & Control Technologies Co., Ltd.
  All rights reserved.

  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to 
    any third party.
  ------------------------------------------------------------------------

  *  Filename:      protocol.c
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
#include <stdlib.h>
#include <string.h>

#include "protocol.h"
#include "protocol_str.h"
#include "./LIBS/lib_link_list/link_list.h"
//#include "log.h"

/******************************************************************************
* Macros.
******************************************************************************/

/******************************************************************************
* Variables.
******************************************************************************/

/******************************************************************************
* Functions.
******************************************************************************/
ProtocolStr* CreatProtocolStr(int len)
{
    char* data = NULL;
    if (len < 0) {
        return NULL;
    }

    ProtocolStr* data_ptr = calloc(1, sizeof(ProtocolStr));
    if (NULL == data_ptr)
    {
        free(data_ptr);
        return NULL;
    }
    
    if (len > 0) {
        data = (char *)calloc(1, len); 
        if (NULL == data)
        {
            free(data);
            free(data_ptr);
            return NULL;
        }        
    }

    data_ptr->data = data;
    data_ptr->data_len = len;
    data_ptr->data_index = 0;
    return data_ptr;
}

int AppendProtocolStr(ProtocolStr* data_ptr, char* append_data, int append_len)
{
    if (NULL == data_ptr || NULL == append_data || data_ptr->data_len - data_ptr->data_index < append_len)
    {
        return RET_ERROR;
    }
    memcpy((char *)data_ptr->data + data_ptr->data_index, append_data, append_len);
    data_ptr->data_index += append_len;
    return RET_OK;
}

void FreeProtocolStr(ProtocolStr** data_ptr)
{
    if (NULL == *data_ptr)
    {
        return;     
    }
    ProtocolStr* ptr = *data_ptr;
    if (NULL != ptr->data)
    {
        free(ptr->data);
        ptr->data = NULL;
    }
    ptr->data_index = 0;
    ptr->data_len = 0;
    free(ptr);
    *data_ptr = NULL;
}

/*将链表转换成带长度的字符串结构体*/
ProtocolStr* GetProtocolStrFromLinkList(LinkList* l_list)
{
    int list_len = LinkListLength(l_list);
    // LOG_DEBUG("list_len:%u", list_len);
    ProtocolStr* data = CreatProtocolStr(list_len);
    ListNode* node = l_list->head;
    for(int i =0; i < l_list->node_num; ++i)
    {
        if (RET_ERROR == AppendProtocolStr(data, node->data, node->data_len))
        {
            // TODO:打印错误日志
            FreeProtocolStr(&data);
            return NULL;
        }
        node = node->next;
    }
    return data;
}
