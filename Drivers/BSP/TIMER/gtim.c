
#include "./BSP/TIMER/gtim.h"
#include "./BSP/LED/led.h"
#include "./BSP/ADS8319/ads8319.h"
#include "./BSP/SPI/spi.h"
#include "./BSP/DMA_LIST/dma_spi_adc.h"
#include "collector_processor.h"

#include "usbd_cdc_if.h"
#include "collector_processor.h"

/* TIM句柄 */
TIM_HandleTypeDef g_gtimx_handle = {0};
TIM_HandleTypeDef g_timx_ticks_handle = {0};

/* 统计变量 */
volatile uint32_t g_gtim_it_counts = 0;
volatile uint32_t g_isr_overrun_count = 0;

/**
 * @brief  通用定时器初始化
 */
void gtim_timx_int_init(unsigned short arr, unsigned short psc)
{
    g_gtimx_handle.Instance = GTIM_TIMX;
    g_gtimx_handle.Init.Prescaler = psc;
    g_gtimx_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_gtimx_handle.Init.Period = arr;
    g_gtimx_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    g_gtimx_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_TIM_Base_Init(&g_gtimx_handle);
    //    HAL_TIM_Base_Start_IT(&g_gtimx_handle);

    g_gtim_it_counts = 0;
    g_isr_overrun_count = 0;
}

/**
 * @brief  HAL库TIM中断优先级配置
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == GTIM_TIMX)
    {
        GTIM_TIMX_CLK_ENABLE();

        // 降低中断优先级，避免影响任务调度
        // 优先级10（数字越大优先级越低）
        HAL_NVIC_SetPriority(GTIM_TIMX_IRQn, 3, 0);
        HAL_NVIC_EnableIRQ(GTIM_TIMX_IRQn);
    }
}
 
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    static uint8_t first_run = 1;

    if (htim->Instance != GTIM_TIMX)
        return;
    
    g_gtim_it_counts++;

    if (first_run)
    {
        // 第一次运行，仅启动转换和DMA，不读取数据
        first_run = 0;
        ads8319_start_convst();
        dma_adc_start_collect();
        return;
    }

    // 读取上一帧DMA数据
    uint16_t adc_buf[SPI_NUM][SPI_CH_NUM];
    
    for (uint8_t spi = 0; spi < SPI_NUM; spi++)
    {
        if (g_spi_adc_cnt[spi] == 0)
            continue;
        
        // 从DMA缓冲区读取数据
        if (!dma_adc_get_data(spi, adc_buf[spi]))
        {
            // 如果没有就绪数据，跳过此SPI
            continue;
        }
    }

    // 交错映射写回 g_cb_ch
    for (uint8_t spi = 0; spi < SPI_NUM; spi++)
    {
        for (uint8_t pos = 0; pos < g_spi_adc_cnt[spi]; pos++)
        {
            uint8_t ch = pos * SPI_NUM + spi; // 交错映射还原通道号
            if (ch >= ADC_CH_TOTAL)
                continue;
            if (!(g_ch_enable_mask & (1u << ch)))
                continue;
            cb_write(g_cb_ch[ch], (const char *)&adc_buf[spi][pos], ADC_DATA_LEN);
        }
    }

    // 启动新一轮转换和DMA
    ads8319_start_convst();
    dma_adc_start_collect();
}

/**
 * @brief  TIM定时器中断服务函数
 */
void GTIM_TIMX_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_gtimx_handle);
}

/**
 * @brief  启动定时器
 */
void gtim_timx_cfg(uint16_t arr, uint16_t psc)
{
    gtim_timx_int_init(arr, psc);
}

/**
 * @brief  启动定时器
 */
void gtim_timx_start(void)
{
    HAL_TIM_Base_Start_IT(&g_gtimx_handle);
}

/**
 * @brief  停止定时器
 */
void gtim_timx_stop(void)
{
    HAL_TIM_Base_Stop_IT(&g_gtimx_handle);
}

/**
 * @brief  获取中断计数
 */
uint32_t get_gtim_interrupt_count(void)
{
    return g_gtim_it_counts;
}

/*****************************************滴答定时器**********************************************/

/**
 * @brief  滴答定时器初始化
 */
static void ticks_timx_int_init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};

    TICKS_TIMX_CLK_ENABLE();

    g_timx_ticks_handle.Instance = TICKS_TIMX;
    g_timx_ticks_handle.Init.Prescaler = TICKS_TIMX_PRESCALER - 1;
    g_timx_ticks_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_timx_ticks_handle.Init.Period = TICKS_TIMX_PERIOD;
    g_timx_ticks_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    g_timx_ticks_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_TIM_Base_Init(&g_timx_ticks_handle);
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    HAL_TIM_ConfigClockSource(&g_timx_ticks_handle, &sClockSourceConfig);
}

/**
 * @brief  启动滴答定时器
 */
void ticks_timx_start(void)
{
    ticks_timx_int_init();
    __HAL_TIM_CLEAR_FLAG(&g_timx_ticks_handle, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start(&g_timx_ticks_handle);
}

/**
 * @brief  获取滴答定时器计数值
 */
uint32_t ticks_timx_get_counter(void)
{
    return __HAL_TIM_GET_COUNTER(&g_timx_ticks_handle);
}

/**
 * @brief  重置滴答定时器
 */
void ticks_timx_reset_counter(void)
{
    __HAL_TIM_SET_COUNTER(&g_timx_ticks_handle, 0);
}
