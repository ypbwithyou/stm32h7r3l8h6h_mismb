/**
 ****************************************************************************************************
 * @file        gtim.c
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       ͨ�ö�ʱ����������
 * @license     Copyright (c) 2020-2032, �������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 * 
 * ʵ��ƽ̨:����ԭ�� H7R3������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 * 
 ****************************************************************************************************
 */

#include "./BSP/TIMER/gtim.h"
#include "./BSP/LED/led.h"
#include "./BSP/ADS8319/ads8319.h"
#include "./BSP/SPI/spi.h"
#include "string.h"

/* TIM��� */
TIM_HandleTypeDef g_gtimx_handle = {0};
TIM_HandleTypeDef g_timx_ticks_handle = {0};
uint32_t g_gtim_it_counts = 0;

/**
 * @brief   通用定时器初始化
 * @note    TIM2、TIM3、TIM4、TIM5、TIM12、TIM13、TIM14时钟源为timg1_ck
 * @note    TIM9、TIM15、TIM16、TIM17时钟源为Ϊtimg2_ck
 * @param   arr:自动重装载值
 * @param   psc:预分频系数
 * @retval  无
 */
void gtim_timx_int_init(unsigned short arr, unsigned short psc)
{
    /* ��ʼ��TIM */
    g_gtimx_handle.Instance = GTIM_TIMX;
    g_gtimx_handle.Init.Prescaler = psc;                                    /* 预分频系数 */
    g_gtimx_handle.Init.CounterMode = TIM_COUNTERMODE_UP;                   /* 计数模式 */
    g_gtimx_handle.Init.Period = arr;                                       /* 重装载值 */
    g_gtimx_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;             /* 时钟分频 */
    g_gtimx_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE; /* 自动重装载预加载模式ʽ */
    HAL_TIM_Base_Init(&g_gtimx_handle);
    
    /* 启动定时器 */
    HAL_TIM_Base_Start_IT(&g_gtimx_handle);
    
    g_gtim_it_counts = 0;
}

/**
 * @brief   HAL库TIM中断优先级配置
 * @param   TIM句柄指针
 * @retval  无
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == GTIM_TIMX)
    {
        /* 使能时钟 */
        GTIM_TIMX_CLK_ENABLE();
        
        /* 配置中断优先级并使能中断 */
        HAL_NVIC_SetPriority(GTIM_TIMX_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(GTIM_TIMX_IRQn);
    }
}

/**
 * @brief   HAL库TIM周期性回调函数
 * @param   TIM句柄指针
 * @retval  无
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == GTIM_TIMX)
    {
//        LED0_TOGGLE();
//        
        g_gtim_it_counts++;
        
        ticks_timx_reset_counter();
        uint32_t start_time = ticks_timx_get_counter();
        // 启动转换
        ads8319_start_convst();
        uint32_t middle_time = ticks_timx_get_counter();
        
        // 读取菊花链中的所有ADC数据
        ads8319_read_daisy_chain(SPI1_SPI);
        ads8319_read_daisy_chain(SPI2_SPI);
        ads8319_read_daisy_chain(SPI3_SPI);
        
        // 传输完成
        ads8319_stop_transfer();
        uint32_t stop_time = ticks_timx_get_counter();
    }
}

/**
 * @brief   TIM定时器中断服务函数
 * @param   TIM句柄指针
 * @retval  无
 */
void GTIM_TIMX_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_gtimx_handle);
}

// 停止定时器
void gtim_timx_stop(void)
{
    HAL_TIM_Base_Stop_IT(&g_gtimx_handle);
}

// 启动定时器
void gtim_timx_start(uint16_t arr, uint16_t psc)
{
    gtim_timx_int_init(arr, psc);
}

/*****************************************滴答定时器**********************************************/

static void ticks_timx_int_init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    
    TICKS_TIMX_CLK_ENABLE();
    
    /* ��ʼ��TIM */
    g_timx_ticks_handle.Instance = TICKS_TIMX;
    g_timx_ticks_handle.Init.Prescaler = TICKS_TIMX_PRESCALER-1;                   /* 预分频系数 */
    g_timx_ticks_handle.Init.CounterMode = TIM_COUNTERMODE_UP;                   /* 计数模式 */
    g_timx_ticks_handle.Init.Period = TICKS_TIMX_PERIOD;                         /* 重装载值 */
    g_timx_ticks_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;             /* 时钟分频 */
    g_timx_ticks_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE; /* 自动重装载预加载模式 */
    HAL_TIM_Base_Init(&g_timx_ticks_handle);

    // 配置时钟源为内部时钟
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    HAL_TIM_ConfigClockSource(&g_timx_ticks_handle, &sClockSourceConfig);

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&g_timx_ticks_handle, &sMasterConfig);
}

// 滴答计数器使能
void ticks_timx_start(void)
{
    ticks_timx_int_init();
    
    __HAL_TIM_CLEAR_FLAG(&g_timx_ticks_handle, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start(&g_timx_ticks_handle);
}

// 获取滴答定时器当前计数值
uint32_t ticks_timx_get_counter(void)
{
    return __HAL_TIM_GET_COUNTER(&g_timx_ticks_handle);
}
// 重置滴答定时器
void ticks_timx_reset_counter(void)
{
    __HAL_TIM_SET_COUNTER(&g_timx_ticks_handle, 0);
}

/***********************************************************************************************/

/**
  * @brief 计算经过的时间（毫秒）
  * @param start_count 开始时的计数值
  * @return 经过的时间（毫秒）
  */
float Ticks_Get_Elapsed_Time_ms(uint32_t start_count)
{
  uint32_t current_count = ticks_timx_get_counter();
  uint32_t elapsed_ticks;
  
  // 处理计数器溢出情况
  if (current_count >= start_count) {
    elapsed_ticks = current_count - start_count;
  } else {
    // 32位计数器溢出处理
    elapsed_ticks = (0xFFFFFFFF - start_count) + current_count + 1;
  }
  
  // 转换为毫秒（计数器频率为10kHz，1个tick=0.1ms）
  return (float)elapsed_ticks / 10.0f;
}

/**
  * @brief 计算经过的时间（微秒）
  * @param start_count 开始时的计数值
  * @return 经过的时间（微秒）
  */
float Ticks_Get_Elapsed_Time_us(uint32_t start_count)
{
  uint32_t current_count = ticks_timx_get_counter();
  uint32_t elapsed_ticks;
  
  if (current_count >= start_count) {
    elapsed_ticks = current_count - start_count;
  } else {
    elapsed_ticks = (0xFFFFFFFF - start_count) + current_count + 1;
  }
  
  // 转换为微秒（计数器频率为10kHz，1个tick=0.1ms=100us）
  return (float)elapsed_ticks * 100.0f;
}

void check_timer_status(void)
{
//    printf("TIM3 Status Check:\n");
//    printf("CNT: 0x%08lX\n", TIM3->CNT);
//    printf("CR1: 0x%08lX\n", TIM3->CR1);
//    printf("SR: 0x%08lX\n", TIM3->SR);
//    printf("PSC: %lu\n", TIM3->PSC);
//    printf("ARR: %lu\n", TIM3->ARR);
    
    // 检查计数器是否运行
    if (TIM3->CR1 & TIM_CR1_CEN) {
//        printf("Counter: RUNNING\n");
        return;
    } else {
//        printf("Counter: STOPPED\n");
        return;
    }
    
    // 检查更新标志
    if (TIM3->SR & TIM_SR_UIF) {
//        printf("Update Flag: SET\n");
        return;
    } else {
//        printf("Update Flag: CLEAR\n");
        return;
    }
}

