/**
 ****************************************************************************************************
 * @file        exti.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       外部中断驱动代码
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

#include "./BSP/EXTI/exti.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "ida_config.h"

/**
 * @brief   初始化外部中断
 * @param   无
 * @retval  无
 */
void exti_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};
    
    /* 使能GPIO端口时钟 */
    WORKMODE_GPIO_CLK_ENABLE();
    EVENT_GPIO_CLK_ENABLE();
    
    /* 配置OFFLINE控制引脚 */
    gpio_init_struct.Pin = WORKMODE_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_IT_RISING_FALLING;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(WORKMODE_GPIO_PORT, &gpio_init_struct);
    
    /* 配置EVENT控制引脚 */
    gpio_init_struct.Pin = EVENT_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_IT_RISING_FALLING;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(EVENT_GPIO_PORT, &gpio_init_struct);
    
    
    /* 配置中断优先级并使能中断 */
    HAL_NVIC_SetPriority(WORKMODE_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(WORKMODE_IRQn);
    HAL_NVIC_SetPriority(EVENT_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EVENT_IRQn);
}

/**
 * @brief   HAL库外部中断回调函数
 * @param   GPIO_Pin: 外部中断线对应的引脚
 * @retval  无
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    delay_ms(1);               /* 机械按键消抖（仅演示，切勿在实际工程的中断服务函数中进行阻塞延时） */
    
    switch (GPIO_Pin)
    {
        case WORKMODE_GPIO_PIN: /* WKUP按键对应引脚发生中断 */
        {
            g_IdaSystemStatus.st_dev_mode.work_mode = (WORKMODE_STATUS==0)?WORKMODE_OFFLINE:WORKMODE_ONLINE;
            break;
        }
        case EVENT_GPIO_PIN: /* KEY0按键对应引脚发生中断 */
        {
            LED0_TOGGLE();      /* 翻转LED0状态 */
            break;
        }
    }
}

/**
 * @brief   WORKMODE按键外部中断服务函数
 * @param   无
 * @retval  无
 */
void WKUP_INT_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(WORKMODE_GPIO_PIN);
}

/**
 * @brief   EVENT按键外部中断服务函数
 * @param   无
 * @retval  无
 */
void KEY0_INT_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(EVENT_GPIO_PIN);
}
