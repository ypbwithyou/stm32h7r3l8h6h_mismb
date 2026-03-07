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

#include "protocol.h"
#include "frame_analys.h"

/******************************************************************************
* Macros.
******************************************************************************/
#define UN_USED(x)      (void)(x)
/******************************************************************************
* Variables.
******************************************************************************/

/******************************************************************************
* Functions.
******************************************************************************/
int DataDecode(uint32_t type, ProtocolStr* data_in, ProtocolStr** data_out)
{
    UN_USED(type);
    UN_USED(data_in);
    UN_USED(data_out);
    return RET_OK;
}
int DataEncode(uint32_t type, ProtocolStr* data_in, ProtocolStr** data_out)
{
    UN_USED(type);
    UN_USED(data_in);
    UN_USED(data_out);
    return RET_OK;
}

int DataCrc(ProtocolStr* data_frame)
{
    UN_USED(data_frame);
    return RET_OK;
}
