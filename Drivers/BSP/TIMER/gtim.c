
#include "./BSP/TIMER/gtim.h"
#include "./BSP/LED/led.h"
#include "./BSP/ADS8319/ads8319.h"
#include "./BSP/SPI/spi.h"
#include "./BSP/SPI_DMA/spi_dma.h"
#include "collector_processor.h"

#include "usbd_cdc_if.h"
#include <string.h>

/* TIM句柄 */
TIM_HandleTypeDef g_gtimx_handle = {0};
TIM_HandleTypeDef g_timx_ticks_handle = {0};

/* 统计变量 */
volatile uint32_t g_gtim_it_counts = 0;
volatile uint32_t g_isr_overrun_count = 0;
static uint16_t g_adc_buf[SPI_NUM][SPI_CH_NUM];
static uint8_t g_write_spi[ADC_CH_TOTAL];
static uint8_t g_write_pos[ADC_CH_TOTAL];
static uint8_t g_write_ch[ADC_CH_TOTAL];
static uint8_t g_write_cnt = 0U;
static uint32_t g_map_cached_mask = 0xFFFFFFFFUL;
static uint8_t g_map_cached_spi_cnt[SPI_NUM] = {0xFFU, 0xFFU, 0xFFU};

/* DMA传输缓冲区 */
static uint8_t g_spi_tx_buf[3][SPI_DMA_MAX_TRANSFER_SIZE];
static uint8_t g_spi_rx_buf[3][SPI_DMA_MAX_TRANSFER_SIZE];
static uint16_t g_spi_dma_size[3];

/* DMA模式标志 */
static volatile uint8_t g_dma_transfer_pending = 0;

static void gtim_rebuild_write_map_if_needed(void)
{
    if ((g_map_cached_mask == g_ch_enable_mask) &&
        (memcmp(g_map_cached_spi_cnt, g_spi_adc_cnt, sizeof(g_map_cached_spi_cnt)) == 0))
    {
        return;
    }

    g_write_cnt = 0U;
    for (uint8_t spi = 0U; spi < SPI_NUM; spi++)
    {
        for (uint8_t pos = 0U; pos < g_spi_adc_cnt[spi]; pos++)
        {
            uint8_t ch = (uint8_t)(pos * SPI_NUM + spi);
            if (ch >= ADC_CH_TOTAL)
            {
                continue;
            }
            if ((g_ch_enable_mask & (1UL << ch)) == 0U)
            {
                continue;
            }
            g_write_spi[g_write_cnt] = spi;
            g_write_pos[g_write_cnt] = pos;
            g_write_ch[g_write_cnt] = ch;
            g_write_cnt++;
        }
    }

    g_map_cached_mask = g_ch_enable_mask;
    memcpy(g_map_cached_spi_cnt, g_spi_adc_cnt, sizeof(g_map_cached_spi_cnt));
}

uint8_t gtim_dma_path_enabled(void)
{
    return 1U; /* 启用DMA模式 */
}

uint16_t gtim_dma_fail_streak_get(void)
{
    return 0U;
}

/**
 * @brief  通用定时器初始化
 */
