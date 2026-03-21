/**
 * @file        dma_multi_spi.h
 * @brief       3路SPI DMA同步采集驱动
 * @note        支持SPI1/SPI2/SPI4同时DMA接收，用于AD8319多通道采集
 */

#ifndef __DMA_MULTI_SPI_H
#define __DMA_MULTI_SPI_H

#include "./SYSTEM/sys/sys.h"
#include "./BSP/ADS8319/ads8319.h"

/* DMA通道配置 */
#define DMA_SPI_NUM             3           /* SPI数量 */
#define DMA_RX_BUF_SIZE         16          /* 单次接收缓冲区大小（字节）= 8个ADC * 2字节 */
#define DMA_DOUBLE_BUF_SIZE     2           /* 双缓冲区数量 */

/* DMA通道定义 */
typedef struct {
    DMA_HandleTypeDef hdma_rx;              /* DMA接收句柄 */
    DMA_HandleTypeDef hdma_tx;              /* DMA发送句柄 */
    uint8_t spi_idx;                        /* SPI索引 0-2 */
    uint8_t tx_buf[DMA_RX_BUF_SIZE];        /* 发送缓冲区 */
    uint8_t rx_buf[DMA_DOUBLE_BUF_SIZE][DMA_RX_BUF_SIZE]; /* 双缓冲接收 */
    volatile uint8_t buf_idx;               /* 当前缓冲区索引 */
    volatile uint8_t data_ready;            /* 数据就绪标志 */
} DMA_SPI_Channel_t;

extern DMA_SPI_Channel_t g_dma_spi[DMA_SPI_NUM];

/* 函数声明 */
void dma_multi_spi_init(void);
void dma_multi_spi_start(void);
void dma_multi_spi_stop(void);
void dma_multi_spi_process_data(void);

/* 回调函数类型 */
typedef void (*dma_multi_spi_cb_t)(uint8_t spi_idx, const uint8_t *data, uint8_t len);

#endif /* __DMA_MULTI_SPI_H */
