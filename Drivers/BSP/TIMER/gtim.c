
#include "./BSP/TIMER/gtim.h"
#include "./BSP/LED/led.h"
#include "./BSP/ADS8319/ads8319.h"
#include "./BSP/SPI/spi.h"
#include "./BSP/DMA_LIST/dma_list.h"
#include "collector_processor.h"

#include "usbd_cdc_if.h"
#include "collector_processor.h"

/* TIM鍙ユ焺 */
TIM_HandleTypeDef g_gtimx_handle = {0};
TIM_HandleTypeDef g_timx_ticks_handle = {0};

/* 缁熻鍙橀噺 */
volatile uint32_t g_gtim_it_counts = 0;
volatile uint32_t g_isr_overrun_count = 0;
static uint8_t g_use_dma_path = 1U;
static uint16_t g_dma_fail_streak = 0U;

/**
 * @brief  閫氱敤瀹氭椂鍣ㄥ垵濮嬪寲
 */
void gtim_timx_int_init(unsigned short arr, unsigned short psc)
{
    /* 鍏堝仠姝㈠苟鍙嶅垵濮嬪寲锛屽己鍒剁姸鎬佸洖鍒?RESET */
    HAL_TIM_Base_Stop_IT(&g_gtimx_handle);
    HAL_TIM_Base_DeInit(&g_gtimx_handle);

    g_gtimx_handle.Instance = GTIM_TIMX;
    g_gtimx_handle.Init.Prescaler = psc;
    g_gtimx_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_gtimx_handle.Init.Period = arr;
    g_gtimx_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    g_gtimx_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_TIM_Base_Init(&g_gtimx_handle);

    g_gtim_it_counts = 0;
    g_isr_overrun_count = 0;
    g_use_dma_path = 1U;
    g_dma_fail_streak = 0U;
}

/**
 * @brief  HAL搴揟IM涓柇浼樺厛绾ч厤缃?
 */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == GTIM_TIMX)
    {
        GTIM_TIMX_CLK_ENABLE();

        HAL_NVIC_SetPriority(GTIM_TIMX_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(GTIM_TIMX_IRQn);
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    uint16_t adc_buf[SPI_NUM][SPI_CH_NUM];

    if (htim->Instance != GTIM_TIMX)
    {
        return;
    }

    g_gtim_it_counts++;

    if (g_use_dma_path != 0U)
    {
        dma_ads8319_watchdog_kick();

        if (dma_ads8319_frame_busy() != 0U)
        {
            g_isr_overrun_count++;
            return;
        }

        ads8319_start_convst();

        if (dma_ads8319_start_frame() == HAL_OK)
        {
            g_dma_fail_streak = 0U;
            return;
        }

        ads8319_stop_transfer();
        g_isr_overrun_count++;
        if (g_dma_fail_streak < 0xFFFFU)
        {
            g_dma_fail_streak++;
        }
        if (g_dma_fail_streak > 256U)
        {
            g_use_dma_path = 0U;
        }
    }

    /* Fallback path: keep acquisition alive even if DMA path is unhealthy. */
    ads8319_start_convst();
    for (uint8_t spi = 0; spi < SPI_NUM; spi++)
    {
        if (g_spi_adc_cnt[spi] == 0U)
        {
            continue;
        }
        ads8319_read_daisy_chain_fast(spi + 1U, g_spi_adc_cnt[spi], adc_buf[spi]);
    }
    ads8319_stop_transfer();

    for (uint8_t spi = 0; spi < SPI_NUM; spi++)
    {
        for (uint8_t pos = 0; pos < g_spi_adc_cnt[spi]; pos++)
        {
            uint8_t ch = pos * SPI_NUM + spi;
            if (ch >= ADC_CH_TOTAL)
            {
                continue;
            }
            if ((g_ch_enable_mask & (1u << ch)) == 0U)
            {
                continue;
            }
            cb_write(g_cb_ch[ch], (const char *)&adc_buf[spi][pos], ADC_DATA_LEN);
        }
    }
}

/**
 * @brief  TIM瀹氭椂鍣ㄤ腑鏂湇鍔″嚱鏁?
 */
void GTIM_TIMX_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_gtimx_handle);
}

/**
 * @brief  鍚姩瀹氭椂鍣?
 */
void gtim_timx_cfg(uint16_t arr, uint16_t psc)
{
    if (g_gtimx_handle.State == HAL_TIM_STATE_RESET)
    {
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
        gtim_timx_stop();

        __HAL_TIM_SET_PRESCALER(&g_gtimx_handle, psc);
        __HAL_TIM_SET_AUTORELOAD(&g_gtimx_handle, arr);
        __HAL_TIM_SET_COUNTER(&g_gtimx_handle, 0);
        g_gtimx_handle.Instance->EGR = TIM_EGR_UG;
        __HAL_TIM_CLEAR_FLAG(&g_gtimx_handle, TIM_FLAG_UPDATE);
    }

    g_gtim_it_counts = 0;
    g_isr_overrun_count = 0;
}

/**
 * @brief  鍚姩瀹氭椂鍣?
 */
void gtim_timx_start(void)
{
    __HAL_TIM_CLEAR_FLAG(&g_gtimx_handle, TIM_FLAG_UPDATE);
    __HAL_TIM_ENABLE_IT(&g_gtimx_handle, TIM_IT_UPDATE);
    __HAL_TIM_ENABLE(&g_gtimx_handle);
}

/**
 * @brief  鍋滄瀹氭椂鍣?
 */
void gtim_timx_stop(void)
{
    __HAL_TIM_DISABLE(&g_gtimx_handle);
    __HAL_TIM_DISABLE_IT(&g_gtimx_handle, TIM_IT_UPDATE);
    __HAL_TIM_CLEAR_FLAG(&g_gtimx_handle, TIM_FLAG_UPDATE);
}

/**
 * @brief  鑾峰彇涓柇璁℃暟
 */
uint32_t get_gtim_interrupt_count(void)
{
    return g_gtim_it_counts;
}

/*****************************************婊寸瓟瀹氭椂鍣?*********************************************/

/**
 * @brief  婊寸瓟瀹氭椂鍣ㄥ垵濮嬪寲
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
 * @brief  鍚姩婊寸瓟瀹氭椂鍣?
 */
void ticks_timx_start(void)
{
    ticks_timx_int_init();
    __HAL_TIM_CLEAR_FLAG(&g_timx_ticks_handle, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start(&g_timx_ticks_handle);
}

/**
 * @brief  鑾峰彇婊寸瓟瀹氭椂鍣ㄨ鏁板€?
 */
uint32_t ticks_timx_get_counter(void)
{
    return __HAL_TIM_GET_COUNTER(&g_timx_ticks_handle);
}

/**
 * @brief  閲嶇疆婊寸瓟瀹氭椂鍣?
 */
void ticks_timx_reset_counter(void)
{
    __HAL_TIM_SET_COUNTER(&g_timx_ticks_handle, 0);
}