void gtim_timx_int_init(unsigned short arr, unsigned short psc)
{
    /* 先停止并反初始化，强制状态回到 RESET */
    HAL_TIM_Base_Stop_IT(&g_gtimx_handle);
    HAL_TIM_Base_DeInit(&g_gtimx_handle); // ← State 复位为 HAL_TIM_STATE_RESET

    g_gtimx_handle.Instance = GTIM_TIMX;
    g_gtimx_handle.Init.Prescaler = psc;
    g_gtimx_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_gtimx_handle.Init.Period = arr;
    g_gtimx_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    g_gtimx_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_TIM_Base_Init(&g_gtimx_handle);

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
        HAL_NVIC_SetPriority(GTIM_TIMX_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(GTIM_TIMX_IRQn);
    }
}
 
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != GTIM_TIMX)
        return;
    g_gtim_it_counts++;

    gtim_rebuild_write_map_if_needed();

    /* 启动ADC转换 */
    ads8319_start_convst();

    /* 使用DMA同时启动三路SPI传输 */
    memset(g_spi_dma_size, 0, sizeof(g_spi_dma_size));
    
    for (uint8_t spi = 0; spi < SPI_NUM; spi++)
    {
        if (g_spi_adc_cnt[spi] == 0)
            continue;
        
        /* 计算传输字节数 */
        g_spi_dma_size[spi] = (g_spi_adc_cnt[spi] * ADS8319_DATA_BITS + 7) / 8;
        
        /* 填充TX缓冲区（发送0xFF生成时钟） */
        memset(g_spi_tx_buf[spi], 0xFF, g_spi_dma_size[spi]);
    }
    
    /* 同时启动三路DMA传输 */
    spi_dma_start_all(g_spi_tx_buf, g_spi_rx_buf, g_spi_dma_size);
    
    /* 等待所有DMA传输完成 */
    spi_dma_wait_all_complete();
    
    /* 停止ADC转换 */
    ads8319_stop_transfer();

    /* 解析DMA接收的数据 */
    for (uint8_t spi = 0; spi < SPI_NUM; spi++)
    {
        if (g_spi_adc_cnt[spi] == 0)
            continue;
        
        for (uint8_t i = 0; i < g_spi_adc_cnt[spi]; i++)
        {
            g_adc_buf[spi][i] = (g_spi_rx_buf[spi][0 + 2 * i] << 8) | 
                                 g_spi_rx_buf[spi][1 + 2 * i];
        }
    }

    /* 写入缓冲区 */
    for (uint8_t i = 0U; i < g_write_cnt; i++)
    {
        uint8_t spi = g_write_spi[i];
        uint8_t pos = g_write_pos[i];
        uint8_t ch = g_write_ch[i];
        cb_write(g_cb_ch[ch], (const char *)&g_adc_buf[spi][pos], ADC_DATA_LEN);
    }
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

    if (g_gtimx_handle.State == HAL_TIM_STATE_RESET)
    {
        /* 首次初始化走完整 HAL 流程 */
        g_gtimx_handle.Instance = GTIM_TIMX;
        g_gtimx_handle.Init.Prescaler = psc;
        g_gtimx_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
        g_gtimx_handle.Init.Period = arr;
        g_gtimx_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
        g_gtimx_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
        HAL_TIM_Base_Init(&g_gtimx_handle);
    }
    else
    {
        /* 后续重配：直接操作寄存器 */
        gtim_timx_stop(); // ← 用新的stop

        __HAL_TIM_SET_PRESCALER(&g_gtimx_handle, psc);
        __HAL_TIM_SET_AUTORELOAD(&g_gtimx_handle, arr);
        __HAL_TIM_SET_COUNTER(&g_gtimx_handle, 0);
        g_gtimx_handle.Instance->EGR = TIM_EGR_UG; // 强制装载影子寄存器
        __HAL_TIM_CLEAR_FLAG(&g_gtimx_handle, TIM_FLAG_UPDATE);
    }

    g_gtim_it_counts = 0;
    g_isr_overrun_count = 0;
}

/**
 * @brief  启动定时器
 */
void gtim_timx_start(void)
{
    /* 清除上次可能残留的更新中断标志，防止启动后立即误触发 */
    __HAL_TIM_CLEAR_FLAG(&g_gtimx_handle, TIM_FLAG_UPDATE);

    /* 使能更新中断 */
    __HAL_TIM_ENABLE_IT(&g_gtimx_handle, TIM_IT_UPDATE);

    /* 启动计数器 */
    __HAL_TIM_ENABLE(&g_gtimx_handle);
}

/**
 * @brief  停止定时器
 */
void gtim_timx_stop(void)
{
    /* 停止计数器 */
    __HAL_TIM_DISABLE(&g_gtimx_handle);

    /* 关闭更新中断 */
    __HAL_TIM_DISABLE_IT(&g_gtimx_handle, TIM_IT_UPDATE);

    /* 清除中断标志，防止下次启动时立即触发 */
    __HAL_TIM_CLEAR_FLAG(&g_gtimx_handle, TIM_FLAG_UPDATE);
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
