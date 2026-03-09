#ifndef __GTIM_H
#define __GTIM_H

#include "./SYSTEM/sys/sys.h"

/* 通用定时器定义 */
#define GTIM_TIMX               TIM2
#define GTIM_TIMX_IRQn          TIM2_IRQn
#define GTIM_TIMX_IRQHandler    TIM2_IRQHandler
#define GTIM_TIMX_CLK_ENABLE()  do { __HAL_RCC_TIM2_CLK_ENABLE(); } while (0)

/* 滴答定时器 */
#define TICKS_TIMX              TIM3
#define TICKS_TIMX_CLK_ENABLE() do { __HAL_RCC_TIM3_CLK_ENABLE(); } while (0)
// 滴答定时器配置参数
#define TICKS_TIMX_CLOCK_FREQ     300000000     // 300MHz
#define TICKS_TIMX_PRESCALER      300           // 不分频
#define TICKS_TIMX_PERIOD         0xFFFFFFFF    // 最大计数值

/* 函数声明 */
void gtim_timx_int_init(unsigned short arr, unsigned short psc);
void gtim_timx_stop(void);
void gtim_timx_start(uint16_t arr, uint16_t psc);


void ticks_timx_start(void);
uint32_t ticks_timx_get_counter(void);
void ticks_timx_reset_counter(void);
float Ticks_Get_Elapsed_Time_ms(uint32_t start_count);
float Ticks_Get_Elapsed_Time_us(uint32_t start_count);
void check_timer_status(void);

#endif
