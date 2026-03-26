#include "./BSP/TIMER/gtim.h"
#include "./BSP/LED/led.h"
#include "./BSP/ADS8319/ads8319.h"
#include "./BSP/SPI/spi.h"
#include "collector_processor.h"

#include "usbd_cdc_if.h"
#include <string.h>

TIM_HandleTypeDef g_gtimx_handle = {0};
TIM_HandleTypeDef g_timx_ticks_handle = {0};

volatile uint32_t g_gtim_it_counts = 0;
volatile uint32_t g_isr_overrun_count = 0;
static volatile uint8_t g_dma_busy = 0U;
static volatile uint8_t g_dma_pending_mask = 0U;
static volatile uint8_t g_dma_done_mask = 0U;
static volatile uint16_t g_dma_fail_streak = 0U;
static volatile uint32_t g_dma_start_ok_count[SPI_NUM] = {0U};
static volatile uint32_t g_dma_cplt_count[SPI_NUM] = {0U};
static volatile uint32_t g_dma_error_count[SPI_NUM] = {0U};
static volatile uint32_t g_dma_abort_count = 0U;
static volatile uint32_t g_dma_finish_count = 0U;
static volatile uint32_t g_dma_start_fail_count = 0U;
static volatile uint8_t g_last_start_fail_spi = 0xFFU;
static volatile uint8_t g_last_error_spi = 0xFFU;
static volatile uint8_t g_last_abort_pending_mask = 0U;
static volatile uint8_t g_last_abort_done_mask = 0U;
static uint16_t g_adc_buf[SPI_NUM][SPI_CH_NUM];
static uint8_t g_write_spi[ADC_CH_TOTAL];
static uint8_t g_write_pos[ADC_CH_TOTAL];
static uint8_t g_write_ch[ADC_CH_TOTAL];
static uint8_t g_write_cnt = 0U;
static uint32_t g_map_cached_mask = 0xFFFFFFFFUL;
static uint8_t g_map_cached_spi_cnt[SPI_NUM] = {0xFFU, 0xFFU, 0xFFU};

static void gtim_rebuild_write_map_if_needed(void);
static uint8_t gtim_dma_size_bytes(uint8_t spi_idx);
static void gtim_finish_frame_from_isr(void);
static void gtim_abort_frame_from_isr(void);
static void gtim_copy_rx_to_adc_buf(uint8_t spi_idx);
static void gtim_invalidate_spi_rx_cache(uint8_t spi_idx, uint8_t bytes);

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

static uint8_t gtim_dma_size_bytes(uint8_t spi_idx)
{
    return (uint8_t)((g_spi_adc_cnt[spi_idx] * ADS8319_DATA_BITS + 7U) / 8U);
}

static void gtim_invalidate_spi_rx_cache(uint8_t spi_idx, uint8_t bytes)
{
    uintptr_t start;
    uintptr_t end;

    if ((spi_idx >= SPI_NUM) || (bytes == 0U))
    {
        return;
    }

    start = ((uintptr_t)&spi_rx_buffer[spi_idx][0]) & ~((uintptr_t)31U);
    end = (((uintptr_t)&spi_rx_buffer[spi_idx][0]) + bytes + 31U) & ~((uintptr_t)31U);
    SCB_InvalidateDCache_by_Addr((uint32_t *)start, (int32_t)(end - start));
}

static void gtim_copy_rx_to_adc_buf(uint8_t spi_idx)
{
    uint8_t adc_cnt = g_spi_adc_cnt[spi_idx];

    for (uint8_t i = 0U; i < adc_cnt; i++)
    {
        g_adc_buf[spi_idx][i] = (uint16_t)(((uint16_t)spi_rx_buffer[spi_idx][2U * i] << 8) |
                                           spi_rx_buffer[spi_idx][2U * i + 1U]);
    }
}

static void gtim_finish_frame_from_isr(void)
{
    ads8319_stop_transfer();

    for (uint8_t i = 0U; i < g_write_cnt; i++)
    {
        uint8_t spi = g_write_spi[i];
        uint8_t pos = g_write_pos[i];
        uint8_t ch = g_write_ch[i];
        cb_write(g_cb_ch[ch], (const char *)&g_adc_buf[spi][pos], ADC_DATA_LEN);
    }

    g_dma_finish_count++;
    g_dma_busy = 0U;
    g_dma_pending_mask = 0U;
    g_dma_done_mask = 0U;
    g_dma_fail_streak = 0U;
}

