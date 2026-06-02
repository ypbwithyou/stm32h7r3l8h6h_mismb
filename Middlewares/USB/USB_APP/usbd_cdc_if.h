/**
 ****************************************************************************************************
 * @file        usbd_cdc_if.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       USB CDC应用代码
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

#ifndef __USBD_CDC_IF_H
#define __USBD_CDC_IF_H

#include "usbd_cdc.h"
#include "slidingWindowReceiver_c.h"

/* 缓冲区大小定义 */
#define USBD_CDC_RX_BUF_SIZE        2000
#define USBD_CDC_PRINTF_BUF_SIZE    200
#define USBD_CDC_TX_TIMROUT         10

/* 导出相关变量 */
extern USBD_CDC_ItfTypeDef USBD_CDC_fops;
extern uint16_t g_usbd_cdc_rx_sta;
extern uint16_t g_usbd_cdc_rx_len;
extern uint8_t g_usbd_cdc_rx_buffer[USBD_CDC_RX_BUF_SIZE];
extern SlidingWindowReceiver_C g_slidingWindow_receiver;

/* 函数声明 */
void usbd_cdc_transmit(uint8_t *data, uint32_t len);
void usb_printf(char *fmt, ...);

#endif
