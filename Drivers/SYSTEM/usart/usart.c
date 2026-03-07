/**
 ****************************************************************************************************
 * @file        usart.c
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

#include "./SYSTEM/usart/usart.h"
#if SYS_SUPPORT_OS
#include "os.h"
#endif

int fputc(int c, FILE *stream)
{
    while (!(USART_UX->ISR & USART_ISR_TC));
    USART_UX->TDR = (uint8_t)c;
    
    return c;
}

/* UART句柄 */
UART_HandleTypeDef g_uart1_handle = {0};

#if USART_EN_RX
/* 串口中断接收缓冲区 */
uint8_t g_rx_buffer[RXBUFFERSIZE];
/* 串口接收缓冲区 */
uint8_t g_usart_rx_buf[USART_REC_LEN];
/* 串口接收状态标记 */
uint16_t g_usart_rx_sta = 0;
#endif

/**
 * @brief   初始化串口
 * @param   baudrate: 通信波特率（单位：bps）
 * @retval  无
 */
void usart_init(uint32_t baudrate)
{
    /* 初始化UART */
    g_uart1_handle.Instance = USART_UX;
    g_uart1_handle.Init.BaudRate = baudrate;
    g_uart1_handle.Init.WordLength = UART_WORDLENGTH_8B;
    g_uart1_handle.Init.StopBits = UART_STOPBITS_1;
    g_uart1_handle.Init.Parity = UART_PARITY_NONE;
#if USART_EN_RX
    g_uart1_handle.Init.Mode = UART_MODE_TX_RX;
#else
    g_uart1_handle.Init.Mode = UART_MODE_TX;
#endif
    g_uart1_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    g_uart1_handle.Init.OverSampling = UART_OVERSAMPLING_16;
    g_uart1_handle.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    g_uart1_handle.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    g_uart1_handle.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    HAL_UART_Init(&g_uart1_handle);
    
#if USART_EN_RX
    /* UART中断接收数据 */
    HAL_UART_Receive_IT(&g_uart1_handle, g_rx_buffer, sizeof(g_rx_buffer));
#endif
}

/**
 * @brief   HAL库UART初始化MSP函数
 * @param   huart: UART句柄指针
 * @retval  无
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    RCC_PeriphCLKInitTypeDef rcc_periph_clk_init_struct = {0};
    GPIO_InitTypeDef gpio_init_struct = {0};
    
    if (huart->Instance == USART_UX)
    {
        /* 配置时钟源 */
        rcc_periph_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_USART1;
        rcc_periph_clk_init_struct.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
        HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init_struct);
        
        /* 使能时钟 */
        USART_UX_CLK_ENABLE();
        USART_TX_GPIO_CLK_ENABLE();
#if USART_EN_RX
        USART_RX_GPIO_CLK_ENABLE();
#endif
        
        /* 初始化TX引脚 */
        gpio_init_struct.Pin = USART_TX_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init_struct.Alternate = USART_TX_GPIO_AF;
        HAL_GPIO_Init(USART_TX_GPIO_PORT, &gpio_init_struct);
        
#if USART_EN_RX
        /* 初始化RX引脚 */
        gpio_init_struct.Pin = USART_RX_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init_struct.Alternate = USART_RX_GPIO_AF;
        HAL_GPIO_Init(USART_RX_GPIO_PORT, &gpio_init_struct);
#endif
        
#if USART_EN_RX
        /* 配置中断 */
        HAL_NVIC_SetPriority(USART_UX_IRQn, 3, 0);
        HAL_NVIC_EnableIRQ(USART_UX_IRQn);
#endif
    }
}

///**
// * @brief   HAL库UART接收完成回调函数
// * @param   huart: UART句柄指针
// * @retval  无
// */
//void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
//{
//    if (huart->Instance == USART_UX)
//    {
//#if USART_EN_RX
//        if((g_usart_rx_sta & 0x8000) == 0)
//        {
//            if(g_usart_rx_sta & 0x4000)
//            {
//                if(g_rx_buffer[0] != 0x0A)
//                {
//                    g_usart_rx_sta = 0;
//                }
//                else
//                {
//                    g_usart_rx_sta |= 0x8000;
//                }
//            }
//            else
//            {
//                if(g_rx_buffer[0] == 0x0D)
//                {
//                    g_usart_rx_sta |= 0x4000;
//                }
//                else
//                {
//                    g_usart_rx_buf[g_usart_rx_sta & 0x3FFF] = g_rx_buffer[0];
//                    g_usart_rx_sta++;
//                    if(g_usart_rx_sta > (USART_REC_LEN - 1))
//                    {
//                        g_usart_rx_sta = 0;
//                    }
//                }
//            }
//        }
//        
//        HAL_UART_Receive_IT(&g_uart1_handle, g_rx_buffer, sizeof(g_rx_buffer));
//#endif
//    }
//}

/**
 * @brief   UART中断回调函数
 * @param   无
 * @retval  无
 */
void USART_UX_IRQHandler(void)
{
#if SYS_SUPPORT_OS
    OSIntEnter();
#endif
    
    HAL_UART_IRQHandler(&g_uart1_handle);
    
#if SYS_SUPPORT_OS
    OSIntExit();
#endif
}