static void gtim_abort_frame_from_isr(void)
{
    g_last_abort_pending_mask = g_dma_pending_mask;
    g_last_abort_done_mask = g_dma_done_mask;

    for (uint8_t spi = 0U; spi < SPI_NUM; spi++)
    {
        if ((g_dma_pending_mask & (1U << spi)) != 0U)
        {
            spi_dma_stop((unsigned int)(spi + 1U));
        }
    }

    ads8319_stop_transfer();
    g_dma_busy = 0U;
    g_dma_pending_mask = 0U;
    g_dma_done_mask = 0U;
    g_dma_abort_count++;
    g_dma_fail_streak++;
}

uint8_t gtim_dma_path_enabled(void)
{
    return 1U;
}

uint16_t gtim_dma_fail_streak_get(void)
{
    return g_dma_fail_streak;
}

void gtim_timx_int_init(unsigned short arr, unsigned short psc)
{
    HAL_TIM_Base_Stop_IT(&g_gtimx_handle);
    HAL_TIM_Base_DeInit(&g_gtimx_handle);

    g_gtimx_handle.Instance = GTIM_TIMX;
    g_gtimx_handle.Init.Prescaler = psc;
    g_gtimx_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    g_gtimx_handle.Init.Period = arr;
    g_gtimx_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    g_gtimx_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

    HAL_TIM_Base_Init(&g_gtimx_handle);

    g_gtim_it_counts = 0U;
    g_isr_overrun_count = 0U;
    g_dma_busy = 0U;
    g_dma_pending_mask = 0U;
    g_dma_done_mask = 0U;
    g_dma_fail_streak = 0U;
    memset((void *)g_dma_start_ok_count, 0, sizeof(g_dma_start_ok_count));
    memset((void *)g_dma_cplt_count, 0, sizeof(g_dma_cplt_count));
    memset((void *)g_dma_error_count, 0, sizeof(g_dma_error_count));
    g_dma_abort_count = 0U;
    g_dma_finish_count = 0U;
    g_dma_start_fail_count = 0U;
    g_last_start_fail_spi = 0xFFU;
    g_last_error_spi = 0xFFU;
    g_last_abort_pending_mask = 0U;
    g_last_abort_done_mask = 0U;
}

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
    if (htim->Instance != GTIM_TIMX)
    {
        return;
    }

    g_gtim_it_counts++;
    gtim_rebuild_write_map_if_needed();

    if (g_dma_busy != 0U)
    {
        g_isr_overrun_count++;
        return;
    }

    g_dma_busy = 1U;
    g_dma_pending_mask = 0U;
    g_dma_done_mask = 0U;

    ads8319_start_convst();

    for (uint8_t spi = 0U; spi < SPI_NUM; spi++)
    {
        uint8_t bytes;
        HAL_StatusTypeDef ret;

        if (g_spi_adc_cnt[spi] == 0U)
        {
            continue;
        }

        bytes = gtim_dma_size_bytes(spi);
        ret = spi_read_write_dma_start((unsigned int)(spi + 1U),
                                       &spi_tx_buffer[0],
                                       &spi_rx_buffer[spi][0],
                                       bytes);
        if (ret != HAL_OK)
        {
            g_dma_start_fail_count++;
            g_last_start_fail_spi = spi;
            gtim_abort_frame_from_isr();
            return;
        }

        g_dma_start_ok_count[spi]++;
        g_dma_pending_mask |= (uint8_t)(1U << spi);
    }

    if (g_dma_pending_mask == 0U)
    {
        ads8319_stop_transfer();
        g_dma_busy = 0U;
    }
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    unsigned int spi_periph = spi_get_periph_from_instance(hspi->Instance);
    uint8_t spi_idx;
    uint8_t mask;
    uint8_t bytes;

    if ((spi_periph < SPI1_SPI) || (spi_periph > SPI3_SPI) || (g_dma_busy == 0U))
    {
        return;
    }

    spi_idx = (uint8_t)(spi_periph - 1U);
    mask = (uint8_t)(1U << spi_idx);

    if ((g_dma_pending_mask & mask) == 0U)
    {
        return;
    }

    bytes = gtim_dma_size_bytes(spi_idx);
    gtim_invalidate_spi_rx_cache(spi_idx, bytes);
    gtim_copy_rx_to_adc_buf(spi_idx);

    g_dma_cplt_count[spi_idx]++;
    g_dma_done_mask |= mask;
    if (g_dma_done_mask == g_dma_pending_mask)
    {
        gtim_finish_frame_from_isr();
    }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    unsigned int spi_periph = spi_get_periph_from_instance(hspi->Instance);

    if ((spi_periph >= SPI1_SPI) && (spi_periph <= SPI3_SPI) && (g_dma_busy != 0U))
    {
        g_last_error_spi = (uint8_t)(spi_periph - 1U);
        g_dma_error_count[g_last_error_spi]++;
        gtim_abort_frame_from_isr();
    }
}

