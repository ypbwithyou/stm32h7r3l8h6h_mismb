/******************************************************************************
  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to any 
    third party.
  ------------------------------------------------------------------------
  文件名称：rs485_processor.h
  文件标识：485协议处理
  摘    要：
  当前版本：1.0
  作    者：
  创建日期：2025年12月25日
  修改记录：
*******************************************************************************/
#ifndef __RS485_PROCESSOR_H
#define __RS485_PROCESSOR_H

#include <stdint.h>
#include "app_common.h"

#define RS485_BAUDRATE          (115200)

enum Event
{
    DEVINFO_READ_REQ        = 0x00,     // 读取子卡设备信息请求
    DEVINFO_READ_REQ_ACK    = 0x01,     // 读取子卡设备信息响应
    DEVINFO_WRITE_REQ       = 0x02,     // 写入子卡设备信息请求
    DEVINFO_WRITE_REQ_ACK   = 0x03,     // 写入子卡设备信息响应
};

/* 本地函数声明 */
void rs485_parse_frame(uint8_t *data, uint8_t len);
int8_t rs485_send_frame(uint8_t dev_num, uint8_t cmd, uint8_t *data, uint16_t len);

#endif // __RS485_PROCESSOR_H
