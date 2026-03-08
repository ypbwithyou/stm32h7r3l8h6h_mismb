/*******************************************************************************
  文件名称：gtim.c

  TIM中断驱动ADC采集：
    TIM2 产生定时中断，每次中断执行一次ADC采集序列。
    采集方式：轮询（ads8319_read_daisy_chain_fast），三路SPI串行读取。
    读取字节数 = g_adc_chan_cfg.ch_per_spi × 2，动态跟随通道配置。
    采集完成后将数据写入CircularBuffer（通过 process_adc_data）。

  注意：当前为轮询方案（非DMA），TIM优先级设为5。
        如需切换为DMA方案，将回调中读取+写CB的逻辑替换为 dma_start_transfer()，
        数据写CB的工作移入 DMA RX 完成回调中完成。
*******************************************************************************/
#include "./BSP/TIMER/gtim.h"
#include "./BSP/ADS8319/ads8319.h"
#include "./BSP/SPI/spi.h"
#include "collector_processor.h"

/* TIM句柄 */
TIM_HandleTypeDef g_gtimx_handle    = {0};
TIM_HandleTypeDef g_timx_ticks_handle = {0};

/* 统计变量 */
volatile uint32_t g_gtim_it_counts   = 0;
volatile uint32_t g_isr_overrun_count = 0;

/*===========================================================================*/
/* 采样TIM（TIM2）初始化                                                      */
/*===========================================================================*/

void gtim_timx_int_init(unsigned short arr, unsigned short psc)
{
    g_gtimx_handle.Instance               = GTIM_TIMX;
    g_gtimx_handle.Init.Prescaler         = psc;
    g_gtimx_handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    g_gtimx_handle.Init.Period            = arr;
    g_gtimx_handle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    g_gtimx_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    HAL_TIM_Base_Init(&g_gtimx_handle);

    g_gtim_it_counts    = 0;
    g_isr_overrun_count = 0;
}

void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == GTIM_TIMX)
    {
        GTIM_TIMX_CLK_ENABLE();
        HAL_NVIC_SetPriority(GTIM_TIMX_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(GTIM_TIMX_IRQn);
    }
}

/*===========================================================================*/
/* TIM中断回调（每个采样周期执行一次）                                        */
/*===========================================================================*/

/*
 * 执行顺序：
 *   1. CONVST 拉高，触发三路ADS8319同步转换（tCONV ≥ 700ns，NOP等待）
 *   2. 三路SPI依次读取各自菊花链数据（ch_per_spi 个ADC × 2字节）
 *   3. CONVST 拉低（ads8319_stop_transfer），结束本轮采集
 *   4. 将三路数据填入 ADC_Data_t，调用 process_adc_data 写入CircularBuffer
 *
 * 动态通道数：
 *   ads8319_read_daisy_chain_fast 的 adc_num 参数直接使用
 *   g_adc_chan_cfg.ch_per_spi，SPI读取字节数 = adc_num × 2，
 *   无需任何额外配置，自然适配1~8路。
 *
 * Overrun检测：
 *   若上一帧DMA/处理尚未完成（可扩展），g_isr_overrun_count 递增。
 *   当前轮询方案下，此处作为预留。
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != GTIM_TIMX) return;

    g_gtim_it_counts++;

    const uint8_t n = g_adc_chan_cfg.ch_per_spi;  /* 每路激活ADC数，运行时确定 */

    /* ── 1. 启动转换 ── */
    ads8319_start_convst();
    /* ads8319_start_convst 内部已有 NOP 等待（约 700ns），无需额外延迟 */

    /* ── 2. 三路SPI读取菊花链数据 ──
     *
     * spi_rx_buffer[spi_idx][0..n*2-1] 存放本路 n 个ADC的原始字节。
     * ads8319_read_daisy_chain_fast 内部已完成 SPI 收发 + 字节转uint16_t。
     * adc_data 返回 n 个 uint16_t，adc_data[0] 对应菊花链最近SPI端的ADC。
     */
    uint16_t adc_ch0[SPI_CH_ADC_MAX_HW];  /* SPI1（SPI1_SPI=1）的n路ADC数据 */
    uint16_t adc_ch1[SPI_CH_ADC_MAX_HW];  /* SPI2（SPI2_SPI=2）的n路ADC数据 */
    uint16_t adc_ch2[SPI_CH_ADC_MAX_HW];  /* SPI3/SPI4（SPI3_SPI=3）的n路ADC数据 */

    ads8319_read_daisy_chain_fast(SPI1_SPI, n, adc_ch0);
    ads8319_read_daisy_chain_fast(SPI2_SPI, n, adc_ch1);
    ads8319_read_daisy_chain_fast(SPI3_SPI, n, adc_ch2);

    /* ── 3. 结束采集，CONVST 拉低 ── */
    ads8319_stop_transfer();

    /* ── 4. 组装 ADC_Data_t 并写入CircularBuffer ──
     *
     * ADC_Data_t.data[spi][ch] 布局（ch_per_spi=3 时）：
     *   data[0][0..2] ← SPI1的3个ADC
     *   data[1][0..2] ← SPI2的3个ADC
     *   data[2][0..2] ← SPI3的3个ADC
     *
     * process_adc_data 内部只写前 ch_per_spi 列到CB，多余列忽略。
     */
    ADC_Data_t sample;
    for (uint8_t ch = 0; ch < n; ch++) {
        sample.data[0][ch] = adc_ch0[ch];
        sample.data[1][ch] = adc_ch1[ch];
        sample.data[2][ch] = adc_ch2[ch];
    }
    sample.timestamp = g_gtim_it_counts;

    process_adc_data(&sample);
}

