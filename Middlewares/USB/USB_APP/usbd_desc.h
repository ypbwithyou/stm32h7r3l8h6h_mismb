/**
 ****************************************************************************************************
 * @file        usbd_conf.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       USB设备描述符配置文件
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 * 
 * 实验平台:正点原子 H7R3开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 * 
 ****************************************************************************************************
 */

#ifndef __USBD_DESC_H
#define __USBD_DESC_H

#include "usbd_def.h"

#define DEVICE_ID1              (UID_BASE)
#define DEVICE_ID2              (UID_BASE + 0x4)
#define DEVICE_ID3              (UID_BASE + 0x8)

#define USB_SIZ_STRING_SERIAL   0x1AU

/* 导出USB复合设备描述符 */
extern USBD_DescriptorsTypeDef Composite_Desc;

#endif
