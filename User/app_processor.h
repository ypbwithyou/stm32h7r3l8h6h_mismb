/******************************************************************************
  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to any 
    third party.
  ------------------------------------------------------------------------
  文件名称：app_processor.h
  文件标识：核心处理模块
  摘    要：
  当前版本：1.0
  作    者：
  创建日期：2025年12月25日
  修改记录：
*******************************************************************************/

#ifndef __APP_PROCESSOR_H
#define __APP_PROCESSOR_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* 函数声明 */
void ProcessorTask(void *pvParameters);
BaseType_t adc_send_data_from_isr(uint8_t spi_num, uint8_t adc_num, uint16_t data);

#endif /* __APP_PROCESSOR_H */