/*===========================================================================*/
/* TIM IRQ Handler                                                            */
/*===========================================================================*/

void GTIM_TIMX_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_gtimx_handle);
}

/*===========================================================================*/
/* 采样率配置接口                                                             */
/*===========================================================================*/

/*
 * 采样率计算：
 *   TIM2 时钟 = 300MHz（APB1 × 2）
 *   ARR 固定为 99（100分频），PSC 按采样率反推：
 *   PSC = TIM_CLK / (sample_rate × (ARR+1)) - 1
 *
 *   例：sample_rate=51200, TIM_CLK=300MHz, ARR=99
 *   PSC = 300000000 / (51200 × 100) - 1 = 57.59 → 取57
 *   实际采样率 = 300000000 / ((57+1) × 100) ≈ 51724 Hz（误差<1%）
 *
 * 如需精确采样率，可调整 ARR 和 PSC 的组合。
 */
void gtim_timx_cfg(uint16_t arr, uint16_t psc)
{
    gtim_timx_int_init(arr, psc);
}

void gtim_timx_start(void)
{
    HAL_TIM_Base_Start_IT(&g_gtimx_handle);
}

void gtim_timx_stop(void)
{
    HAL_TIM_Base_Stop_IT(&g_gtimx_handle);
}

uint32_t get_gtim_interrupt_count(void)
{
    return g_gtim_it_counts;
}

/*===========================================================================*/
/* 滴答定时器（TIM3，用于精确计时）                                           */
/*===========================================================================*/

static void ticks_timx_int_init(void)
{
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};

    TICKS_TIMX_CLK_ENABLE();

    g_timx_ticks_handle.Instance               = TICKS_TIMX;
    g_timx_ticks_handle.Init.Prescaler         = TICKS_TIMX_PRESCALER - 1;
    g_timx_ticks_handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    g_timx_ticks_handle.Init.Period            = TICKS_TIMX_PERIOD;
    g_timx_ticks_handle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    g_timx_ticks_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
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

float Ticks_Get_Elapsed_Time_ms(uint32_t start_count)
{
    uint32_t now = ticks_timx_get_counter();
    uint32_t elapsed = now - start_count;
    /* TICKS_TIMX_PRESCALER=300 → 每tick=1us → /1000 → ms */
    return (float)elapsed / 1000.0f;
}

float Ticks_Get_Elapsed_Time_us(uint32_t start_count)
{
    uint32_t now = ticks_timx_get_counter();
    uint32_t elapsed = now - start_count;
    return (float)elapsed;
}

void check_timer_status(void)
{
    /* 预留：可在此检查TIM是否正常运行 */
}