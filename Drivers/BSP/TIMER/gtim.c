
#include "./BSP/TIMER/gtim.h"
#include "./BSP/LED/led.h"
#include "./BSP/ADS8319/ads8319.h"
#include "./BSP/SPI/spi.h"
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

/**
 * @brief  HAL库TIM周期性回调函数（优化版）
 */
// void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
// {
//     uint16_t adc_data[ADS8319_CHAIN_LENGTH][SPI_USED_MAX];
//     if (htim->Instance == GTIM_TIMX)
//     {
//         g_gtim_it_counts++;

//         ticks_timx_reset_counter();
// //        timestamp_t ts = dwt_get_timestamp();
//         uint32_t start_time = ticks_timx_get_counter();
//         // 启动转换
//         ads8319_start_convst();
//         uint32_t middle_time = ticks_timx_get_counter();
// //        timestamp_t te = dwt_get_timestamp();

//         // 读取菊花链中的所有ADC数据
//         ads8319_read_daisy_chain_fast(SPI1_SPI, 1, &adc_data[0][0]);

//         ads8319_read_daisy_chain_fast(SPI2_SPI, 1, &adc_data[0][1]);

//         ads8319_read_daisy_chain_fast(SPI3_SPI, 1, &adc_data[0][2]);
//         if ((adc_data[0][0]>50000) || (adc_data[0][0]<20000)) {
//             adc_data[0][0]++;
//         }
//         if ((adc_data[0][1]>50000) || (adc_data[0][1]<20000)) {
//             adc_data[0][1]++;
//         }
//         if ((adc_data[0][2]>50000) || (adc_data[0][2]<20000)) {
//             adc_data[0][2]++;
//         }
//         // 传输完成
//         ads8319_stop_transfer();
//         uint32_t stop_time = ticks_timx_get_counter();
//         // int8_t ret = cb_write(g_cb_adc, (const char*)&adc_data[0][0], SPI_USED_MAX*2);
// //        timestamp_t tl = dwt_get_timestamp();
//     }
// }

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{

    if (htim->Instance != GTIM_TIMX)
        return;
    g_gtim_it_counts++;

    static uint8_t first_frame = 1;
    uint16_t adc_buf[SPI_NUM][SPI_CH_NUM];

    // 第1步：先读上一次转换结果（第1帧跳过）
    if (!first_frame)
    {
        for (uint8_t spi = 0; spi < SPI_NUM; spi++)
        {
            if (g_spi_adc_cnt[spi] == 0)
                continue;
            ads8319_read_daisy_chain_fast(spi + 1, g_spi_adc_cnt[spi], adc_buf[spi]);
        }
        ADS8319_CONVST_LOW(); // 结束上次转换周期
    }
    else
    {
        first_frame = 0;
        ADS8319_CONVST_LOW();
    }

    // 第2步：启动本次转换
    ADS8319_CONVST_HIGH();
    for (volatile uint16_t i = 0; i < 750; i++)
    {
        __NOP();
    } // 等待1.2μs转换完成

    // 第3步：写入缓冲区（使用刚读到的上帧数据）
    if (g_gtim_it_counts > 1)
    {
        for (uint8_t spi = 0; spi < SPI_NUM; spi++)
        {
            for (uint8_t pos = 0; pos < g_spi_adc_cnt[spi]; pos++)
            {
                uint8_t ch = pos * SPI_NUM + spi;
                if (ch >= ADC_CH_TOTAL)
                    continue;
                if (!(g_ch_enable_mask & (1u << ch)))
                    continue;
                cb_write(g_cb_ch[ch], (const char *)&adc_buf[spi][pos], ADC_DATA_LEN);
            }
        }
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
