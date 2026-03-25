#include "./BSP/SPI_DMA/spi_dma.h"
#include "./BSP/SPI/spi.h"
#include <string.h>

/* SPI DMA句柄数组 */
SPI_DMA_Handle_t g_spi_dma_handle[3] = {0};

/* 默认TX数据（发送0xFF生成时钟） */
static uint8_t g_default_tx_data[SPI_DMA_MAX_TRANSFER_SIZE] = {[0 ... SPI_DMA_MAX_TRANSFER_SIZE-1] = 0xFF};

/**
 * @brief   初始化SPI DMA
 * @param   无
 * @retval  无
 */
void spi_dma_init(void)
{
    /* 使能GPDMA1时钟 */
    __HAL_RCC_GPDMA1_CLK_ENABLE();
    
    /* ========== SPI1 DMA通道 ========== */
    /* RX通道: GPDMA1_Channel6 (避免与dma_list.c的Channel0冲突) */
    g_spi_dma_handle[0].hdma_rx.Instance = GPDMA1_Channel6;
    g_spi_dma_handle[0].hdma_rx.Init.Request = GPDMA1_REQUEST_SPI1_RX;
    g_spi_dma_handle[0].hdma_rx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    g_spi_dma_handle[0].hdma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    g_spi_dma_handle[0].hdma_rx.Init.SrcInc = DMA_SINC_FIXED;
    g_spi_dma_handle[0].hdma_rx.Init.DestInc = DMA_DINC_INCREMENTED;
    g_spi_dma_handle[0].hdma_rx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    g_spi_dma_handle[0].hdma_rx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    g_spi_dma_handle[0].hdma_rx.Init.Priority = DMA_HIGH_PRIORITY;
    g_spi_dma_handle[0].hdma_rx.Init.SrcBurstLength = 1;
    g_spi_dma_handle[0].hdma_rx.Init.DestBurstLength = 1;
    g_spi_dma_handle[0].hdma_rx.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    g_spi_dma_handle[0].hdma_rx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    g_spi_dma_handle[0].hdma_rx.Init.Mode = DMA_NORMAL;
    HAL_DMA_Init(&g_spi_dma_handle[0].hdma_rx);
    HAL_DMA_ConfigChannelAttributes(&g_spi_dma_handle[0].hdma_rx, DMA_CHANNEL_NPRIV);
    HAL_DMA_RegisterCallback(&g_spi_dma_handle[0].hdma_rx, HAL_DMA_XFER_CPLT_CB_ID, spi_dma_rx_cplt_callback);
    HAL_NVIC_SetPriority(GPDMA1_Channel6_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel6_IRQn);
    
    /* TX通道: GPDMA1_Channel3 */
    g_spi_dma_handle[0].hdma_tx.Instance = GPDMA1_Channel3;
    g_spi_dma_handle[0].hdma_tx.Init.Request = GPDMA1_REQUEST_SPI1_TX;
    g_spi_dma_handle[0].hdma_tx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    g_spi_dma_handle[0].hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    g_spi_dma_handle[0].hdma_tx.Init.SrcInc = DMA_SINC_INCREMENTED;
    g_spi_dma_handle[0].hdma_tx.Init.DestInc = DMA_DINC_FIXED;
    g_spi_dma_handle[0].hdma_tx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    g_spi_dma_handle[0].hdma_tx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    g_spi_dma_handle[0].hdma_tx.Init.Priority = DMA_HIGH_PRIORITY;
    g_spi_dma_handle[0].hdma_tx.Init.SrcBurstLength = 1;
    g_spi_dma_handle[0].hdma_tx.Init.DestBurstLength = 1;
    g_spi_dma_handle[0].hdma_tx.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    g_spi_dma_handle[0].hdma_tx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    g_spi_dma_handle[0].hdma_tx.Init.Mode = DMA_NORMAL;
    HAL_DMA_Init(&g_spi_dma_handle[0].hdma_tx);
    HAL_DMA_ConfigChannelAttributes(&g_spi_dma_handle[0].hdma_tx, DMA_CHANNEL_NPRIV);
    HAL_NVIC_SetPriority(GPDMA1_Channel3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel3_IRQn);
    
    /* ========== SPI2 DMA通道 ========== */
    /* RX通道: GPDMA1_Channel7 */
    g_spi_dma_handle[1].hdma_rx.Instance = GPDMA1_Channel7;
    g_spi_dma_handle[1].hdma_rx.Init.Request = GPDMA1_REQUEST_SPI2_RX;
    g_spi_dma_handle[1].hdma_rx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    g_spi_dma_handle[1].hdma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    g_spi_dma_handle[1].hdma_rx.Init.SrcInc = DMA_SINC_FIXED;
    g_spi_dma_handle[1].hdma_rx.Init.DestInc = DMA_DINC_INCREMENTED;
    g_spi_dma_handle[1].hdma_rx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    g_spi_dma_handle[1].hdma_rx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    g_spi_dma_handle[1].hdma_rx.Init.Priority = DMA_HIGH_PRIORITY;
    g_spi_dma_handle[1].hdma_rx.Init.SrcBurstLength = 1;
    g_spi_dma_handle[1].hdma_rx.Init.DestBurstLength = 1;
    g_spi_dma_handle[1].hdma_rx.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    g_spi_dma_handle[1].hdma_rx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    g_spi_dma_handle[1].hdma_rx.Init.Mode = DMA_NORMAL;
    HAL_DMA_Init(&g_spi_dma_handle[1].hdma_rx);
    HAL_DMA_ConfigChannelAttributes(&g_spi_dma_handle[1].hdma_rx, DMA_CHANNEL_NPRIV);
    HAL_DMA_RegisterCallback(&g_spi_dma_handle[1].hdma_rx, HAL_DMA_XFER_CPLT_CB_ID, spi_dma_rx_cplt_callback);
    HAL_NVIC_SetPriority(GPDMA1_Channel7_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel7_IRQn);
    
    /* TX通道: GPDMA1_Channel4 */
    g_spi_dma_handle[1].hdma_tx.Instance = GPDMA1_Channel4;
    g_spi_dma_handle[1].hdma_tx.Init.Request = GPDMA1_REQUEST_SPI2_TX;
    g_spi_dma_handle[1].hdma_tx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    g_spi_dma_handle[1].hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    g_spi_dma_handle[1].hdma_tx.Init.SrcInc = DMA_SINC_INCREMENTED;
    g_spi_dma_handle[1].hdma_tx.Init.DestInc = DMA_DINC_FIXED;
    g_spi_dma_handle[1].hdma_tx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    g_spi_dma_handle[1].hdma_tx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    g_spi_dma_handle[1].hdma_tx.Init.Priority = DMA_HIGH_PRIORITY;
    g_spi_dma_handle[1].hdma_tx.Init.SrcBurstLength = 1;
    g_spi_dma_handle[1].hdma_tx.Init.DestBurstLength = 1;
    g_spi_dma_handle[1].hdma_tx.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    g_spi_dma_handle[1].hdma_tx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    g_spi_dma_handle[1].hdma_tx.Init.Mode = DMA_NORMAL;
    HAL_DMA_Init(&g_spi_dma_handle[1].hdma_tx);
    HAL_DMA_ConfigChannelAttributes(&g_spi_dma_handle[1].hdma_tx, DMA_CHANNEL_NPRIV);
    HAL_NVIC_SetPriority(GPDMA1_Channel4_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel4_IRQn);
    
    /* ========== SPI3 (SPI4) DMA通道 ========== */
    /* RX通道: GPDMA1_Channel8 */
    g_spi_dma_handle[2].hdma_rx.Instance = GPDMA1_Channel8;
    g_spi_dma_handle[2].hdma_rx.Init.Request = GPDMA1_REQUEST_SPI4_RX;
    g_spi_dma_handle[2].hdma_rx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    g_spi_dma_handle[2].hdma_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    g_spi_dma_handle[2].hdma_rx.Init.SrcInc = DMA_SINC_FIXED;
    g_spi_dma_handle[2].hdma_rx.Init.DestInc = DMA_DINC_INCREMENTED;
    g_spi_dma_handle[2].hdma_rx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    g_spi_dma_handle[2].hdma_rx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    g_spi_dma_handle[2].hdma_rx.Init.Priority = DMA_HIGH_PRIORITY;
    g_spi_dma_handle[2].hdma_rx.Init.SrcBurstLength = 1;
    g_spi_dma_handle[2].hdma_rx.Init.DestBurstLength = 1;
    g_spi_dma_handle[2].hdma_rx.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    g_spi_dma_handle[2].hdma_rx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    g_spi_dma_handle[2].hdma_rx.Init.Mode = DMA_NORMAL;
    HAL_DMA_Init(&g_spi_dma_handle[2].hdma_rx);
    HAL_DMA_ConfigChannelAttributes(&g_spi_dma_handle[2].hdma_rx, DMA_CHANNEL_NPRIV);
    HAL_DMA_RegisterCallback(&g_spi_dma_handle[2].hdma_rx, HAL_DMA_XFER_CPLT_CB_ID, spi_dma_rx_cplt_callback);
    HAL_NVIC_SetPriority(GPDMA1_Channel8_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel8_IRQn);
    
    /* TX通道: GPDMA1_Channel5 */
    g_spi_dma_handle[2].hdma_tx.Instance = GPDMA1_Channel5;
    g_spi_dma_handle[2].hdma_tx.Init.Request = GPDMA1_REQUEST_SPI4_TX;
    g_spi_dma_handle[2].hdma_tx.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    g_spi_dma_handle[2].hdma_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    g_spi_dma_handle[2].hdma_tx.Init.SrcInc = DMA_SINC_INCREMENTED;
    g_spi_dma_handle[2].hdma_tx.Init.DestInc = DMA_DINC_FIXED;
    g_spi_dma_handle[2].hdma_tx.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    g_spi_dma_handle[2].hdma_tx.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    g_spi_dma_handle[2].hdma_tx.Init.Priority = DMA_HIGH_PRIORITY;
    g_spi_dma_handle[2].hdma_tx.Init.SrcBurstLength = 1;
    g_spi_dma_handle[2].hdma_tx.Init.DestBurstLength = 1;
    g_spi_dma_handle[2].hdma_tx.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    g_spi_dma_handle[2].hdma_tx.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    g_spi_dma_handle[2].hdma_tx.Init.Mode = DMA_NORMAL;
    HAL_DMA_Init(&g_spi_dma_handle[2].hdma_tx);
    HAL_DMA_ConfigChannelAttributes(&g_spi_dma_handle[2].hdma_tx, DMA_CHANNEL_NPRIV);
    HAL_NVIC_SetPriority(GPDMA1_Channel5_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel5_IRQn);
    
    /* 初始化状态 */
    for (uint8_t i = 0; i < 3; i++)
    {
        g_spi_dma_handle[i].state = SPI_DMA_IDLE;
    }
}