void GTIM_TIMX_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&g_gtimx_handle);
}

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

    g_gtim_it_counts = 0U;
    g_isr_overrun_count = 0U;
    g_dma_busy = 0U;
    g_dma_pending_mask = 0U;
    g_dma_done_mask = 0U;
    g_dma_fail_streak = 0U;
    memset((void *)g_dma_start_ok_count, 0, sizeof(g_dma_start_ok_count));
    memset((void *)g_dma_cplt_count, 0, sizeof(g_dma_cplt_count));
    memset((void *)g_dma_error_count, 0, sizeof(g_dma_error_count));
    g_dma_abort_count = 0U;
    g_dma_finish_count = 0U;
    g_dma_start_fail_count = 0U;
    g_last_start_fail_spi = 0xFFU;
    g_last_error_spi = 0xFFU;
    g_last_abort_pending_mask = 0U;
    g_last_abort_done_mask = 0U;
}

void gtim_timx_start(void)
{
    __HAL_TIM_CLEAR_FLAG(&g_gtimx_handle, TIM_FLAG_UPDATE);
    __HAL_TIM_ENABLE_IT(&g_gtimx_handle, TIM_IT_UPDATE);
    __HAL_TIM_ENABLE(&g_gtimx_handle);
}

void gtim_timx_stop(void)
{
    __HAL_TIM_DISABLE(&g_gtimx_handle);
    __HAL_TIM_DISABLE_IT(&g_gtimx_handle, TIM_IT_UPDATE);
    __HAL_TIM_CLEAR_FLAG(&g_gtimx_handle, TIM_FLAG_UPDATE);

    if (g_dma_busy != 0U)
    {
        gtim_abort_frame_from_isr();
    }
}

void gtim_debug_poll(void)
{
    static uint32_t s_last_log_tick = 0U;
    uint32_t now = HAL_GetTick();

    if ((now - s_last_log_tick) < 500U)
    {
        return;
    }
    s_last_log_tick = now;

    usb_printf("[DMA] tick=%lu tim=%lu fin=%lu abort=%lu overrun=%lu busy=%u pending=0x%02X done=0x%02X fail_streak=%u start_fail=%lu last_start_fail_spi=%d last_err_spi=%d\r\n",
               (unsigned long)now,
               (unsigned long)g_gtim_it_counts,
               (unsigned long)g_dma_finish_count,
               (unsigned long)g_dma_abort_count,
               (unsigned long)g_isr_overrun_count,
               (unsigned int)g_dma_busy,
               (unsigned int)g_dma_pending_mask,
               (unsigned int)g_dma_done_mask,
               (unsigned int)g_dma_fail_streak,
               (unsigned long)g_dma_start_fail_count,
               (g_last_start_fail_spi == 0xFFU) ? -1 : (int)g_last_start_fail_spi,
               (g_last_error_spi == 0xFFU) ? -1 : (int)g_last_error_spi);

    usb_printf("[DMA] start_ok=[%lu,%lu,%lu] cplt=[%lu,%lu,%lu] err=[%lu,%lu,%lu] last_abort pending=0x%02X done=0x%02X spi_adc_cnt=[%u,%u,%u]\r\n",
               (unsigned long)g_dma_start_ok_count[0],
               (unsigned long)g_dma_start_ok_count[1],
               (unsigned long)g_dma_start_ok_count[2],
               (unsigned long)g_dma_cplt_count[0],
               (unsigned long)g_dma_cplt_count[1],
               (unsigned long)g_dma_cplt_count[2],
               (unsigned long)g_dma_error_count[0],
               (unsigned long)g_dma_error_count[1],
               (unsigned long)g_dma_error_count[2],
               (unsigned int)g_last_abort_pending_mask,
               (unsigned int)g_last_abort_done_mask,
               (unsigned int)g_spi_adc_cnt[0],
               (unsigned int)g_spi_adc_cnt[1],
               (unsigned int)g_spi_adc_cnt[2]);
}

uint32_t get_gtim_interrupt_count(void)
{
    return g_gtim_it_counts;
}

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
