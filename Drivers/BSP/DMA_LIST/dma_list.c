/**
 * @file    dma_list.c
 * @brief   DMA list support for multi-SPI
 */
#include "./BSP/TIMER/gtim.h"
#include "./BSP/DMA_LIST/dma_list.h"
#include "string.h"

/* DMA handles */
DMA_HandleTypeDef g_handle_GPDMA1_Channel0 = {0};
DMA_HandleTypeDef g_handle_GPDMA1_Channel1 = {0};
DMA_HandleTypeDef g_handle_GPDMA1_Channel2 = {0};
DMA_HandleTypeDef g_handle_GPDMA1_Channel3 = {0};
DMA_HandleTypeDef g_handle_GPDMA1_Channel4 = {0};
DMA_HandleTypeDef g_handle_GPDMA1_Channel5 = {0};

static DMA_HandleTypeDef *g_dma_tx_handles[SPI_USED_MAX] = {
    &g_handle_GPDMA1_Channel0,
    &g_handle_GPDMA1_Channel2,
    &g_handle_GPDMA1_Channel4
};
static DMA_HandleTypeDef *g_dma_rx_handles[SPI_USED_MAX] = {
    &g_handle_GPDMA1_Channel1,
    &g_handle_GPDMA1_Channel3,
    &g_handle_GPDMA1_Channel5
};

static DMA_Channel_TypeDef *g_dma_tx_instances[SPI_USED_MAX] = {
    GPDMA1_Channel0,
    GPDMA1_Channel2,
    GPDMA1_Channel4
};
static DMA_Channel_TypeDef *g_dma_rx_instances[SPI_USED_MAX] = {
    GPDMA1_Channel1,
    GPDMA1_Channel3,
    GPDMA1_Channel5
};

static const IRQn_Type g_dma_tx_irqs[SPI_USED_MAX] = {
    GPDMA1_Channel0_IRQn,
    GPDMA1_Channel2_IRQn,
    GPDMA1_Channel4_IRQn
};
static const IRQn_Type g_dma_rx_irqs[SPI_USED_MAX] = {
    GPDMA1_Channel1_IRQn,
    GPDMA1_Channel3_IRQn,
    GPDMA1_Channel5_IRQn
};

static const uint32_t g_dma_tx_requests[SPI_USED_MAX] = {
    GPDMA1_REQUEST_SPI1_TX,
    GPDMA1_REQUEST_SPI2_TX,
    GPDMA1_REQUEST_SPI4_TX
};
static const uint32_t g_dma_rx_requests[SPI_USED_MAX] = {
    GPDMA1_REQUEST_SPI1_RX,
    GPDMA1_REQUEST_SPI2_RX,
    GPDMA1_REQUEST_SPI4_RX
};

/* DMA lists */
DMA_QListTypeDef g_dma_list_tx_struct[SPI_USED_MAX] = {0};
DMA_QListTypeDef g_dma_list_rx_struct[SPI_USED_MAX] = {0};

/* DMA list nodes */
__ALIGNED(32) DMA_NodeTypeDef g_dma_list_node_tx_struct[SPI_USED_MAX][DMA_SPI_TX_NODE_USED] = {0};
__ALIGNED(32) DMA_NodeTypeDef g_dma_list_node_rx_struct[SPI_USED_MAX][DMA_SPI_RX_NODE_USED] = {0};

/* DMA ready flag */
uint8_t dma_ready = 1;

/* RX buffers */
uint8_t spi_rx_dma_buf[SPI_USED_MAX][DMA_SPI_RX_NODE_USED][RX_BUFFER_SIZE];

/* SPI handles */
extern SPI_HandleTypeDef g_spi_handle[3];

/* Buffer managers */
BufferManager_t g_buffer_mgr[SPI_USED_MAX] = {0};

static volatile uint8_t g_dma_rx_done_mask = 0;

/* Forward declarations */
static void dma_transfer_complete_cb(DMA_HandleTypeDef *const hdma);
static int dma_get_spi_index_from_instance(DMA_Channel_TypeDef *instance, uint8_t *is_rx);
static void tx_list_config(uint8_t spi_idx);
static void rx_list_config(uint8_t spi_idx);

