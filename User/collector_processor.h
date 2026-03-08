/*******************************************************************************
  文件名称：collector_processor.h

  硬件规格：
    SPI_NUM           = 3   三路SPI总线（SPI1/SPI2/SPI4）
    SPI_CH_ADC_MAX_HW = 8   每路菊花链最大ADS8319数量（硬件上限）

  动态配置接口：
    PC 端在 DVSARM_CSP_START 帧的 nTotalChannelNum 字段指定总通道数。
    合法值：3/6/9/12/15/18/21/24（必须是SPI_NUM的整数倍）。
    ARM端解析后调用 AdcChanCfgSet(ch_per_spi, sample_rate) 完成配置。

  调用时序：
    1. AdcCollectorContrl(0)          停止TIM
    2. AdcChanCfgSet(n, rate)         更新 g_adc_chan_cfg（含DMA同步，轮询方案不需要）
    3. AdcCollectorContrl(1)          重启TIM
*******************************************************************************/
#ifndef __COLLECTOR_PROCESSOR_H
#define __COLLECTOR_PROCESSOR_H

#include <stdint.h>
#include "app_common.h"
#include "./LIBS/lib_circular_buffer/CircularBuffer.h"

/*---------------------------------------------------------------------------*/
/* 硬件上限（编译期固定）                                                     */
/*---------------------------------------------------------------------------*/
#define SPI_NUM               3
#define SPI_CH_ADC_MAX_HW     8
#define SPI_CH_ADC_MAX        SPI_CH_ADC_MAX_HW   /* 向后兼容别名 */
#define ADC_DATA_LEN          2                    /* uint16_t = 2字节 */

/*---------------------------------------------------------------------------*/
/* CircularBuffer 容量                                                        */
/*---------------------------------------------------------------------------*/
/* 内部SRAM（按最大通道）：3 × 8 × 2 × 256 = 12288 字节 */
#define ADC_CB_SIZE_INTERNAL  (SPI_NUM * ADC_DATA_LEN * SPI_CH_ADC_MAX_HW * BLOCK_LEN)
/* 外部HyperRAM：4MB ≈ 3.4秒@204.8kHz×24ch */
#define ADC_CB_SIZE_EXTERNAL  (4 * 1024 * 1024)

/*---------------------------------------------------------------------------*/
/* 运行时通道配置（全局唯一）                                                 */
/*---------------------------------------------------------------------------*/
typedef struct {
    uint8_t  ch_per_spi;        /* 每路SPI激活的ADC数  [1, SPI_CH_ADC_MAX_HW] */
    uint8_t  total_ch;          /* 总通道数 = SPI_NUM × ch_per_spi            */
    uint32_t sample_rate;       /* 采样率 Hz                                  */
    uint32_t bytes_per_sample;  /* 每次采样写CB字节数
                                   = SPI_NUM × ch_per_spi × ADC_DATA_LEN     */
} AdcChanCfg_t;

extern AdcChanCfg_t g_adc_chan_cfg;

/*---------------------------------------------------------------------------*/
/* ADC 原始采样数据结构                                                       */
/* 按最大硬件通道分配，运行时只有前 ch_per_spi 列有效                        */
/*---------------------------------------------------------------------------*/
typedef struct {
    uint16_t data[SPI_NUM][SPI_CH_ADC_MAX_HW];  /* [SPI路][该路第N个ADC] */
    uint32_t timestamp;
} ADC_Data_t;

/*---------------------------------------------------------------------------*/
/* 全局变量                                                                   */
/*---------------------------------------------------------------------------*/
extern CircularBuffer *g_cb_adc;

/*---------------------------------------------------------------------------*/
/* 函数声明                                                                   */
/*---------------------------------------------------------------------------*/

/** 内部SRAM初始化 */
CircularBuffer *collect_cb_init(uint32_t cb_len);

/** 外部HyperRAM初始化（推荐高采样率场景）*/
CircularBuffer *collect_cb_init_ex(uint32_t cb_len);

/**
 * @brief  动态设置每路激活ADC数量，并清空CB
 * @param  ch_per_spi  [1, SPI_CH_ADC_MAX_HW]
 * @param  sample_rate Hz，0 表示不修改采样率
 * @retval 0:成功  -1:参数非法
 * @note   调用前必须先 AdcCollectorContrl(0) 停止采集
 */
int8_t AdcChanCfgSet(uint8_t ch_per_spi, uint32_t sample_rate);

/**
 * @brief  将一次采样写入CircularBuffer
 *         每路只写前 g_adc_chan_cfg.ch_per_spi 列
 */
void process_adc_data(ADC_Data_t *data);

void CfgAdcSampleRate(uint32_t sample_rate);
void AdcCollectorContrl(uint8_t run_status);
void AdcCbClear(void);

#endif /* __COLLECTOR_PROCESSOR_H */