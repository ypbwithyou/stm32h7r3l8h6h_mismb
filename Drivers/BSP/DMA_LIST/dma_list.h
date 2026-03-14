 
#ifndef __DMA_H
#define __DMA_H

#include "./SYSTEM/sys/sys.h"
 
#include "./BSP/ADS8319/ads8319.h"
 
#define DMA_SPI_RX_NODE_USED   10
#define DMA_SPI_TX_NODE_USED   1
 
typedef struct {
    volatile uint32_t current_rx_node;    
    volatile uint32_t current_tx_node;   
    volatile uint32_t processed_node;     
    volatile uint8_t data_ready;          
} BufferManager_t;

extern BufferManager_t g_buffer_mgr[SPI_USED_MAX];

extern DMA_HandleTypeDef g_handle_GPDMA1_Channel0;
extern DMA_HandleTypeDef g_handle_GPDMA1_Channel1;
extern DMA_HandleTypeDef g_handle_GPDMA1_Channel2;
extern DMA_HandleTypeDef g_handle_GPDMA1_Channel3;
extern DMA_HandleTypeDef g_handle_GPDMA1_Channel4;
extern DMA_HandleTypeDef g_handle_GPDMA1_Channel5;

extern uint8_t spi_rx_dma_buf[SPI_USED_MAX][DMA_SPI_RX_NODE_USED][RX_BUFFER_SIZE];
 
void dma_list_rtx_init(void);   
void dma_start_transfer(void);
void dma_start_transfer_all(void);
void dma_set_spi_xfer_size(uint8_t spi_idx, uint32_t size_bytes);                                         
void dma_list_data_init(void);
uint8_t dma_get_ready_data(uint32_t *node_index);
uint8_t dma_get_ready_data_by_spi(uint8_t spi_idx, uint32_t *node_index);

#endif