/**
 * @brief   启动单路SPI DMA传输
 * @param   spi_idx: SPI索引 (0-2)
 * @param   tx_data: 发送数据缓冲区
 * @param   rx_data: 接收数据缓冲区
 * @param   size: 传输字节数
 * @retval  无
 */
void spi_dma_start_transfer(uint8_t spi_idx, uint8_t *tx_data, uint8_t *rx_data, uint16_t size)
{
    SPI_TypeDef *SPIx;
    SPI_HandleTypeDef *hspi;
    uint8_t *tx_ptr;
    
    if (spi_idx > 2 || size == 0 || size > SPI_DMA_MAX_TRANSFER_SIZE)
        return;
    
    /* 获取SPI实例 */
    hspi = &g_spi_handle[spi_idx];
    SPIx = hspi->Instance;
    
    /* 设置传输大小 */
    MODIFY_REG(SPIx->CR2, SPI_CR2_TSIZE, size);
    
    /* 清除状态标志 */
    SPIx->IFCR = SPI_IFCR_EOTC | SPI_IFCR_TXTFC;
    
    /* 配置DMA状态 */
    g_spi_dma_handle[spi_idx].state = SPI_DMA_BUSY;
    g_spi_dma_handle[spi_idx].rx_buffer = rx_data;
    g_spi_dma_handle[spi_idx].tx_buffer = tx_data ? tx_data : g_default_tx_data;
    g_spi_dma_handle[spi_idx].transfer_size = size;
    
    /* 选择TX数据源 */
    tx_ptr = tx_data ? tx_data : g_default_tx_data;
    
    /* 启用SPI DMA */
    SET_BIT(SPIx->CFG1, SPI_CFG1_RXDMAEN | SPI_CFG1_TXDMAEN);
    
    /* 启动SPI传输 */
    __HAL_SPI_ENABLE(hspi);
    SET_BIT(SPIx->CR1, SPI_CR1_CSTART);
    
    /* 启动RX DMA传输 */
    HAL_DMA_Start_IT(&g_spi_dma_handle[spi_idx].hdma_rx, 
                     (uint32_t)&SPIx->RXDR, 
                     (uint32_t)rx_data, 
                     size);
    
    /* 启动TX DMA传输 */
    HAL_DMA_Start_IT(&g_spi_dma_handle[spi_idx].hdma_tx,
                     (uint32_t)tx_ptr,
                     (uint32_t)&SPIx->TXDR,
                     size);
}

