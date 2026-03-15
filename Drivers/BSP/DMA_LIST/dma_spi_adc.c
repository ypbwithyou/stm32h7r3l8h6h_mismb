/**
 * @file    dma_spi_adc.c
 * @brief   使用DMA从SPI菊花链ADS8319 ADC采集数据
 */

#include <stdint.h>
#include "./BSP/DMA_LIST/dma_spi_adc.h"
#include "./BSP/DMA_LIST/dma_list.h"
#include "./BSP/ADS8319/ads8319.h"
#include "./BSP/SPI/spi.h"
#include "collector_processor.h"  // 用于访问 g_spi_adc_cnt

/* 外部变量 */
extern uint8_t spi_rx_dma_buf[SPI_USED_MAX][DMA_SPI_RX_NODE_USED][RX_BUFFER_SIZE];
extern BufferManager_t g_buffer_mgr[SPI_USED_MAX];
extern SPI_HandleTypeDef g_spi_handle[SPI_USED_MAX];

/* 双缓冲管理 */
static uint16_t adc_dma_buffer[SPI_USED_MAX][DMA_SPI_RX_NODE_USED][ADS8319_CHAIN_LENGTH];
static volatile uint8_t buffer_ready[SPI_USED_MAX][DMA_SPI_RX_NODE_USED];
static volatile uint8_t current_buffer[SPI_USED_MAX];
static volatile uint8_t processed_buffer[SPI_USED_MAX];

/* DMA传输状态 */
static volatile uint8_t dma_transfer_active[SPI_USED_MAX];
static volatile uint32_t dma_transfer_count[SPI_USED_MAX];

/* DMA完成回调统计 */
static volatile uint32_t dma_rx_callback_count[SPI_USED_MAX];
static volatile uint32_t dma_tx_callback_count[SPI_USED_MAX];

/**
 * @brief  DMA采集模块初始化
 */
void dma_adc_init(void)
{
    /* 初始化DMA列表 */
    dma_list_rtx_init();
    
    /* 初始化数据缓冲区 */
    dma_list_data_init();
    
    /* 初始化缓冲区管理器 */
    for (uint8_t spi_idx = 0; spi_idx < SPI_USED_MAX; spi_idx++)
    {
        current_buffer[spi_idx] = 0;
        processed_buffer[spi_idx] = 0;
        dma_transfer_active[spi_idx] = 0;
        dma_transfer_count[spi_idx] = 0;
        dma_rx_callback_count[spi_idx] = 0;
        dma_tx_callback_count[spi_idx] = 0;
    }
    
    /* 设置SPI传输大小（根据g_spi_adc_cnt每个通道的采集数动态设置） */
    for (uint8_t spi_idx = 0; spi_idx < SPI_USED_MAX; spi_idx++)
    {
        /* 每个ADC转换结果为2字节，计算总传输大小 */
        /* 如果g_spi_adc_cnt未初始化或为0，使用默认值ADS8319_CHAIN_LENGTH */
        uint8_t adc_count = g_spi_adc_cnt[spi_idx];
        if (adc_count == 0)
        {
            adc_count = ADS8319_CHAIN_LENGTH;  // 默认使用最大ADC数量
        }
        uint32_t xfer_size = adc_count * 2;  // 2字节/ADC
        dma_set_spi_xfer_size(spi_idx, xfer_size);
    }
}

/**
 * @brief  启动DMA采集（在定时器中调用）
 */
void dma_adc_start_collect(void)
{
    /* 启动DMA传输（异步） */
    dma_start_transfer_all();
}

/**
 * @brief  检查是否有就绪数据
 * @param  spi_idx: SPI索引
 * @return 1: 有数据, 0: 无数据
 */
uint8_t dma_adc_has_data(uint8_t spi_idx)
{
    if (spi_idx >= SPI_USED_MAX)
        return 0;
    
    return buffer_ready[spi_idx][processed_buffer[spi_idx]];
}

/**
 * @brief  获取最新就绪数据
 * @param  spi_idx: SPI索引
 * @param  adc_data: 输出ADC数据缓冲区
 * @return 1: 成功, 0: 无数据
 */
