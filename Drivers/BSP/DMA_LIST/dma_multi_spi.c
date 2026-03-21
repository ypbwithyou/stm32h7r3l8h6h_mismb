/**
 * @file        dma_multi_spi.c
 * @brief       3路SPI DMA同步采集驱动实现
 * @note        支持SPI1/SPI2/SPI4同时DMA接收，用于AD8319多通道采集
 */

#include "./BSP/DMA_LIST/dma_multi_spi.h"
#include "./BSP/SPI/spi.h"
#include "./BSP/ADS8319/ads8319.h"
#include "./BSP/TIMER/gtim.h"
#include "collector_processor.h"
#include "string.h"

/* DMA通道数组 */
DMA_SPI_Channel_t g_dma_spi[DMA_SPI_NUM] = {0};

/* 外部声明 */
extern SPI_HandleTypeDef g_spi_handle[3];

/* DMA请求映射表 */
static const uint32_t dma_spi_rx_request[DMA_SPI_NUM] = {
    GPDMA1_REQUEST_SPI1_RX,
    GPDMA1_REQUEST_SPI2_RX,
    GPDMA1_REQUEST_SPI4_RX
};

static const uint32_t dma_spi_tx_request[DMA_SPI_NUM] = {
    GPDMA1_REQUEST_SPI1_TX,
    GPDMA1_REQUEST_SPI2_TX,
    GPDMA1_REQUEST_SPI4_TX
};

/* DMA通道映射表 */
static DMA_Channel_TypeDef * const dma_rx_channel[DMA_SPI_NUM] = {
    GPDMA1_Channel2,
    GPDMA1_Channel3,
    GPDMA1_Channel4
};

static DMA_Channel_TypeDef * const dma_tx_channel[DMA_SPI_NUM] = {
    GPDMA1_Channel5,
    GPDMA1_Channel6,
    GPDMA1_Channel7
};

/* DMA中断号映射 */
static const IRQn_Type dma_rx_irq[DMA_SPI_NUM] = {
    GPDMA1_Channel2_IRQn,
    GPDMA1_Channel3_IRQn,
    GPDMA1_Channel4_IRQn
};

/**
 * @brief   配置单个SPI的DMA接收通道
 * @param   spi_idx: SPI索引 (0-2)
 */
static void dma_spi_rx_config(uint8_t spi_idx)
{
    DMA_NodeConfTypeDef dma_node_conf = {0};
    DMA_SPI_Channel_t *ch = &g_dma_spi[spi_idx];

    ch->spi_idx = spi_idx;

    /* 配置DMA节点 */
    dma_node_conf.NodeType = DMA_GPDMA_LINEAR_NODE;
    dma_node_conf.Init.Request = dma_spi_rx_request[spi_idx];
    dma_node_conf.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    dma_node_conf.Init.Direction = DMA_PERIPH_TO_MEMORY;
    dma_node_conf.Init.SrcInc = DMA_SINC_FIXED;
    dma_node_conf.Init.DestInc = DMA_DINC_INCREMENTED;
    dma_node_conf.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    dma_node_conf.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    dma_node_conf.Init.Priority = DMA_HIGH_PRIORITY_HIGH_WEIGHT;
    dma_node_conf.Init.SrcBurstLength = 1;
    dma_node_conf.Init.DestBurstLength = 1;
    dma_node_conf.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT1;
    dma_node_conf.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    dma_node_conf.Init.Mode = DMA_NORMAL;
    dma_node_conf.DataHandlingConfig.DataExchange = DMA_EXCHANGE_NONE;
    dma_node_conf.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;
    dma_node_conf.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;
    dma_node_conf.SrcAddress = (uint32_t)&g_spi_handle[spi_idx].Instance->RXDR;
    dma_node_conf.DstAddress = (uint32_t)ch->rx_buf[0];
    dma_node_conf.DataSize = DMA_RX_BUF_SIZE;

    /* 初始化DMA句柄 */
    ch->hdma_rx.Instance = dma_rx_channel[spi_idx];
    ch->hdma_rx.InitLinkedList.Priority = DMA_HIGH_PRIORITY_HIGH_WEIGHT;
    ch->hdma_rx.InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;
    ch->hdma_rx.InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;
    ch->hdma_rx.InitLinkedList.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    ch->hdma_rx.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_NORMAL;

    HAL_DMAEx_List_Init(&ch->hdma_rx);
    HAL_DMA_ConfigChannelAttributes(&ch->hdma_rx, DMA_CHANNEL_NPRIV);

    /* 关联SPI与DMA */
    __HAL_LINKDMA(&g_spi_handle[spi_idx], hdmarx, ch->hdma_rx);

    /* 配置中断 */
    HAL_NVIC_SetPriority(dma_rx_irq[spi_idx], 0, 0);
    HAL_NVIC_EnableIRQ(dma_rx_irq[spi_idx]);
}

