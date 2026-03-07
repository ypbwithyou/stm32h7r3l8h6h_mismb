/******************************************************************************
  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to any 
    third party.
  ------------------------------------------------------------------------
  文件名称：dev_usbhs.h
  文件标识：485协议处理
  摘    要：
  当前版本：1.0
  作    者：
  创建日期：2025年12月25日
  修改记录：
*******************************************************************************/
#ifndef __DEV_USBHS_H
#define __DEV_USBHS_H

#include <stdint.h>
#include "usbd_cdc_if.h"
#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"

/*FreeRTOS*********************************************************************************************/


/******************************************************************************************************/

void USBHsTask(void *pvParameters);

#endif // __DEV_USBHS_H

