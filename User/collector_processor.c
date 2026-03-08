/*******************************************************************************
  文件名称：collector_processor.c
*******************************************************************************/
#include "collector_processor.h"
#include "./BSP/TIMER/gtim.h"
#include "./MALLOC/malloc.h"

/*---------------------------------------------------------------------------*/
/* 全局运行时配置，默认最大通道                                               */
/*---------------------------------------------------------------------------*/
AdcChanCfg_t g_adc_chan_cfg = {
    .ch_per_spi       = SPI_CH_ADC_MAX_HW,
    .total_ch         = SPI_NUM * SPI_CH_ADC_MAX_HW,
    .sample_rate      = 0,
    .bytes_per_sample = (uint32_t)SPI_NUM * SPI_CH_ADC_MAX_HW * ADC_DATA_LEN,
};

CircularBuffer *g_cb_adc = NULL;

/*===========================================================================*/
/* CircularBuffer 初始化                                                      */
/*===========================================================================*/

CircularBuffer *collect_cb_init(uint32_t cb_len)
{
    return cb_init((int)cb_len);
}

CircularBuffer *collect_cb_init_ex(uint32_t cb_len)
{
    CircularBuffer *cb = (CircularBuffer *)mymalloc(SRAMIN, sizeof(CircularBuffer));
    if (!cb) return NULL;

    char *buf = (char *)mymalloc(SRAMEX, cb_len);
    if (!buf) {
        myfree(SRAMIN, cb);
        return NULL;
    }

    cb->buffer    = buf;
    cb->capacity  = (int)cb_len;
    cb->size      = 0;
    cb->read_pos  = 0;
    cb->write_pos = 0;

    return cb;
}

/*===========================================================================*/
/* 动态通道配置                                                               */
/*===========================================================================*/

int8_t AdcChanCfgSet(uint8_t ch_per_spi, uint32_t sample_rate)
{
    if (ch_per_spi == 0 || ch_per_spi > SPI_CH_ADC_MAX_HW) {
        return -1;
    }

    g_adc_chan_cfg.ch_per_spi       = ch_per_spi;
    g_adc_chan_cfg.total_ch         = (uint8_t)(SPI_NUM * ch_per_spi);
    g_adc_chan_cfg.bytes_per_sample = (uint32_t)SPI_NUM * ch_per_spi * ADC_DATA_LEN;

    if (sample_rate > 0) {
        g_adc_chan_cfg.sample_rate = sample_rate;
        CfgAdcSampleRate(sample_rate);
    }

    /* 清空旧数据，避免帧格式错乱 */
    if (g_cb_adc) {
        cb_clear(g_cb_adc);
    }

    return 0;
}

/*===========================================================================*/
/* ADC 数据写入 CircularBuffer                                                */
/*===========================================================================*/

/*
 * CB内数据布局（以 ch_per_spi=3 为例，每样本18字节）：
 *
 *   [SPI0_CH0_Hi][SPI0_CH0_Lo][SPI0_CH1_Hi][SPI0_CH1_Lo][SPI0_CH2_Hi][SPI0_CH2_Lo]
 *   [SPI1_CH0_Hi][SPI1_CH0_Lo][SPI1_CH1_Hi][SPI1_CH1_Lo][SPI1_CH2_Hi][SPI1_CH2_Lo]
 *   [SPI2_CH0_Hi][SPI2_CH0_Lo][SPI2_CH1_Hi][SPI2_CH1_Lo][SPI2_CH2_Hi][SPI2_CH2_Lo]
 *
 * data[spi][0..ch_per_spi-1] 在内存中连续，每路写 ch_per_spi×2 字节。
 * data[spi][ch_per_spi..7] 是无效列，不写入CB。
 */
void process_adc_data(ADC_Data_t *data)
{
    const uint8_t  n   = g_adc_chan_cfg.ch_per_spi;
    const uint32_t row = (uint32_t)n * ADC_DATA_LEN;

    for (uint8_t spi = 0; spi < SPI_NUM; spi++) {
        int ret = cb_write(g_cb_adc, (const char *)data->data[spi], (int)row);
        if (ret != (int)row) {
            /* CB溢出，停止写入（不写入半帧数据，保持帧对齐）*/
            break;
        }
    }
}

/*===========================================================================*/
/* 采样率 / 采集控制                                                          */
/*===========================================================================*/

/*
 * 采样率计算（ARR=99固定，PSC动态）：
 *   TIM2 CLK = 300MHz
 *   PSC = 300,000,000 / (sample_rate × 100) - 1
 *
 * 常用采样率对应PSC值：
 *   51200 Hz → PSC = 57  → 实际 51724 Hz（误差 +1%）
 *   102400Hz → PSC = 28  → 实际 103448Hz（误差 +1%）
 *   204800Hz → PSC = 13  → 实际 214286Hz（误差 +4.6%）
 *   25600 Hz → PSC = 116 → 实际 25641 Hz（误差 +0.16%）
 *
 * 如需精确采样率，用 arr=(PSC+1)*n-1 配合更大PSC来降低误差。
 */
void CfgAdcSampleRate(uint32_t sample_rate)
{
    const uint32_t tim_clk = 300000000U;
    const uint32_t arr     = 100U - 1U;
    uint32_t psc = (tim_clk / (sample_rate * (arr + 1U)));
    if (psc > 0) psc -= 1U;
    gtim_timx_cfg((uint16_t)arr, (uint16_t)psc);
}

void AdcCollectorContrl(uint8_t run_status)
{
    if (run_status) {
        gtim_timx_start();
    } else {
        gtim_timx_stop();
    }
}

void AdcCbClear(void)
{
    if (g_cb_adc) cb_clear(g_cb_adc);
}