/**
 * @brief   配置单个SPI的DMA发送通道
 * @param   spi_idx: SPI索引 (0-2)
 */
static void dma_spi_tx_config(uint8_t spi_idx)
{
    DMA_NodeConfTypeDef dma_node_conf = {0};
    DMA_SPI_Channel_t *ch = &g_dma_spi[spi_idx];

    /* 发送缓冲区填充0xFF */
    memset(ch->tx_buf, 0xFF, DMA_RX_BUF_SIZE);

    /* 配置DMA节点 */
    dma_node_conf.NodeType = DMA_GPDMA_LINEAR_NODE;
    dma_node_conf.Init.Request = dma_spi_tx_request[spi_idx];
    dma_node_conf.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    dma_node_conf.Init.Direction = DMA_MEMORY_TO_PERIPH;
    dma_node_conf.Init.SrcInc = DMA_SINC_INCREMENTED;
    dma_node_conf.Init.DestInc = DMA_DINC_FIXED;
    dma_node_conf.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    dma_node_conf.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    dma_node_conf.Init.Priority = DMA_HIGH_PRIORITY_HIGH_WEIGHT;
    dma_node_conf.Init.SrcBurstLength = 1;
    dma_node_conf.Init.DestBurstLength = 1;
    dma_node_conf.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT1 | DMA_DEST_ALLOCATED_PORT0;
    dma_node_conf.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    dma_node_conf.Init.Mode = DMA_NORMAL;
    dma_node_conf.DataHandlingConfig.DataExchange = DMA_EXCHANGE_NONE;
    dma_node_conf.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;
    dma_node_conf.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;
    dma_node_conf.SrcAddress = (uint32_t)ch->tx_buf;
    dma_node_conf.DstAddress = (uint32_t)&g_spi_handle[spi_idx].Instance->TXDR;
    dma_node_conf.DataSize = DMA_RX_BUF_SIZE;

    /* 初始化DMA句柄 */
    ch->hdma_tx.Instance = dma_tx_channel[spi_idx];
    ch->hdma_tx.InitLinkedList.Priority = DMA_HIGH_PRIORITY_HIGH_WEIGHT;
    ch->hdma_tx.InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;
    ch->hdma_tx.InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;
    ch->hdma_tx.InitLinkedList.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    ch->hdma_tx.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_NORMAL;

    HAL_DMAEx_List_Init(&ch->hdma_tx);
    HAL_DMA_ConfigChannelAttributes(&ch->hdma_tx, DMA_CHANNEL_NPRIV);

    /* 关联SPI与DMA */
    __HAL_LINKDMA(&g_spi_handle[spi_idx], hdmatx, ch->hdma_tx);
}

/**
 * @brief   DMA接收完成回调
 * @param   hdma: DMA句柄
 */
static void dma_rx_complete_cb(DMA_HandleTypeDef *const hdma)
{
    /* 找到对应的SPI通道 */
    for (uint8_t i = 0; i < DMA_SPI_NUM; i++) {
        if (hdma == &g_dma_spi[i].hdma_rx) {
            DMA_SPI_Channel_t *ch = &g_dma_spi[i];

            /* Cache一致性处理 */
            SCB_InvalidateDCache_by_Addr((uint32_t*)ch->rx_buf[ch->buf_idx], DMA_RX_BUF_SIZE);

            /* 标记数据就绪 */
            ch->data_ready = 1;

            /* 切换到下一个缓冲区 */
            ch->buf_idx = (ch->buf_idx + 1) % DMA_DOUBLE_BUF_SIZE;

            return;
        }
    }
}