static int dma_get_spi_index_from_instance(DMA_Channel_TypeDef *instance, uint8_t *is_rx)
{
    for (uint8_t i = 0; i < SPI_USED_MAX; i++)
    {
        if (instance == g_dma_tx_instances[i])
        {
            *is_rx = 0;
            return i;
        }
    }
    for (uint8_t i = 0; i < SPI_USED_MAX; i++)
    {
        if (instance == g_dma_rx_instances[i])
        {
            *is_rx = 1;
            return i;
        }
    }
    return -1;
}

static void tx_list_config(uint8_t spi_idx)
{
    DMA_NodeConfTypeDef dma_node_conf_struct = {0};
    uint32_t node_index;
    DMA_QListTypeDef *tx_list = &g_dma_list_tx_struct[spi_idx];
    DMA_NodeTypeDef *tx_nodes = g_dma_list_node_tx_struct[spi_idx];

    __HAL_RCC_GPDMA1_CLK_ENABLE();

    HAL_DMAEx_List_ResetQ(tx_list);

    dma_node_conf_struct.NodeType = DMA_GPDMA_LINEAR_NODE;
    dma_node_conf_struct.Init.Request = g_dma_tx_requests[spi_idx];
    dma_node_conf_struct.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    dma_node_conf_struct.Init.Direction = DMA_MEMORY_TO_PERIPH;
    dma_node_conf_struct.Init.SrcInc = DMA_SINC_INCREMENTED;
    dma_node_conf_struct.Init.DestInc = DMA_DINC_FIXED;
    dma_node_conf_struct.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    dma_node_conf_struct.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    dma_node_conf_struct.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    dma_node_conf_struct.Init.SrcBurstLength = 1;
    dma_node_conf_struct.Init.DestBurstLength = 1;
    dma_node_conf_struct.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    dma_node_conf_struct.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    dma_node_conf_struct.Init.Mode = DMA_NORMAL;
    dma_node_conf_struct.DataHandlingConfig.DataExchange = DMA_EXCHANGE_NONE;
    dma_node_conf_struct.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;
    dma_node_conf_struct.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;
    dma_node_conf_struct.DstAddress = (uint32_t)&g_spi_handle[spi_idx].Instance->TXDR;

    memset(tx_list, 0x0, sizeof(*tx_list));

    for (node_index = 0; node_index < DMA_SPI_TX_NODE_USED; node_index++)
    {
        dma_node_conf_struct.SrcAddress = (uint32_t)&spi_tx_buffer[node_index];
        dma_node_conf_struct.DataSize = RX_BUFFER_SIZE;

        HAL_DMAEx_List_BuildNode(&dma_node_conf_struct, &tx_nodes[node_index]);
        SCB_CleanDCache_by_Addr((uint32_t *)&tx_nodes[node_index], sizeof(tx_nodes[node_index]));
        HAL_DMAEx_List_InsertNode_Tail(tx_list, &tx_nodes[node_index]);
    }
}

static void rx_list_config(uint8_t spi_idx)
{
    DMA_NodeConfTypeDef dma_node_conf_struct = {0};
    uint32_t node_index;
    DMA_QListTypeDef *rx_list = &g_dma_list_rx_struct[spi_idx];
    DMA_NodeTypeDef *rx_nodes = g_dma_list_node_rx_struct[spi_idx];

    __HAL_RCC_GPDMA1_CLK_ENABLE();

    HAL_DMAEx_List_ResetQ(rx_list);

    dma_node_conf_struct.NodeType = DMA_GPDMA_LINEAR_NODE;
    dma_node_conf_struct.Init.Request = g_dma_rx_requests[spi_idx];
    dma_node_conf_struct.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    dma_node_conf_struct.Init.Direction = DMA_PERIPH_TO_MEMORY;
    dma_node_conf_struct.Init.SrcInc = DMA_SINC_FIXED;
    dma_node_conf_struct.Init.DestInc = DMA_DINC_INCREMENTED;
    dma_node_conf_struct.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    dma_node_conf_struct.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    dma_node_conf_struct.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
    dma_node_conf_struct.Init.SrcBurstLength = 1;
    dma_node_conf_struct.Init.DestBurstLength = 1;
    dma_node_conf_struct.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    dma_node_conf_struct.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    dma_node_conf_struct.Init.Mode = DMA_NORMAL;
    dma_node_conf_struct.DataHandlingConfig.DataExchange = DMA_EXCHANGE_NONE;
    dma_node_conf_struct.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;
    dma_node_conf_struct.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;
    dma_node_conf_struct.SrcAddress = (uint32_t)&g_spi_handle[spi_idx].Instance->RXDR;

    memset(rx_list, 0x0, sizeof(*rx_list));

    for (node_index = 0; node_index < DMA_SPI_RX_NODE_USED; node_index++)
    {
        dma_node_conf_struct.DstAddress = (uint32_t)&spi_rx_dma_buf[spi_idx][node_index][0];
        dma_node_conf_struct.DataSize = RX_BUFFER_SIZE;

        HAL_DMAEx_List_BuildNode(&dma_node_conf_struct, &rx_nodes[node_index]);
        SCB_CleanDCache_by_Addr((uint32_t *)&rx_nodes[node_index], sizeof(rx_nodes[node_index]));
        HAL_DMAEx_List_InsertNode_Tail(rx_list, &rx_nodes[node_index]);
    }
}

