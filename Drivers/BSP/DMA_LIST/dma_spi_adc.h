/**
 * @file    dma_spi_adc.h
 * @brief   使用DMA从SPI菊花链ADS8319 ADC采集数据
 */

#ifndef __DMA_SPI_ADC_H
#define __DMA_SPI_ADC_H

#include <stdint.h>

/* 配置参数 */
#define SPI_USED_MAX                3       // SPI总线数量
#define DMA_SPI_RX_NODE_USED        10      // DMA接收节点数量
// #define RX_BUFFER_SIZE              16      // 每个ADC的缓冲区大小（字节）
#define ADS8319_CHAIN_LENGTH        8       // 菊花链中ADC数量
#define SPI_NUM                     3       // SPI总线数量（与SPI_USED_MAX一致）

/**
 * @brief  DMA采集模块初始化
 */
void dma_adc_init(void);

/**
 * @brief  启动DMA采集（在定时器中调用）
 */
void dma_adc_start_collect(void);

/**
 * @brief  检查是否有就绪数据
 * @param  spi_idx: SPI索引
 * @return 1: 有数据, 0: 无数据
 */
uint8_t dma_adc_has_data(uint8_t spi_idx);

/**
 * @brief  获取最新就绪数据
 * @param  spi_idx: SPI索引
 * @param  adc_data: 输出ADC数据缓冲区
 * @return 1: 成功, 0: 无数据
 */
uint8_t dma_adc_get_data(uint8_t spi_idx, uint16_t *adc_data);

/**
 * @brief  获取批量就绪数据（用于定时器处理）
 * @param  adc_buf: 输出缓冲区 [SPI_NUM][SPI_CH_NUM]
 * @return 成功获取的SPI数量
 */
uint8_t dma_adc_get_batch_data(uint16_t adc_buf[SPI_NUM][ADS8319_CHAIN_LENGTH]);

/**
 * @brief  DMA传输完成回调（在中断中调用）
 * @param  spi_idx: SPI索引
 */
void dma_adc_transfer_complete(uint8_t spi_idx);

/**
 * @brief  获取DMA传输统计信息
 * @param  spi_idx: SPI索引
 * @param  rx_count: 接收完成次数输出
 * @param  tx_count: 发送完成次数输出
 */
void dma_adc_get_stats(uint8_t spi_idx, uint32_t *rx_count, uint32_t *tx_count);

/**
 * @brief  重置DMA采集统计
 */
void dma_adc_reset_stats(void);

#endif /* __DMA_SPI_ADC_H */
