#ifndef __EXTI_H
#define __EXTI_H

#include "./SYSTEM/sys/sys.h"

/* 引脚、中断编号和中断服务函数定义 */
#define WORKMODE_GPIO_PORT           GPIOA
#define WORKMODE_GPIO_PIN            GPIO_PIN_1
#define WORKMODE_GPIO_CLK_ENABLE()   do { __HAL_RCC_GPIOA_CLK_ENABLE(); } while (0)
#define WORKMODE_IRQn                EXTI1_IRQn
#define WORKMODE_IRQHandler          EXTI1_IRQHandler

#define EVENT_GPIO_PORT             GPIOA
#define EVENT_GPIO_PIN              GPIO_PIN_2
#define EVENT_GPIO_CLK_ENABLE()     do { __HAL_RCC_GPIOA_CLK_ENABLE(); } while (0)
#define EVENT_IRQn                  EXTI2_IRQn
#define EVENT_IRQHandler            EXTI2_IRQHandler

#define WORKMODE_STATUS             ((HAL_GPIO_ReadPin(WORKMODE_GPIO_PORT, WORKMODE_GPIO_PIN) == GPIO_PIN_RESET) ? 0 : 1)
#define EVENT_STATUS                ((HAL_GPIO_ReadPin(EVENT_GPIO_PORT, EVENT_GPIO_PIN) == GPIO_PIN_RESET) ? 0 : 1)

/* 函数声明 */
void exti_init(void);   /* 初始化外部中断 */

#endif // __EXTI_H