uint8_t dma_adc_get_data(uint8_t spi_idx, uint16_t *adc_data)
{
    if (spi_idx >= SPI_USED_MAX || adc_data == NULL)
        return 0;
    
    /* 检查当前缓冲区是否就绪 */
    if (!buffer_ready[spi_idx][processed_buffer[spi_idx]])
        return 0;
    
    /* 根据g_spi_adc_cnt复制实际采集的ADC数据 */
    uint8_t adc_count = g_spi_adc_cnt[spi_idx];
    for (uint8_t i = 0; i < adc_count; i++)
    {
        adc_data[i] = adc_dma_buffer[spi_idx][processed_buffer[spi_idx]][i];
    }
    
    /* 标记为已处理 */
    buffer_ready[spi_idx][processed_buffer[spi_idx]] = 0;
    
    /* 移动到下一个缓冲区 */
    processed_buffer[spi_idx] = (processed_buffer[spi_idx] + 1) % DMA_SPI_RX_NODE_USED;
    
    return 1;
}

/**
 * @brief  获取批量就绪数据（用于定时器处理）
 * @param  adc_buf: 输出缓冲区 [SPI_NUM][ADS8319_CHAIN_LENGTH]
 * @return 成功获取的SPI数量
 */
uint8_t dma_adc_get_batch_data(uint16_t adc_buf[SPI_NUM][ADS8319_CHAIN_LENGTH])
{
    uint8_t count = 0;
    
    for (uint8_t spi_idx = 0; spi_idx < SPI_NUM; spi_idx++)
    {
        if (dma_adc_get_data(spi_idx, adc_buf[spi_idx]))
        {
            count++;
        }
    }
    
    return count;
}

/**
 * @brief  DMA传输完成回调（在中断中调用）
 * @param  spi_idx: SPI索引
 */
void dma_adc_transfer_complete(uint8_t spi_idx)
{
    if (spi_idx >= SPI_USED_MAX)
        return;
    
    /* 获取当前接收节点索引 */
    uint32_t node_idx = g_buffer_mgr[spi_idx].current_rx_node;
    
    /* 转换DMA字节数据为16位ADC数据 */
    uint8_t *dma_buf = &spi_rx_dma_buf[spi_idx][node_idx][0];
    
    /* 根据g_spi_adc_cnt处理实际采集的ADC数量 */
    uint8_t adc_count = g_spi_adc_cnt[spi_idx];
    for (uint8_t i = 0; i < adc_count; i++)
    {
        adc_dma_buffer[spi_idx][node_idx][i] = (dma_buf[2*i] << 8) | dma_buf[2*i + 1];
    }
    
    /* 标记缓冲区就绪 */
    buffer_ready[spi_idx][node_idx] = 1;
    
    /* 更新当前缓冲区索引 */
    current_buffer[spi_idx] = node_idx;
    
    /* 统计 */
    dma_rx_callback_count[spi_idx]++;
    dma_transfer_count[spi_idx]++;
}
 
/**
 * @brief  重置DMA采集统计
 */
void dma_adc_reset_stats(void)
{
    for (uint8_t spi_idx = 0; spi_idx < SPI_USED_MAX; spi_idx++)
    {
        dma_rx_callback_count[spi_idx] = 0;
        dma_tx_callback_count[spi_idx] = 0;
        dma_transfer_count[spi_idx] = 0;
    }
}

/**
 * @brief  获取DMA传输统计信息
 * @param  spi_idx: SPI索引
 * @param  rx_count: 接收完成次数输出
 * @param  tx_count: 发送完成次数输出
 */
void dma_adc_get_stats(uint8_t spi_idx, uint32_t *rx_count, uint32_t *tx_count)
{
    if (spi_idx >= SPI_USED_MAX)
        return;
    
    if (rx_count) *rx_count = dma_rx_callback_count[spi_idx];
    if (tx_count) *tx_count = dma_tx_callback_count[spi_idx];
}
 
/**
 * @brief  更新DMA回调（在dma_list.c的回调函数中调用）
 */
void dma_adc_update_callback(void)
{
    /* 在dma_list.c的dma_transfer_complete_cb中调用此函数 */
    /* 具体逻辑已在dma_adc_transfer_complete中实现 */
}
