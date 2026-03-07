/**
 ****************************************************************************************************
 * @file        rs485.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       RS485驱动代码
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

#ifndef __RS485_H
#define __RS485_H

#include "./SYSTEM/sys/sys.h"

/****************************************************************************************************************/
/* 引脚定义和串口定义 */

#define RS485_RE_GPIO_PORT                  GPIOF
#define RS485_RE_GPIO_PIN                   GPIO_PIN_11
#define RS485_RE_GPIO_CLK_ENABLE()          do{ __HAL_RCC_GPIOF_CLK_ENABLE(); }while(0)     /* PF口时钟使能 */

#define RS485_UARTX                         USART2
#define RS485_UARTX_IRQn                    USART2_IRQn
#define RS485_UARTX_IRQHandler              USART2_IRQHandler
#define RS485_UARTX_CLK_ENABLE()            do { __HAL_RCC_USART2_CLK_ENABLE(); } while (0) /* USART2时钟使能 */

#define RS485_UARTX_TX_GPIO_PORT            GPIOD
#define RS485_UARTX_TX_GPIO_PIN             GPIO_PIN_6
#define RS485_UARTX_TX_GPIO_AF              GPIO_AF7_USART2
#define RS485_UARTX_TX_GPIO_CLK_ENABLE()    do { __HAL_RCC_GPIOD_CLK_ENABLE(); } while (0)  /* PD口时钟使能 */

#define RS485_UARTX_RX_GPIO_PORT            GPIOD
#define RS485_UARTX_RX_GPIO_PIN             GPIO_PIN_5
#define RS485_UARTX_RX_GPIO_AF              GPIO_AF7_USART2
#define RS485_UARTX_RX_GPIO_CLK_ENABLE()    do { __HAL_RCC_GPIOD_CLK_ENABLE(); } while (0)  /* PD口时钟使能 */

/* 控制RS485_RE脚, 控制RS485发送/接收状态
 * RS485_RE = 0, 进入接收模式
 * RS485_RE = 1, 进入发送模式
 */
#define RS485_RE(x)   do{ x ? \
                          HAL_GPIO_WritePin(RS485_RE_GPIO_PORT, RS485_RE_GPIO_PIN, GPIO_PIN_SET) : \
                          HAL_GPIO_WritePin(RS485_RE_GPIO_PORT, RS485_RE_GPIO_PIN, GPIO_PIN_RESET); \
                      }while(0)

/* 接收缓冲区相关定义 */
#define RS485_RX_BUF_LEN                    1024
                      
extern uint8_t g_RS485_rx_buf[RS485_RX_BUF_LEN];    /* 接收缓冲,最大RS485_REC_LEN个字节 */
extern uint8_t g_RS485_rx_cnt;                      /* 接收数据长度 */

/* 帧格式定义 */
#define RS485_FRAME_HEAD        0x3C            /* 帧头 */
#define RS485_FRAME_END         0x3E            /* 帧尾 */

/* 接收状态机定义 */
typedef enum {
    RX_STATE_IDLE = 0,          /* 空闲状态，等待帧头 */
    RX_STATE_HEADER,            /* 已收到帧头 */
    RX_STATE_RECEIVING,         /* 正在接收数据 */
    RX_STATE_FOOTER             /* 已收到帧尾 */
} rs485_rx_state_t;

/****************************************************************************************************************/
/* 函数声明 */
void rs485_init(uint32_t baudrate);                 /* 初始化RS485 */
void rs485_send_data(uint8_t *buf, uint8_t len);    /* RS485发送数据 */
int8_t rs485_recv_data(uint8_t *buf, uint8_t *len);   /* RS485接收数据 */

uint8_t rs485_get_pending_frames(void);

#endif