/**
 * @brief   检查SPI DMA传输是否完成
 * @param   spi_idx: SPI索引 (0-2)
 * @retval  1=完成, 0=未完成
 */
uint8_t spi_dma_is_complete(uint8_t spi_idx)
{
    if (spi_idx > 2)
        return 1;
    
    return (g_spi_dma_handle[spi_idx].state == SPI_DMA_COMPLETE);
}

/**
 * @brief   等待SPI DMA传输完成
 * @param   spi_idx: SPI索引 (0-2)
 * @retval  无
 */
void spi_dma_wait_complete(uint8_t spi_idx)
{
    if (spi_idx > 2)
        return;
    
    while (g_spi_dma_handle[spi_idx].state == SPI_DMA_BUSY)
    {
        __NOP();
    }
}

/**
 * @brief   中止SPI DMA传输
 * @param   spi_idx: SPI索引 (0-2)
 * @retval  无
 */
void spi_dma_abort(uint8_t spi_idx)
{
    if (spi_idx > 2)
        return;
    
    SPI_HandleTypeDef *hspi = &g_spi_handle[spi_idx];
    
    HAL_DMA_Abort(&g_spi_dma_handle[spi_idx].hdma_rx);
    __HAL_SPI_DISABLE(hspi);
    g_spi_dma_handle[spi_idx].state = SPI_DMA_IDLE;
}

