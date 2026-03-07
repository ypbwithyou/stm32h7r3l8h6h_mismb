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
#define ADC_SAMPLE_RATE     10000     // 50KSPS
//#define ADC_BUFFER_SIZE     1024      // ADC数据缓冲区大小
#define ADC_DATA_QUEUE_SIZE 256       // 数据队列大小
#define SPI_NUM             3         // SPI总线数量
#define SPI_CH_ADC_MAX      1         // 单SPI总线上ADC个数
#define ADC_DATA_LEN        2           // 单个ADC数据字节数

/* ADC数据结构 */
typedef struct {
    uint16_t data[SPI_NUM][SPI_CH_ADC_MAX];  // 3个SPI，每个1个ADC数据
    uint32_t timestamp;   // 时间戳
} ADC_Data_t;

/* 全局变量 */
extern CircularBuffer* g_cb_adc;

/* 本地函数声明 */
//int8_t collect_cb_init(CircularBuffer* cb, uint32_t cb_len);
CircularBuffer* collect_cb_init(uint32_t cb_len);

void process_adc_data(ADC_Data_t *data);
//BaseType_t adc_send_data_from_isr(uint8_t spi_num, uint8_t adc_num, uint16_t data);

void CfgAdcSampleRate(uint32_t sample_rate);
void AdcCollectorContrl(uint8_t run_status);
void AdcCbClear(void);

#endif // __COLLECTOR_PROCESSOR_H
