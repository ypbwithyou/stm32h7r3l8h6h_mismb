/******************************************************************************
  Copyright (c) 2019-2021  (Xi'an) Testing & Control Technologies Co., Ltd.
  All rights reserved.
 
  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to 
    any third party.
  ------------------------------------------------------------------------
 
  *  Filename:      frame_reply.c
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
#include "frame_reply.h"
#include "protocol_str.h"
 
/******************************************************************************
* Macros.
******************************************************************************/
 
/******************************************************************************
* Variables.
******************************************************************************/
 
/******************************************************************************
* Functions.
******************************************************************************/
FrameReply* CreatFrameReply(void)
{
    FrameReply* data_reply = (FrameReply*)calloc(1, sizeof(FrameReply));
    if (NULL == data_reply)
    {
        return NULL;
    }
 
    data_reply->frame_user_encrypt = NULL;    
    data_reply->frame_user_decode = NULL;
    data_reply->frame_user_data = NULL;
 
    return data_reply;
}
 
void FreeFrameReply(FrameReply** data_ptr)
{
    if (NULL == *data_ptr)
    {
        return;
    }
    FrameReply* ptr = *data_ptr;

    if (NULL != ptr->frame_user_data)
    {
        // free(ptr->frame_user_data);
        FreeProtocolStr(&ptr->frame_user_data);
        ptr->frame_user_data = NULL;
    }
    if ((NULL == ptr->frame_user_decode) && (NULL == ptr->frame_user_encrypt)) {
        free(ptr);
        *data_ptr = NULL;
        return;
    }
    else if ((ptr->frame_user_decode == ptr->frame_user_encrypt) && (NULL != ptr->frame_user_encrypt)) {
        FreeProtocolStr(&ptr->frame_user_encrypt);
        ptr->frame_user_encrypt = NULL;
        ptr->frame_user_decode = NULL;
    }
    else if ((NULL != ptr->frame_user_decode) && (NULL != ptr->frame_user_encrypt) && (ptr->frame_user_decode != ptr->frame_user_encrypt)) {
        FreeProtocolStr(&ptr->frame_user_encrypt);
        ptr->frame_user_encrypt = NULL;
        FreeProtocolStr(&ptr->frame_user_decode);
        ptr->frame_user_decode = NULL;
    }
    else if ((NULL != ptr->frame_user_decode) && (NULL == ptr->frame_user_encrypt)) {
        FreeProtocolStr(&ptr->frame_user_decode);
        ptr->frame_user_decode = NULL;
    }
    else if((NULL == ptr->frame_user_decode) && (NULL != ptr->frame_user_encrypt)) {
        FreeProtocolStr(&ptr->frame_user_encrypt);
        ptr->frame_user_encrypt = NULL;
    }
    free(ptr);
    *data_ptr = NULL;
}