/**
 * @brief   同时启动三路SPI DMA传输
 * @param   tx_data: 发送数据缓冲区数组 [3][size]
 * @param   rx_data: 接收数据缓冲区数组 [3][size]
 * @param   size: 传输字节数数组 [3]
 * @retval  无
 */
void spi_dma_start_all(uint8_t tx_data[3][SPI_DMA_MAX_TRANSFER_SIZE],
                       uint8_t rx_data[3][SPI_DMA_MAX_TRANSFER_SIZE],
                       uint16_t size[3])
{
    /* 同时启动三路DMA传输 */
    for (uint8_t spi_idx = 0; spi_idx < 3; spi_idx++)
    {
        if (size[spi_idx] > 0)
        {
            spi_dma_start_transfer(spi_idx, 
                                   tx_data ? tx_data[spi_idx] : NULL,
                                   rx_data[spi_idx],
                                   size[spi_idx]);
        }
    }
}

/**
 * @brief   等待所有SPI DMA传输完成
 * @param   无
 * @retval  1=全部完成, 0=超时
 */
uint8_t spi_dma_wait_all_complete(void)
{
    uint32_t timeout = 100000;
    
    while (timeout--)
    {
        uint8_t all_complete = 1;
        for (uint8_t i = 0; i < 3; i++)
        {
            if (g_spi_dma_handle[i].state == SPI_DMA_BUSY)
            {
                all_complete = 0;
                break;
            }
        }
        if (all_complete)
            return 1;
    }
    
    return 0;
}

/**
 * @brief   SPI DMA传输完成回调
 * @param   hdma: DMA句柄
 * @retval  无
 */
void spi_dma_rx_cplt_callback(DMA_HandleTypeDef *hdma)
{
    for (uint8_t i = 0; i < 3; i++)
    {
        if (hdma == &g_spi_dma_handle[i].hdma_rx)
        {
            SPI_HandleTypeDef *hspi = &g_spi_handle[i];
            SPI_TypeDef *SPIx = hspi->Instance;
            
            /* 等待SPI传输完成，带超时 */
            uint32_t timeout = 10000;
            while (!(SPIx->SR & SPI_SR_EOT) && --timeout)
                ;
            
            /* 清除状态标志 */
            SPIx->IFCR = SPI_IFCR_EOTC | SPI_IFCR_TXTFC;
            
            /* 禁用DMA */
            CLEAR_BIT(SPIx->CFG1, SPI_CFG1_RXDMAEN | SPI_CFG1_TXDMAEN);
            
            /* 停止SPI */
            __HAL_SPI_DISABLE(hspi);
            
            /* 更新状态 */
            g_spi_dma_handle[i].state = SPI_DMA_COMPLETE;
            
            break;
        }
    }
}

/* DMA中断处理函数 */
/* GPDMA1_Channel0: UART DMA (dma_list.c) */
/* GPDMA1_Channel1: 未使用 */
/* GPDMA1_Channel2: 未使用 */

/* TX DMA中断处理函数 */
void GPDMA1_Channel3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_spi_dma_handle[0].hdma_tx);
}

void GPDMA1_Channel4_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_spi_dma_handle[1].hdma_tx);
}

void GPDMA1_Channel5_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_spi_dma_handle[2].hdma_tx);
}

/* RX DMA中断处理函数 */
void GPDMA1_Channel6_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_spi_dma_handle[0].hdma_rx);
}

void GPDMA1_Channel7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_spi_dma_handle[1].hdma_rx);
}

void GPDMA1_Channel8_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_spi_dma_handle[2].hdma_rx);
}