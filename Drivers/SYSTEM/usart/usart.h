/**
 ****************************************************************************************************
 * @file        usart.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       串口初始化代码
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

#ifndef __USART_H
#define __USART_H

#include "./SYSTEM/sys/sys.h"
#include <stdio.h>

/* 硬件定义 */
#define USART_TX_GPIO_PORT          GPIOB
#define USART_TX_GPIO_PIN           GPIO_PIN_14
#define USART_TX_GPIO_AF            GPIO_AF4_USART1
#define USART_TX_GPIO_CLK_ENABLE()  do { __HAL_RCC_GPIOB_CLK_ENABLE(); } while(0)
#define USART_RX_GPIO_PORT          GPIOB
#define USART_RX_GPIO_PIN           GPIO_PIN_15
#define USART_RX_GPIO_AF            GPIO_AF4_USART1
#define USART_RX_GPIO_CLK_ENABLE()  do { __HAL_RCC_GPIOB_CLK_ENABLE(); } while(0)
#define USART_UX                    USART1
#define USART_UX_IRQn               USART1_IRQn
#define USART_UX_IRQHandler         USART1_IRQHandler
#define USART_UX_CLK_ENABLE()       do { __HAL_RCC_USART1_CLK_ENABLE(); } while(0)

/* 功能定义 */
#define USART_EN_RX                 1           /* 使能串口接收功能 */
#define RXBUFFERSIZE                1           /* 串口中断接收缓冲区大小 */
#define USART_REC_LEN               200         /* 串口接收缓冲区大小 */

/* 变量导出 */
extern UART_HandleTypeDef g_uart1_handle;       /* UART句柄 */
#if USART_EN_RX
extern uint8_t g_rx_buffer[RXBUFFERSIZE];       /* 串口中断接收缓冲区 */
extern uint8_t g_usart_rx_buf[USART_REC_LEN];   /* 串口接收缓冲区 */
extern uint16_t g_usart_rx_sta;                 /* 串口接收状态标记 */
#endif

/* 函数声明 */
void usart_init(uint32_t baudrate);             /* 初始化串口 */

#endif
