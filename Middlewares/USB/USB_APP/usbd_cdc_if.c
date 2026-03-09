/**
 ****************************************************************************************************
 * @file        usbd_cdc_if.c
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

#include "usbd_cdc_if.h"
#include "./SYSTEM/delay/delay.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "slidingWindowReceiver_c.h"

/* USBD句柄 */
extern USBD_HandleTypeDef g_usbd_handle;

static uint8_t g_usbd_cdc_tx_done = 1;
uint16_t g_usbd_cdc_rx_sta = 0;
uint16_t g_usbd_cdc_rx_len = 0;
uint8_t g_usbd_cdc_rx_buffer[USBD_CDC_RX_BUF_SIZE];
static uint8_t g_usbd_cdc_printf_buffer[USBD_CDC_PRINTF_BUF_SIZE];
SlidingWindowReceiver_C g_slidingWindow_receiver;

USBD_CDC_LineCodingTypeDef linecoding = {
    115200, /* baud rate*/
    0x00,   /* stop bits-1*/
    0x00,   /* parity - none*/
    0x08    /* nb. of bits 8*/
};

/**
 * @brief   初始化CDC设备
 * @param   无
 * @retval  操作结果
 * @arg     USBD_OK: 成功
 * @arg     USBD_FAIL: 失败
 */
static int8_t CDC_Init(void)
{
    USBD_CDC_SetRxBuffer(&g_usbd_handle, g_usbd_cdc_rx_buffer);
    SWR_Init(&g_slidingWindow_receiver, NULL, NULL);

    return USBD_OK;
}

/**
 * @brief   反初始化CDC设备
 * @param   无
 * @retval  操作结果
 * @arg     USBD_OK: 成功
 * @arg     USBD_FAIL: 失败
 */
static int8_t CDC_DeInit(void)
{
    return USBD_OK;
}

/**
 * @brief   管理CDC请求
 * @param   cmd: 命令
 * @param   pbuf: 数据
 * @param   length: 数据长度
 * @retval  操作结果
 * @arg     USBD_OK: 成功
 * @arg     USBD_FAIL: 失败
 */
static int8_t CDC_Control(uint8_t cmd, uint8_t *pbuf, uint16_t length)
{
    UNUSED(length);

    switch (cmd)
    {
    case CDC_SEND_ENCAPSULATED_COMMAND:
    case CDC_GET_ENCAPSULATED_RESPONSE:
    case CDC_SET_COMM_FEATURE:
    case CDC_GET_COMM_FEATURE:
    case CDC_CLEAR_COMM_FEATURE:
    case CDC_SET_CONTROL_LINE_STATE:
    case CDC_SEND_BREAK:
    default:
    {
        break;
    }
    case CDC_SET_LINE_CODING:
    {
        linecoding.bitrate = (uint32_t)(pbuf[0] | (pbuf[1] << 8) | (pbuf[2] << 16) | (pbuf[3] << 24));
        linecoding.format = pbuf[4];
        linecoding.paritytype = pbuf[5];
        linecoding.datatype = pbuf[6];
        break;
    }
    case CDC_GET_LINE_CODING:
    {
        pbuf[0] = (uint8_t)(linecoding.bitrate);
        pbuf[1] = (uint8_t)(linecoding.bitrate >> 8);
        pbuf[2] = (uint8_t)(linecoding.bitrate >> 16);
        pbuf[3] = (uint8_t)(linecoding.bitrate >> 24);
        pbuf[4] = linecoding.format;
        pbuf[5] = linecoding.paritytype;
        pbuf[6] = linecoding.datatype;
        break;
    }
    }

    return USBD_OK;
}

/**
 * @brief   CDC接收
 * @param   Buf: 数据
 * @param   Len: 数据长度
 * @retval  操作结果
 * @arg     USBD_OK: 成功
 * @arg     USBD_FAIL: 失败
 */
static int8_t CDC_Receive(uint8_t *Buf, uint32_t *Len)
{
    // uint32_t i;

    USBD_CDC_ReceivePacket(&g_usbd_handle);

    SWR_ProcessBytes(&g_slidingWindow_receiver, Buf, *Len);
    // usb_printf("CDC_Receive: %d\n", *Len);
    // usbd_cdc_transmit(Buf, *Len);

    // g_usbd_cdc_rx_len = *Len;

    // if (g_usbd_cdc_rx_len > 0)
    // {
    //     if (g_usbd_cdc_rx_len > USBD_CDC_RX_BUF_SIZE)
    //         g_usbd_cdc_rx_len = USBD_CDC_RX_BUF_SIZE;

    //     memcpy(g_usbd_cdc_tx_buffer, Buf, *Len);

    //     g_usbd_cdc_rx_sta = 1;
    // }

    // for (i = 0; i < *Len; i++)
    // {
    //     if ((g_usbd_cdc_rx_sta & (1 << 15)) != (1 << 15))
    //     {
    //         if ((g_usbd_cdc_rx_sta & (1 << 14)) != (1 << 14))
    //         {
    //             if (Buf[i] == 0x0D)
    //             {
    //                 g_usbd_cdc_rx_sta |= (1 << 14);
    //             }
    //             else
    //             {
    //                 if (++g_usbd_cdc_rx_sta > (USBD_CDC_RX_BUF_SIZE - 1))
    //                 {
    //                     g_usbd_cdc_rx_sta = 0;
    //                 }
    //             }
    //         }
    //         else
    //         {
    //             if (Buf[i] != 0x0A)
    //             {
    //                 g_usbd_cdc_rx_sta = 0;
    //             }
    //             else
    //             {
    //                 g_usbd_cdc_rx_sta |= (1 << 15);
    //             }
    //         }
    //     }
    // }

    return USBD_OK;
}

/**
 * @brief   CDC发送完成
 * @param   Buf: 数据
 * @param   Len: 数据长度
 * @param   epnum: 端点编号
 * @retval  操作结果
 * @arg     USBD_OK: 成功
 * @arg     USBD_FAIL: 失败
 */
static int8_t CDC_TransmitCplt(uint8_t *Buf, uint32_t *Len, uint8_t epnum)
{
    UNUSED(Buf);
    UNUSED(Len);
    UNUSED(epnum);

    g_usbd_cdc_tx_done = 1;

    return USBD_OK;
}

/* CDC操作函数集合 */
USBD_CDC_ItfTypeDef USBD_CDC_fops =
    {
        CDC_Init,
        CDC_DeInit,
        CDC_Control,
        CDC_Receive,
        CDC_TransmitCplt};

void usbd_cdc_transmit(uint8_t *data, uint32_t len)
{
    uint8_t timeout = USBD_CDC_TX_TIMROUT;
   
    g_usbd_cdc_tx_done = 0;
    USBD_CDC_SetTxBuffer(&g_usbd_handle, data, len);
    USBD_CDC_TransmitPacket(&g_usbd_handle);
    while ((g_usbd_cdc_tx_done != 1) && (--timeout != 0))
    {
        delay_ms(1);
    }
}

void usb_printf(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsprintf((char *)g_usbd_cdc_printf_buffer, fmt, ap);
    va_end(ap);
    usbd_cdc_transmit(g_usbd_cdc_printf_buffer, strlen((const char *)g_usbd_cdc_printf_buffer));
}
