#include "collector_processor.h"
#include "./BSP/TIMER/gtim.h"

#include "usbd_cdc_if.h"

CircularBuffer* g_cb_ch[ADC_CH_TOTAL];   // 24通道环形缓冲区
uint32_t        g_ch_enable_mask = 0xFFFFFF;  // 默认全部24通道使能
uint8_t         g_spi_adc_cnt[SPI_NUM];  

int8_t collect_cb_init_all(uint32_t cb_len_per_ch)
{
    for (int i = 0; i < ADC_CH_TOTAL; i++) {
        g_cb_ch[i] = cb_init(cb_len_per_ch);
        if (!g_cb_ch[i]) {
            /* 回滚已分配的缓冲区 */
            for (int j = 0; j < i; j++) { cb_free(g_cb_ch[j]); g_cb_ch[j] = NULL; }
            return RET_ERROR;
        }
    }
    return RET_OK;
}

void collect_cb_free_all(void)
{
    
}

void AdcCbClear(void)
{
    for (int i = 0; i < ADC_CH_TOTAL; i++)
        if (g_cb_ch[i]) cb_clear(g_cb_ch[i]);
}     

/* 从中断/DMA回调中调用，spi_idx: 0~2, adc_data: 8个通道数据 */
void adc_write_spi_channels(uint8_t spi_idx, const uint16_t adc_data[SPI_CH_NUM],
                            uint32_t timestamp)
{
    for (uint8_t adc = 0; adc < SPI_CH_NUM; adc++) {
        uint8_t ch = spi_idx * SPI_CH_NUM + adc;
        if (!(g_ch_enable_mask & (1u << ch))) continue;  // 通道未使能，跳过
        /* cb_write 写入2字节（uint16_t） */
        cb_write(g_cb_ch[ch], (const char*)&adc_data[adc], ADC_DATA_LEN);
    }
    (void)timestamp;  /* 如需时间戳，可另建时间戳缓冲 */
}

void CfgAdcSampleRate(uint32_t sample_rate)
{
    if (sample_rate == 0 || sample_rate > 500000)
    {
        usb_printf("CfgAdcSampleRate: invalid rate %lu (1~500000Hz)", sample_rate);
        return;
    }

    const uint32_t tim_freq = 300000000;

    uint32_t best_arr = 0, best_psc = 0;
    uint32_t best_error = UINT32_MAX;

    /* 
     * 总分频 N = tim_freq / sample_rate
     * 枚举 arr，psc = N/(arr+1) - 1
     * arr 范围：高频时小arr，低频时大arr
     */
    uint32_t N = tim_freq / sample_rate;  // 理想总计数

    /* arr 从小到大枚举，范围 9~65535 */
    for (uint32_t arr = 9; arr <= 65535; arr++)
    {
        uint32_t divisor = arr + 1;

        /* 跳过 psc 为 0 或超出 16bit 的组合 */
        if (N < divisor) break;             // psc 会小于 0

        uint32_t psc = (N / divisor);
        if (psc == 0) continue;
        if (psc > 65536) continue;

        /* 检查 psc 和 psc-1 两个候选（补偿整除误差）*/
        for (uint32_t p = (psc > 1 ? psc - 1 : 1); p <= psc + 1; p++)
        {
            if (p == 0 || p > 65536) continue;

            uint32_t actual_rate = tim_freq / (p * divisor);
            uint32_t error = (actual_rate > sample_rate) ?
                             (actual_rate - sample_rate) :
                             (sample_rate - actual_rate);

            if (error < best_error)
            {
                best_error = error;
                best_arr   = arr;
                best_psc   = p - 1;  // HAL 传入的是 psc-1

                if (error == 0) goto done;  // 完美匹配直接退出
            }
        }
    }

done:
    {
        uint32_t actual = tim_freq / ((best_psc + 1) * (best_arr + 1));
        float error_ppm = (float)best_error / sample_rate * 1e6f;

        usb_printf("Target:%6luHz | Actual:%6luHz | Err:%lu(%.1fppm) | arr:%5lu psc:%5lu",
                   sample_rate, actual, best_error, error_ppm, best_arr, best_psc);

        gtim_timx_cfg((uint16_t)best_arr, (uint16_t)best_psc);
    }
}
 
/**
 * @brief  ADC采集控制函数
 * @param  采集运行状态
 * @retval 无
 */
void AdcCollectorContrl(uint8_t run_status)
{
    if (run_status) {
        gtim_timx_start();
    } else {
        gtim_timx_stop();
    }
}
 