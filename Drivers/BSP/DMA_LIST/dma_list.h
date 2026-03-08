/**
 ****************************************************************************************************
 * @file        dma_list.h
 * @brief       三路SPI并行DMA驱动头文件
 *              SPI1 -> GPDMA1_Channel0(TX) / Channel1(RX)
 *              SPI2 -> GPDMA1_Channel2(TX) / Channel3(RX)
 *              SPI4 -> GPDMA1_Channel4(TX) / Channel5(RX)
 ****************************************************************************************************
 */

#ifndef __DMA_LIST_H
#define __DMA_LIST_H

#include "./SYSTEM/sys/sys.h"
#include "./BSP/ADS8319/ads8319.h"

/*---------------------------------------------------------------------------*/
/* DMA链表节点数量配置                                                        */
/*---------------------------------------------------------------------------*/
#define DMA_SPI_RX_NODE_USED 10 /* 每路SPI的RX链表节点数 */
#define DMA_SPI_TX_NODE_USED 1  /* 每路SPI的TX链表节点数 */

/*---------------------------------------------------------------------------*/
/* 三路SPI并行完成标志位定义                                                  */
/*---------------------------------------------------------------------------*/
#define SPI_RX_DONE_SPI1 (1U << 0)
#define SPI_RX_DONE_SPI2 (1U << 1)
#define SPI_RX_DONE_SPI3 (1U << 2)
#define SPI_RX_DONE_ALL (SPI_RX_DONE_SPI1 | SPI_RX_DONE_SPI2 | SPI_RX_DONE_SPI3)

/*---------------------------------------------------------------------------*/
/* 缓冲区管理结构                                                             */
/*---------------------------------------------------------------------------*/

typedef struct
{
    volatile uint32_t current_rx_node;
    volatile uint32_t current_tx_node;
    volatile uint32_t processed_node;
    volatile uint8_t data_ready;
} BufferManager_t;

/*---------------------------------------------------------------------------*/
/* 外部变量声明                                                               */
/*---------------------------------------------------------------------------*/
/* SPI1 DMA句柄 */
extern DMA_HandleTypeDef g_handle_GPDMA1_Channel0; /* SPI1 TX */
extern DMA_HandleTypeDef g_handle_GPDMA1_Channel1; /* SPI1 RX */
/* SPI2 DMA句柄 */
extern DMA_HandleTypeDef g_handle_GPDMA1_Channel2; /* SPI2 TX */
extern DMA_HandleTypeDef g_handle_GPDMA1_Channel3; /* SPI2 RX */
/* SPI4(SPI3逻辑) DMA句柄 */
extern DMA_HandleTypeDef g_handle_GPDMA1_Channel4; /* SPI4 TX */
extern DMA_HandleTypeDef g_handle_GPDMA1_Channel5; /* SPI4 RX */

/* 缓冲区管理 */
extern BufferManager_t g_buffer_mgr;

/* 三路SPI各自的RX缓冲区 */
extern uint8_t spi_rx_buf0[DMA_SPI_RX_NODE_USED][RX_BUFFER_SIZE]; /* SPI1接收 */
extern uint8_t spi_rx_buf1[DMA_SPI_RX_NODE_USED][RX_BUFFER_SIZE]; /* SPI2接收 */
extern uint8_t spi_rx_buf2[DMA_SPI_RX_NODE_USED][RX_BUFFER_SIZE]; /* SPI4接收 */

/* 并行完成状态标志（ISR中设置，主流程中清除） */
extern volatile uint8_t g_spi_rx_done_flags;

/*---------------------------------------------------------------------------*/
/* 函数声明                                                                   */
/*---------------------------------------------------------------------------*/

/**
 * @brief  更新DMA链表节点的传输字节数（通道配置变更时调用）
 * @param  xfer_bytes  每次传输字节数 = ch_per_spi × ADC_DATA_LEN
 *                     合法范围：[2, RX_BUFFER_SIZE]，必须是2的倍数
 * @note   必须在停止采集后、重启前调用；会重建TX/RX所有链表节点。
 */
void dma_update_xfer_size(uint32_t xfer_bytes);

void dma_list_rtx_init(void);                     /* 初始化三路SPI DMA */
void dma_start_transfer_all(void);                /* 同时启动三路SPI DMA传输 */
void dma_list_data_init(void);                    /* 初始化缓冲区管理结构 */
uint8_t dma_get_ready_data(uint32_t *node_index); /* 获取就绪数据 */

/* 向后兼容旧接口（仅SPI1）*/
void dma_start_transfer(void);

#endif /* __DMA_LIST_H */