/**
 ****************************************************************************************************
 * @file        gtim.c
 * @brief       通用定时器驱动（已适配三路SPI并行DMA采样）
 *
 * 修改说明：
 *   原方案：TIM中断 → CONVST → 串行读SPI1/SPI2/SPI4（轮询） → 写CircularBuffer
 *   新方案：TIM中断 → CONVST → 等待转换完成 → dma_start_transfer_all()
 *           三路DMA并行传输 → 全部完成后在DMA回调中写CircularBuffer
 *
 *   TIM中断函数本身执行时间极短（仅CONVST+等待+启动DMA），
 *   数据处理完全移至DMA RX回调，彻底解放TIM ISR。
 ****************************************************************************************************
 */

#include "./BSP/TIMER/gtim.h"
#include "./BSP/LED/led.h"
#include "./BSP/ADS8319/ads8319.h"
#include "./BSP/SPI/spi.h"
#include "./BSP/DMA_LIST/dma_list.h"
#include "collector_processor.h"

/* TIM句柄 */
TIM_HandleTypeDef g_gtimx_handle      = {0};
TIM_HandleTypeDef g_timx_ticks_handle = {0};

/* 统计变量 */
volatile uint32_t g_gtim_it_counts    = 0;
volatile uint32_t g_isr_overrun_count = 0;

/**
 * @brief  通用定时器初始化
 */
void gtim_timx_int_init(unsigned short arr, unsigned short psc)
{
    g_gtimx_handle.Instance                    = GTIM_TIMX;
    g_gtimx_handle.Init.Prescaler              = psc;
    g_gtimx_handle.Init.CounterMode            = TIM_COUNTERMODE_UP;
    g_gtimx_handle.Init.Period                 = arr;
    g_gtimx_handle.Init.ClockDivision          = TIM_CLOCKDIVISION_DIV1;
    g_gtimx_handle.Init.AutoReloadPreload       = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_TIM_Base_Init(&g_gtimx_handle);

    g_gtim_it_counts    = 0;
    g_isr_overrun_count = 0;
}

/**
 * @brief  HAL库TIM中断优先级配置
 *         提升至优先级5，减少调度抖动对采样时序的影响
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == GTIM_TIMX)
    {
        GTIM_TIMX_CLK_ENABLE();

        /* 优先级5：高于DMA(2)外的其他任务，保证采样时序精准 */
        HAL_NVIC_SetPriority(GTIM_TIMX_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(GTIM_TIMX_IRQn);
    }
}

/**
 * @brief  HAL库TIM周期性回调函数（并行DMA版本）
 *
 * 执行流程：
 *   1. CONVST 拉高，启动三路ADC同步转换
 *   2. 等待转换完成（用IRQ引脚检测，替代固定NOP延迟）
 *   3. 调用 dma_start_transfer_all() 同时启动三路SPI DMA
 *   4. 数据处理移至 DMA RX完成回调中进行
 *
 * 本函数执行时间目标：< 1μs
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != GTIM_TIMX) return;

    g_gtim_it_counts++;

    /* ── 检查上一轮DMA是否已全部完成（overrun检测） ─────────────── */
    if (g_spi_rx_done_flags != 0 && g_spi_rx_done_flags != SPI_RX_DONE_ALL) {
        /* 上一轮未完成就来了新的中断，记录overrun */
        g_isr_overrun_count++;
        /* 可选：直接返回跳过本轮，保护数据完整性 */
        /* return; */
    }

    /* ── 启动ADC转换 ─────────────────────────────────────────────── */
    ADS8319_CONVST_HIGH();

    /*
     * 等待转换完成：
     * 优先用IRQ引脚检测（精确），加超时保护防止硬件异常死锁。
     * ADS8319 tCONV 典型值 700ns，最大值由数据手册确认。
     *
     * 注意：IRQ在转换完成后拉低。三路共用同一CONVST，
     * 等待任意一路IRQ拉低即可（三路转换时间一致）。
     */
    uint32_t timeout = 2000;  /* 足够覆盖tCONV最大值 */
    while (HAL_GPIO_ReadPin(ADS8319_1_IRQ_GPIO, ADS8319_1_IRQ_PIN) == GPIO_PIN_SET
           && --timeout)
    {
        __NOP();
    }
    /* timeout==0 说明IRQ未响应，可在此记录硬件异常 */

    /* ── 同时启动三路SPI DMA传输 ─────────────────────────────────── */
    dma_start_transfer_all();

    /*
     * 从这里退出TIM ISR。
     * SPI数据传输由DMA独立完成，完成后进入 spi_dma_rx_complete_cb()，
     * 在三路全部完成时统一写CircularBuffer。
     * TIM ISR本身不再做任何轮询等待。
     */
}

/**
 * @brief  TIM定时器中断服务函数
 */
void GTIM_TIMX_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_gtimx_handle);
}

/**
 * @brief  配置定时器参数
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

/*===========================================================================*/
/* 滴答定时器（调试用性能计时）                                               */
/*===========================================================================*/

static void ticks_timx_int_init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};

    TICKS_TIMX_CLK_ENABLE();

    g_timx_ticks_handle.Instance                    = TICKS_TIMX;
    g_timx_ticks_handle.Init.Prescaler              = TICKS_TIMX_PRESCALER - 1;
    g_timx_ticks_handle.Init.CounterMode            = TIM_COUNTERMODE_UP;
    g_timx_ticks_handle.Init.Period                 = TICKS_TIMX_PERIOD;
    g_timx_ticks_handle.Init.ClockDivision          = TIM_CLOCKDIVISION_DIV1;
    g_timx_ticks_handle.Init.AutoReloadPreload       = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_TIM_Base_Init(&g_timx_ticks_handle);
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    HAL_TIM_ConfigClockSource(&g_timx_ticks_handle, &sClockSourceConfig);
}

void ticks_timx_start(void)
{
    ticks_timx_int_init();
    __HAL_TIM_CLEAR_FLAG(&g_timx_ticks_handle, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start(&g_timx_ticks_handle);
}

uint32_t ticks_timx_get_counter(void)
{
    return __HAL_TIM_GET_COUNTER(&g_timx_ticks_handle);
}

void ticks_timx_reset_counter(void)
{
    __HAL_TIM_SET_COUNTER(&g_timx_ticks_handle, 0);
}