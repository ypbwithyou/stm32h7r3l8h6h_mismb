#ifndef __LED_H
#define __LED_H

#include "./SYSTEM/sys/sys.h"

/* 引脚定义 */
#define LED0_GPIO_PORT          GPIOD
#define LED0_GPIO_PIN           GPIO_PIN_14
#define LED0_GPIO_CLK_ENABLE()  do { __HAL_RCC_GPIOD_CLK_ENABLE(); } while (0)

/* IO操作 */
#define LED0(x)                 do { (x) ?                                                              \
                                    HAL_GPIO_WritePin(LED0_GPIO_PORT, LED0_GPIO_PIN, GPIO_PIN_SET):     \
                                    HAL_GPIO_WritePin(LED0_GPIO_PORT, LED0_GPIO_PIN, GPIO_PIN_RESET);   \
                                } while (0)

#define LED0_TOGGLE()           do { HAL_GPIO_TogglePin(LED0_GPIO_PORT, LED0_GPIO_PIN); } while (0)

/* 函数声明 */
void led_init(void);    /* 初始化LED */

#endif