void dma_list_rtx_init(void)
{
    for (uint8_t spi_idx = 0; spi_idx < SPI_USED_MAX; spi_idx++)
    {
        tx_list_config(spi_idx);
        rx_list_config(spi_idx);

        DMA_HandleTypeDef *hdma_tx = g_dma_tx_handles[spi_idx];
        hdma_tx->Instance = g_dma_tx_instances[spi_idx];
        hdma_tx->InitLinkedList.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
        hdma_tx->InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;
        hdma_tx->InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;
        hdma_tx->InitLinkedList.TransferEventMode = DMA_TCEM_LAST_LL_ITEM_TRANSFER;
        hdma_tx->InitLinkedList.LinkedListMode = DMA_LINKEDLIST_NORMAL;
        HAL_DMAEx_List_Init(hdma_tx);
        HAL_DMAEx_List_LinkQ(hdma_tx, &g_dma_list_tx_struct[spi_idx]);
        __HAL_LINKDMA(&g_spi_handle[spi_idx], hdmatx, *hdma_tx);
        HAL_DMA_ConfigChannelAttributes(hdma_tx, DMA_CHANNEL_NPRIV);
        HAL_DMA_RegisterCallback(hdma_tx, HAL_DMA_XFER_CPLT_CB_ID, dma_transfer_complete_cb);
        HAL_NVIC_SetPriority(g_dma_tx_irqs[spi_idx], 0, 0);
        HAL_NVIC_EnableIRQ(g_dma_tx_irqs[spi_idx]);

        DMA_HandleTypeDef *hdma_rx = g_dma_rx_handles[spi_idx];
        hdma_rx->Instance = g_dma_rx_instances[spi_idx];
        hdma_rx->InitLinkedList.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;
        hdma_rx->InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;
        hdma_rx->InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;
        hdma_rx->InitLinkedList.TransferEventMode = DMA_TCEM_LAST_LL_ITEM_TRANSFER;
        hdma_rx->InitLinkedList.LinkedListMode = DMA_LINKEDLIST_NORMAL;
        HAL_DMAEx_List_Init(hdma_rx);
        HAL_DMAEx_List_LinkQ(hdma_rx, &g_dma_list_rx_struct[spi_idx]);
        __HAL_LINKDMA(&g_spi_handle[spi_idx], hdmarx, *hdma_rx);
        HAL_DMA_ConfigChannelAttributes(hdma_rx, DMA_CHANNEL_NPRIV);
        HAL_DMA_RegisterCallback(hdma_rx, HAL_DMA_XFER_CPLT_CB_ID, dma_transfer_complete_cb);
        HAL_NVIC_SetPriority(g_dma_rx_irqs[spi_idx], 0, 0);
        HAL_NVIC_EnableIRQ(g_dma_rx_irqs[spi_idx]);
    }
}

void dma_start_transfer(void)
{
    dma_start_transfer_all();
}

