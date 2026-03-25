#ifndef __SPI_DMA_H
#define __SPI_DMA_H

#include "./SYSTEM/sys/sys.h"
#include "./BSP/SPI/spi.h"

/* DMA通道定义 */
#define SPI1_DMA_CHANNEL    GPDMA1_Channel0
#define SPI2_DMA_CHANNEL    GPDMA1_Channel1
#define SPI3_DMA_CHANNEL    GPDMA1_Channel2

/* DMA中断定义 */
#define SPI1_DMA_IRQn       GPDMA1_Channel0_IRQn
#define SPI2_DMA_IRQn       GPDMA1_Channel1_IRQn
#define SPI3_DMA_IRQn       GPDMA1_Channel2_IRQn

#define SPI1_DMA_IRQHandler GPDMA1_Channel0_IRQHandler
#define SPI2_DMA_IRQHandler GPDMA1_Channel1_IRQHandler
#define SPI3_DMA_IRQHandler GPDMA1_Channel2_IRQHandler

/* DMA请求定义 */
#define SPI1_DMA_RX_REQUEST GPDMA1_REQUEST_SPI1_RX
#define SPI2_DMA_RX_REQUEST GPDMA1_REQUEST_SPI2_RX
#define SPI3_DMA_RX_REQUEST GPDMA1_REQUEST_SPI4_RX

#define SPI1_DMA_TX_REQUEST GPDMA1_REQUEST_SPI1_TX
#define SPI2_DMA_TX_REQUEST GPDMA1_REQUEST_SPI2_TX
#define SPI3_DMA_TX_REQUEST GPDMA1_REQUEST_SPI4_TX

/* 最大传输字节数 */
#define SPI_DMA_MAX_TRANSFER_SIZE 32

/* DMA状态定义 */
typedef enum {
    SPI_DMA_IDLE = 0,
    SPI_DMA_BUSY,
    SPI_DMA_COMPLETE,
    SPI_DMA_ERROR
} SPI_DMA_State_t;

/* SPI DMA句柄结构 */
typedef struct {
    DMA_HandleTypeDef hdma_rx;
    DMA_HandleTypeDef hdma_tx;
    volatile SPI_DMA_State_t state;
    uint8_t *rx_buffer;
    uint8_t *tx_buffer;
    uint16_t transfer_size;
} SPI_DMA_Handle_t;

/* 外部声明 */
extern SPI_DMA_Handle_t g_spi_dma_handle[3];

/* 函数声明 */
void spi_dma_init(void);
void spi_dma_start_transfer(uint8_t spi_idx, uint8_t *tx_data, uint8_t *rx_data, uint16_t size);
uint8_t spi_dma_is_complete(uint8_t spi_idx);
void spi_dma_wait_complete(uint8_t spi_idx);
void spi_dma_abort(uint8_t spi_idx);

/* 同时启动三路SPI DMA传输 */
void spi_dma_start_all(uint8_t tx_data[3][SPI_DMA_MAX_TRANSFER_SIZE],
                       uint8_t rx_data[3][SPI_DMA_MAX_TRANSFER_SIZE],
                       uint16_t size[3]);

/* 等待所有SPI DMA完成 */
uint8_t spi_dma_wait_all_complete(void);

/* DMA传输完成回调 */
void spi_dma_rx_cplt_callback(DMA_HandleTypeDef *hdma);

#endif /* __SPI_DMA_H */