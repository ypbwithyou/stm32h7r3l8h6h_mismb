/******************************************************************************
  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to any
    third party.
  ------------------------------------------------------------------------
  文件名称：collector_processor.h
  文件标识：采集数据处理
  摘    要：
  当前版本：1.0
  作    者：
  创建日期：2025年12月25日
  修改记录：
*******************************************************************************/
#ifndef __COLLECTOR_PROCESSOR_H
#define __COLLECTOR_PROCESSOR_H

#include <stdint.h>
#include "app_common.h"
#include "./LIBS/lib_circular_buffer/CircularBuffer.h"

/* 配置参数 */
#define ADC_SAMPLE_RATE 10000   // 50KSPS
#define ADC_DATA_QUEUE_SIZE 256 // 数据队列大小

#define SPI_NUM 3                           // SPI总线数量
#define SPI_CH_NUM 8                        // 单SPI总线上ADC通道数（菊花链）
#define ADC_CH_TOTAL (SPI_NUM * SPI_CH_NUM) // 总逻辑通道数 = 24
#define ADC_DATA_LEN 2                      // 单个ADC数据字节数（uint16_t）
#define ADC_CH_ENABLED_MAX ADC_CH_TOTAL     // 最大使能通道数

/* ADC数据结构 */
typedef struct
{
  uint16_t data[ADC_CH_TOTAL]; // [spi*8+adc] = data
  uint32_t timestamp;
} ADC_Sample_t;

#define ADC_CB_SIZE_PER_CH (BLOCK_LEN * ADC_DATA_LEN * 4) // 每通道缓冲，建议4×BLOCK
extern CircularBuffer *g_cb_ch[ADC_CH_TOTAL];             // 24个独立通道缓冲区

/* 通道使能掩码（运行时由配置文件设置）*/
extern uint32_t g_ch_enable_mask; // bit-i: 通道i使能标志
extern uint8_t  g_enabled_chs[ADC_CH_TOTAL];  // 使能的通道号列表（有序）
extern uint8_t  g_enabled_ch_cnt;             // 使能通道总数
extern uint8_t g_spi_adc_cnt[SPI_NUM];  // 加在 g_ch_enable_mask 下面

int8_t collect_cb_init_all(uint32_t cb_len_per_ch);
void collect_cb_free_all(void);

void AdcCbClear(void);

void AdcCollectorContrl(uint8_t run_status);
void CfgAdcSampleRate(uint32_t sample_rate);

void adc_write_spi_channels(uint8_t spi_idx, const uint16_t adc_data[SPI_CH_NUM],
                            uint32_t timestamp);

#endif // __COLLECTOR_PROCESSOR_H