void dma_start_transfer_all(void)
{
    if (!dma_ready)
        return;

    g_dma_rx_done_mask = 0;

    for (uint8_t spi_idx = 0; spi_idx < SPI_USED_MAX; spi_idx++)
    {
        __HAL_SPI_CLEAR_EOTFLAG(&g_spi_handle[spi_idx]);
        __HAL_SPI_CLEAR_TXTFFLAG(&g_spi_handle[spi_idx]);
        ATOMIC_SET_BIT(g_spi_handle[spi_idx].Instance->CFG1, SPI_CFG1_RXDMAEN | SPI_CFG1_TXDMAEN);
        SET_BIT(g_spi_handle[spi_idx].Instance->CR1, SPI_CR1_SPE);
    }

    for (uint8_t spi_idx = 0; spi_idx < SPI_USED_MAX; spi_idx++)
    {
        HAL_DMAEx_List_Start_IT(g_dma_rx_handles[spi_idx]);
    }
    for (uint8_t spi_idx = 0; spi_idx < SPI_USED_MAX; spi_idx++)
    {
        HAL_DMAEx_List_Start_IT(g_dma_tx_handles[spi_idx]);
    }

    for (uint8_t spi_idx = 0; spi_idx < SPI_USED_MAX; spi_idx++)
    {
        SET_BIT(g_spi_handle[spi_idx].Instance->CR1, SPI_CR1_CSTART);
    }
}

void dma_list_data_init(void)
{
    memset(&g_buffer_mgr[0], 0, sizeof(g_buffer_mgr));
    g_dma_rx_done_mask = 0;
}

static void dma_transfer_complete_cb(DMA_HandleTypeDef *const hdma)
{
    static uint32_t transfer_tx_count[SPI_USED_MAX] = {0};
    static uint32_t transfer_rx_count[SPI_USED_MAX] = {0};
    static uint32_t error_count[SPI_USED_MAX] = {0};
    static uint32_t rx_count_last[SPI_USED_MAX] = {0};

    if (__HAL_DMA_GET_FLAG(hdma, DMA_FLAG_DTE))
    {
        uint8_t is_rx = 0;
        int idx = dma_get_spi_index_from_instance(hdma->Instance, &is_rx);
        if (idx >= 0)
        {
            error_count[idx]++;
        }
        return;
    }

    uint8_t is_rx = 0;
    int spi_idx = dma_get_spi_index_from_instance(hdma->Instance, &is_rx);
    if (spi_idx < 0)
        return;

    if (!is_rx)
    {
        transfer_tx_count[spi_idx]++;
        return;
    }

    uint32_t current_node = g_buffer_mgr[spi_idx].current_rx_node;

    SCB_InvalidateDCache_by_Addr((uint32_t *)&spi_rx_dma_buf[spi_idx][current_node][0], RX_BUFFER_SIZE);

    g_buffer_mgr[spi_idx].current_rx_node = (g_buffer_mgr[spi_idx].current_rx_node + 1) % DMA_SPI_RX_NODE_USED;

    if ((g_buffer_mgr[spi_idx].current_rx_node != rx_count_last[spi_idx]) &&
        ((g_buffer_mgr[spi_idx].current_rx_node % 5) == 0))
    {
        g_buffer_mgr[spi_idx].data_ready = 1;
    }

    rx_count_last[spi_idx] = g_buffer_mgr[spi_idx].current_rx_node;
    transfer_rx_count[spi_idx]++;

    g_dma_rx_done_mask |= (1u << spi_idx);
    if (g_dma_rx_done_mask == ((1u << SPI_USED_MAX) - 1u))
    {
        ads8319_stop_transfer();
        g_dma_rx_done_mask = 0;
    }
}

uint8_t dma_get_ready_data(uint32_t *node_index)
{
    return dma_get_ready_data_by_spi(0, node_index);
}

uint8_t dma_get_ready_data_by_spi(uint8_t spi_idx, uint32_t *node_index)
{
    if (spi_idx >= SPI_USED_MAX)
        return 0;

    if (g_buffer_mgr[spi_idx].data_ready)
    {
        uint32_t ready_node = (g_buffer_mgr[spi_idx].current_rx_node == 0) ?
                              (DMA_SPI_RX_NODE_USED - 1) : (g_buffer_mgr[spi_idx].current_rx_node - 1);
        *node_index = ready_node;
        g_buffer_mgr[spi_idx].processed_node = ready_node;
        g_buffer_mgr[spi_idx].data_ready = 0;
        return 1;
    }
    return 0;
}

void GPDMA1_Channel0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_handle_GPDMA1_Channel0);
}

void GPDMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_handle_GPDMA1_Channel1);
}

void GPDMA1_Channel2_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_handle_GPDMA1_Channel2);
}

void GPDMA1_Channel3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_handle_GPDMA1_Channel3);
}

void GPDMA1_Channel4_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_handle_GPDMA1_Channel4);
}

void GPDMA1_Channel5_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_handle_GPDMA1_Channel5);
}

