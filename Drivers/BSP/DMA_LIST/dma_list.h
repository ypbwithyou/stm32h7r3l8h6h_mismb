/**
 ****************************************************************************************************
 * @file        dma.h
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       DMA��������
 * @license     Copyright (c) 2020-2032, �������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 * 
 * ʵ��ƽ̨:����ԭ�� H7R3������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 * 
 ****************************************************************************************************
 */

#ifndef __DMA_H
#define __DMA_H

#include "./SYSTEM/sys/sys.h"
// #include "./BSP/ADS8922B/ads8922b.h"
#include "./BSP/ADS8319/ads8319.h"

/* DMA链表节点使用量 */
#define DMA_SPI_RX_NODE_USED   10
#define DMA_SPI_TX_NODE_USED   1

/* 缓冲区管理结构 */
typedef struct {
    volatile uint32_t current_rx_node;    // 当前接收节点
    volatile uint32_t current_tx_node;    // 当前发送节点  
    volatile uint32_t processed_node;     // 已处理的节点
    volatile uint8_t data_ready;          // 数据就绪标志
} BufferManager_t;

extern BufferManager_t g_buffer_mgr;

extern DMA_HandleTypeDef g_handle_GPDMA1_Channel0;
extern DMA_HandleTypeDef g_handle_GPDMA1_Channel1;

extern uint8_t spi_rx_buf0[DMA_SPI_RX_NODE_USED][RX_BUFFER_SIZE];

/* �������� */
void dma_list_rtx_init(void);   /* ��ʼ��DMA */
void dma_start_transfer(void);                                          /* ����DMA���� */
void dma_list_data_init(void);
uint8_t dma_get_ready_data(uint32_t *node_index);

#endif