/**
 * @brief   初始化3路SPI DMA
 */
void dma_multi_spi_init(void)
{
    /* 使能GPDMA1时钟 */
    __HAL_RCC_GPDMA1_CLK_ENABLE();

    /* 配置3个SPI的DMA */
    for (uint8_t i = 0; i < DMA_SPI_NUM; i++) {
        dma_spi_rx_config(i);
        dma_spi_tx_config(i);

        /* 注册RX完成回调 */
        HAL_DMA_RegisterCallback(&g_dma_spi[i].hdma_rx, HAL_DMA_XFER_CPLT_CB_ID, dma_rx_complete_cb);
    }
}

/**
 * @brief   启动3路SPI DMA接收
 */
void dma_multi_spi_start(void)
{
    /* 启动ADC转换 */
    ads8319_start_convst();

    /* 同时启动3路SPI DMA接收 */
    for (uint8_t i = 0; i < DMA_SPI_NUM; i++) {
        DMA_SPI_Channel_t *ch = &g_dma_spi[i];

        /* 清除SPI标志 */
        __HAL_SPI_CLEAR_EOTFLAG(&g_spi_handle[i]);
        __HAL_SPI_CLEAR_TXTFFLAG(&g_spi_handle[i]);

        /* 使能SPI DMA */
        ATOMIC_SET_BIT(g_spi_handle[i].Instance->CFG1, SPI_CFG1_RXDMAEN | SPI_CFG1_TXDMAEN);

        /* 使能SPI */
        SET_BIT(g_spi_handle[i].Instance->CR1, SPI_CR1_SPE);

        /* 启动DMA传输 */
        HAL_DMAEx_List_Start_IT(&ch->hdma_rx);
        HAL_DMAEx_List_Start_IT(&ch->hdma_tx);

        /* 启动SPI传输 */
        SET_BIT(g_spi_handle[i].Instance->CR1, SPI_CR1_CSTART);
    }
}

/**
 * @brief   停止3路SPI DMA
 */
void dma_multi_spi_stop(void)
{
    ads8319_stop_transfer();

    for (uint8_t i = 0; i < DMA_SPI_NUM; i++) {
        /* 停止DMA */
        HAL_DMA_Abort(&g_dma_spi[i].hdma_rx);
        HAL_DMA_Abort(&g_dma_spi[i].hdma_tx);

        /* 停止SPI */
        CLEAR_BIT(g_spi_handle[i].Instance->CR1, SPI_CR1_SPE);
    }
}

/**
 * @brief   处理DMA接收的数据，写入环形缓冲区
 * @note    在定时器中断中调用
 */
void dma_multi_spi_process_data(void)
{
    /* 检查并处理每个SPI通道的数据 */
    for (uint8_t spi = 0; spi < DMA_SPI_NUM; spi++) {
        DMA_SPI_Channel_t *ch = &g_dma_spi[spi];

        if (!ch->data_ready)
            continue;

        /* 获取前一个缓冲区的索引（数据在前一个缓冲区） */
        uint8_t prev_idx = (ch->buf_idx == 0) ? (DMA_DOUBLE_BUF_SIZE - 1) : (ch->buf_idx - 1);
        uint8_t *data = ch->rx_buf[prev_idx];

        /* 解析ADC数据并写入环形缓冲区 */
        uint8_t adc_cnt = g_spi_adc_cnt[spi];
        for (uint8_t pos = 0; pos < adc_cnt; pos++) {
            uint16_t adc_val = (data[pos * 2] << 8) | data[pos * 2 + 1];
            uint8_t ch_id = pos * DMA_SPI_NUM + spi;

            if (ch_id < ADC_CH_TOTAL && (g_ch_enable_mask & (1u << ch_id))) {
                cb_write(g_cb_ch[ch_id], (const char *)&adc_val, ADC_DATA_LEN);
            }
        }

        ch->data_ready = 0;
    }
}

/**
 * @brief   DMA中断处理函数
 */
void GPDMA1_Channel2_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_dma_spi[0].hdma_rx);
}

void GPDMA1_Channel3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_dma_spi[1].hdma_rx);
}

void GPDMA1_Channel4_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_dma_spi[2].hdma_rx);
}
