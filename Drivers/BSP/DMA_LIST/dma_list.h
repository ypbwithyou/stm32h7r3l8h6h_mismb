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
#define DMA_SPI_RX_NODE_USED    10      /* 每路SPI的RX链表节点数 */
#define DMA_SPI_TX_NODE_USED    1       /* 每路SPI的TX链表节点数 */

/*---------------------------------------------------------------------------*/
/* 三路SPI并行完成标志位定义                                                  */
/*---------------------------------------------------------------------------*/
#define SPI_RX_DONE_SPI1        (1U << 0)
#define SPI_RX_DONE_SPI2        (1U << 1)
#define SPI_RX_DONE_SPI3        (1U << 2)
#define SPI_RX_DONE_ALL         (SPI_RX_DONE_SPI1 | SPI_RX_DONE_SPI2 | SPI_RX_DONE_SPI3)

/*---------------------------------------------------------------------------*/
/* 缓冲区管理结构                                                             */
/*---------------------------------------------------------------------------*/
typedef struct {
    volatile uint32_t current_rx_node;    /* 当前接收节点索引 */
    volatile uint32_t current_tx_node;    /* 当前发送节点索引 */
    volatile uint32_t processed_node;     /* 已处理节点索引   */
    volatile uint8_t  data_ready;         /* 数据就绪标志     */
} BufferManager_t;

/*---------------------------------------------------------------------------*/
/* 外部变量声明                                                               */
/*---------------------------------------------------------------------------*/
/* SPI1 DMA句柄 */
extern DMA_HandleTypeDef g_handle_GPDMA1_Channel0;   /* SPI1 TX */
extern DMA_HandleTypeDef g_handle_GPDMA1_Channel1;   /* SPI1 RX */
/* SPI2 DMA句柄 */
extern DMA_HandleTypeDef g_handle_GPDMA1_Channel2;   /* SPI2 TX */
extern DMA_HandleTypeDef g_handle_GPDMA1_Channel3;   /* SPI2 RX */
/* SPI4(SPI3逻辑) DMA句柄 */
extern DMA_HandleTypeDef g_handle_GPDMA1_Channel4;   /* SPI4 TX */
extern DMA_HandleTypeDef g_handle_GPDMA1_Channel5;   /* SPI4 RX */

/* 缓冲区管理 */
extern BufferManager_t g_buffer_mgr;

/* 三路SPI各自的RX缓冲区 */
extern uint8_t spi_rx_buf0[DMA_SPI_RX_NODE_USED][RX_BUFFER_SIZE];   /* SPI1接收 */
extern uint8_t spi_rx_buf1[DMA_SPI_RX_NODE_USED][RX_BUFFER_SIZE];   /* SPI2接收 */
extern uint8_t spi_rx_buf2[DMA_SPI_RX_NODE_USED][RX_BUFFER_SIZE];   /* SPI4接收 */

/* 并行完成状态标志（ISR中设置，主流程中清除） */
extern volatile uint8_t g_spi_rx_done_flags;

/*---------------------------------------------------------------------------*/
/* 函数声明                                                                   */
/*---------------------------------------------------------------------------*/
void dma_list_rtx_init(void);           /* 初始化三路SPI DMA */
void dma_start_transfer_all(void);      /* 同时启动三路SPI DMA传输 */
void dma_list_data_init(void);          /* 初始化缓冲区管理结构 */
uint8_t dma_get_ready_data(uint32_t *node_index);  /* 获取就绪数据 */

/* 向后兼容旧接口（仅SPI1）*/
void dma_start_transfer(void);

#endif /* __DMA_LIST_H */