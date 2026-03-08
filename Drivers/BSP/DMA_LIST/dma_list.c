/**
 ****************************************************************************************************
 * @file        dma_list.c
 * @brief       三路SPI并行DMA驱动
 *
 * 架构说明：
 *   CONVST拉高后，同时触发SPI1/SPI2/SPI4三路DMA传输。
 *   每路DMA RX完成后在中断回调中设置对应bit标志。
 *   当三路全部完成（g_spi_rx_done_flags == SPI_RX_DONE_ALL）时，
 *   统一处理数据并写入CircularBuffer。
 *
 * DMA通道分配：
 *   GPDMA1_Channel0 -> SPI1 TX
 *   GPDMA1_Channel1 -> SPI1 RX  ★
 *   GPDMA1_Channel2 -> SPI2 TX
 *   GPDMA1_Channel3 -> SPI2 RX  ★
 *   GPDMA1_Channel4 -> SPI4 TX
 *   GPDMA1_Channel5 -> SPI4 RX  ★
 *   （★ RX完成时触发中断）
 ****************************************************************************************************
 */

#include "./BSP/TIMER/gtim.h"
#include "./BSP/DMA_LIST/dma_list.h"
#include "./BSP/ADS8319/ads8319.h"
#include "collector_processor.h"
#include "string.h"

/*---------------------------------------------------------------------------*/
/* DMA 句柄                                                                   */
/*---------------------------------------------------------------------------*/
DMA_HandleTypeDef g_handle_GPDMA1_Channel0 = {0};  /* SPI1 TX */
DMA_HandleTypeDef g_handle_GPDMA1_Channel1 = {0};  /* SPI1 RX */
DMA_HandleTypeDef g_handle_GPDMA1_Channel2 = {0};  /* SPI2 TX */
DMA_HandleTypeDef g_handle_GPDMA1_Channel3 = {0};  /* SPI2 RX */
DMA_HandleTypeDef g_handle_GPDMA1_Channel4 = {0};  /* SPI4 TX */
DMA_HandleTypeDef g_handle_GPDMA1_Channel5 = {0};  /* SPI4 RX */

/*---------------------------------------------------------------------------*/
/* DMA 链表结构                                                               */
/*---------------------------------------------------------------------------*/
static DMA_QListTypeDef g_dma_list_tx1 = {0};
static DMA_QListTypeDef g_dma_list_rx1 = {0};
static DMA_QListTypeDef g_dma_list_tx2 = {0};
static DMA_QListTypeDef g_dma_list_rx2 = {0};
static DMA_QListTypeDef g_dma_list_tx3 = {0};
static DMA_QListTypeDef g_dma_list_rx3 = {0};

/*---------------------------------------------------------------------------*/
/* DMA 链表节点（32字节对齐，供Cache管理）                                    */
/*---------------------------------------------------------------------------*/
__ALIGNED(32) static DMA_NodeTypeDef g_node_tx1[DMA_SPI_TX_NODE_USED];
__ALIGNED(32) static DMA_NodeTypeDef g_node_rx1[DMA_SPI_RX_NODE_USED];
__ALIGNED(32) static DMA_NodeTypeDef g_node_tx2[DMA_SPI_TX_NODE_USED];
__ALIGNED(32) static DMA_NodeTypeDef g_node_rx2[DMA_SPI_RX_NODE_USED];
__ALIGNED(32) static DMA_NodeTypeDef g_node_tx3[DMA_SPI_TX_NODE_USED];
__ALIGNED(32) static DMA_NodeTypeDef g_node_rx3[DMA_SPI_RX_NODE_USED];

/*---------------------------------------------------------------------------*/
/* 接收缓冲区（32字节对齐，保证Cache操作正确）                                */
/*---------------------------------------------------------------------------*/
__ALIGNED(32) uint8_t spi_rx_buf0[DMA_SPI_RX_NODE_USED][RX_BUFFER_SIZE];  /* SPI1 */
__ALIGNED(32) uint8_t spi_rx_buf1[DMA_SPI_RX_NODE_USED][RX_BUFFER_SIZE];  /* SPI2 */
__ALIGNED(32) uint8_t spi_rx_buf2[DMA_SPI_RX_NODE_USED][RX_BUFFER_SIZE];  /* SPI4 */

/* TX发送缓冲区（全0xFF，仅产生时钟） */
static uint8_t s_tx_dummy[RX_BUFFER_SIZE];

/*---------------------------------------------------------------------------*/
/* 状态变量                                                                   */
/*---------------------------------------------------------------------------*/
BufferManager_t  g_buffer_mgr         = {0};
volatile uint8_t g_spi_rx_done_flags  = 0;   /* bit0=SPI1, bit1=SPI2, bit2=SPI4 */
static   uint8_t s_dma_initialized    = 0;

/*---------------------------------------------------------------------------*/
/* 内部函数声明                                                               */
/*---------------------------------------------------------------------------*/
static void spi_dma_rx_complete_cb(DMA_HandleTypeDef *hdma);
static void config_one_spi_dma(
    SPI_HandleTypeDef   *hspi,
    DMA_HandleTypeDef   *h_tx,   DMA_QListTypeDef *q_tx,
    DMA_NodeTypeDef     *node_tx, uint32_t dma_req_tx,
    IRQn_Type            irq_tx,
    DMA_HandleTypeDef   *h_rx,   DMA_QListTypeDef *q_rx,
    DMA_NodeTypeDef     *node_rx, uint32_t dma_req_rx,
    IRQn_Type            irq_rx,
    uint8_t (*rx_bufs)[RX_BUFFER_SIZE],
    DMA_Channel_TypeDef *ch_tx_inst,
    DMA_Channel_TypeDef *ch_rx_inst);

/*===========================================================================*/
/* 公共接口实现                                                               */
/*===========================================================================*/

/**
 * @brief  初始化缓冲区管理结构
 */
void dma_list_data_init(void)
{
    memset(&g_buffer_mgr, 0, sizeof(g_buffer_mgr));
    g_spi_rx_done_flags = 0;
    /* TX dummy buffer全部填0，仅驱动SPI时钟 */
    memset(s_tx_dummy, 0x00, sizeof(s_tx_dummy));
}

/**
 * @brief  初始化三路SPI并行DMA
 *         调用前须确保 spi_init() 已对三路SPI完成初始化
 */
void dma_list_rtx_init(void)
{
    extern SPI_HandleTypeDef g_spi_handle[3];

    __HAL_RCC_GPDMA1_CLK_ENABLE();

    /* ── SPI1: Channel0(TX) / Channel1(RX) ─────────────────────── */
    config_one_spi_dma(
        &g_spi_handle[0],
        &g_handle_GPDMA1_Channel0, &g_dma_list_tx1, g_node_tx1, GPDMA1_REQUEST_SPI1_TX,
        GPDMA1_Channel0_IRQn,
        &g_handle_GPDMA1_Channel1, &g_dma_list_rx1, g_node_rx1, GPDMA1_REQUEST_SPI1_RX,
        GPDMA1_Channel1_IRQn,
        spi_rx_buf0,
        GPDMA1_Channel0, GPDMA1_Channel1);

    /* ── SPI2: Channel2(TX) / Channel3(RX) ─────────────────────── */
    config_one_spi_dma(
        &g_spi_handle[1],
        &g_handle_GPDMA1_Channel2, &g_dma_list_tx2, g_node_tx2, GPDMA1_REQUEST_SPI2_TX,
        GPDMA1_Channel2_IRQn,
        &g_handle_GPDMA1_Channel3, &g_dma_list_rx2, g_node_rx2, GPDMA1_REQUEST_SPI2_RX,
        GPDMA1_Channel3_IRQn,
        spi_rx_buf1,
        GPDMA1_Channel2, GPDMA1_Channel3);

    /* ── SPI4: Channel4(TX) / Channel5(RX) ─────────────────────── */
    config_one_spi_dma(
        &g_spi_handle[2],
        &g_handle_GPDMA1_Channel4, &g_dma_list_tx3, g_node_tx3, GPDMA1_REQUEST_SPI4_TX,
        GPDMA1_Channel4_IRQn,
        &g_handle_GPDMA1_Channel5, &g_dma_list_rx3, g_node_rx3, GPDMA1_REQUEST_SPI4_RX,
        GPDMA1_Channel5_IRQn,
        spi_rx_buf2,
        GPDMA1_Channel4, GPDMA1_Channel5);

    s_dma_initialized = 1;
}

/**
 * @brief  同时触发三路SPI DMA传输（在TIM中断中调用）
 *         调用前须已完成 CONVST 等待（IRQ或足够NOP）
 */
void dma_start_transfer_all(void)
{
    extern SPI_HandleTypeDef g_spi_handle[3];

    if (!s_dma_initialized) return;

    /* 清除上次完成标志 */
    g_spi_rx_done_flags = 0;

    /* ── 预清理三路SPI状态 ─────────────────────────────────────── */
    __HAL_SPI_CLEAR_EOTFLAG(&g_spi_handle[0]);
    __HAL_SPI_CLEAR_TXTFFLAG(&g_spi_handle[0]);
    __HAL_SPI_CLEAR_EOTFLAG(&g_spi_handle[1]);
    __HAL_SPI_CLEAR_TXTFFLAG(&g_spi_handle[1]);
    __HAL_SPI_CLEAR_EOTFLAG(&g_spi_handle[2]);
    __HAL_SPI_CLEAR_TXTFFLAG(&g_spi_handle[2]);

    /* ── 重新挂载DMA链表（每次使用前重置到链表头） ──────────────── */
    HAL_DMAEx_List_LinkQ(&g_handle_GPDMA1_Channel1, &g_dma_list_rx1);
    HAL_DMAEx_List_LinkQ(&g_handle_GPDMA1_Channel0, &g_dma_list_tx1);
    HAL_DMAEx_List_LinkQ(&g_handle_GPDMA1_Channel3, &g_dma_list_rx2);
    HAL_DMAEx_List_LinkQ(&g_handle_GPDMA1_Channel2, &g_dma_list_tx2);
    HAL_DMAEx_List_LinkQ(&g_handle_GPDMA1_Channel5, &g_dma_list_rx3);
    HAL_DMAEx_List_LinkQ(&g_handle_GPDMA1_Channel4, &g_dma_list_tx3);

    /* ── 使能三路SPI的DMA请求 ───────────────────────────────────── */
    ATOMIC_SET_BIT(g_spi_handle[0].Instance->CFG1, SPI_CFG1_RXDMAEN | SPI_CFG1_TXDMAEN);
    ATOMIC_SET_BIT(g_spi_handle[1].Instance->CFG1, SPI_CFG1_RXDMAEN | SPI_CFG1_TXDMAEN);
    ATOMIC_SET_BIT(g_spi_handle[2].Instance->CFG1, SPI_CFG1_RXDMAEN | SPI_CFG1_TXDMAEN);

    /* ── 先启动全部RX DMA（必须在TX之前，避免错过第一个字节） ───── */
    HAL_DMAEx_List_Start_IT(&g_handle_GPDMA1_Channel1);  /* SPI1 RX */
    HAL_DMAEx_List_Start_IT(&g_handle_GPDMA1_Channel3);  /* SPI2 RX */
    HAL_DMAEx_List_Start_IT(&g_handle_GPDMA1_Channel5);  /* SPI4 RX */

    /* ── 再启动全部TX DMA ───────────────────────────────────────── */
    HAL_DMAEx_List_Start_IT(&g_handle_GPDMA1_Channel0);  /* SPI1 TX */
    HAL_DMAEx_List_Start_IT(&g_handle_GPDMA1_Channel2);  /* SPI2 TX */
    HAL_DMAEx_List_Start_IT(&g_handle_GPDMA1_Channel4);  /* SPI4 TX */

    /* ── 同时触发三路SPI传输开始（CSTART需SPI已使能） ──────────── */
    SET_BIT(g_spi_handle[0].Instance->CR1, SPI_CR1_CSTART);
    SET_BIT(g_spi_handle[1].Instance->CR1, SPI_CR1_CSTART);
    SET_BIT(g_spi_handle[2].Instance->CR1, SPI_CR1_CSTART);
}

/**
 * @brief  向后兼容旧接口，仅启动SPI1
 */
void dma_start_transfer(void)
{
    dma_start_transfer_all();
}

/**
 * @brief  获取当前就绪的接收节点索引
 * @param  node_index: 输出，就绪节点索引
 * @retval 1-有新数据可读, 0-尚无就绪数据
 */
uint8_t dma_get_ready_data(uint32_t *node_index)
{
    if (g_buffer_mgr.data_ready) {
        uint32_t ready_node = (g_buffer_mgr.current_rx_node == 0) ?
                              (DMA_SPI_RX_NODE_USED - 1) :
                              (g_buffer_mgr.current_rx_node - 1);
        *node_index = ready_node;
        g_buffer_mgr.processed_node = ready_node;
        g_buffer_mgr.data_ready = 0;
        return 1;
    }
    return 0;
}

/*===========================================================================*/
/* DMA RX 完成回调（ISR上下文）                                               */
/*===========================================================================*/

/**
 * @brief  三路SPI公共RX完成回调
 *         当三路全部完成后，统一解析数据并写入CircularBuffer
 */
static void spi_dma_rx_complete_cb(DMA_HandleTypeDef *hdma)
{
    extern SPI_HandleTypeDef g_spi_handle[3];
    extern CircularBuffer *g_cb_adc;

    uint32_t node = g_buffer_mgr.current_rx_node;

    /* 检查DMA传输错误 */
    if (__HAL_DMA_GET_FLAG(hdma, DMA_FLAG_DTE)) {
        return;
    }

    /* ── 标记对应路完成 ─────────────────────────────────────────── */
    if (hdma->Instance == GPDMA1_Channel1) {
        /* SPI1 RX完成：Cache Invalidate确保CPU读到最新数据 */
        SCB_InvalidateDCache_by_Addr(
            (uint32_t*)&spi_rx_buf0[node][0], RX_BUFFER_SIZE);
        g_spi_rx_done_flags |= SPI_RX_DONE_SPI1;
    }
    else if (hdma->Instance == GPDMA1_Channel3) {
        /* SPI2 RX完成 */
        SCB_InvalidateDCache_by_Addr(
            (uint32_t*)&spi_rx_buf1[node][0], RX_BUFFER_SIZE);
        g_spi_rx_done_flags |= SPI_RX_DONE_SPI2;
    }
    else if (hdma->Instance == GPDMA1_Channel5) {
        /* SPI4 RX完成 */
        SCB_InvalidateDCache_by_Addr(
            (uint32_t*)&spi_rx_buf2[node][0], RX_BUFFER_SIZE);
        g_spi_rx_done_flags |= SPI_RX_DONE_SPI3;
    }

    /* ── 三路全部完成才处理数据 ─────────────────────────────────── */
    if (g_spi_rx_done_flags != SPI_RX_DONE_ALL) {
        return;
    }

    /* 拉低CONVST，结束本轮采样 */
    ads8319_stop_transfer();

    /* ── 解析三路ADC数据（每路当前只用adc_num=1，即2字节） ─────── */
    /* 
     * 当前配置：adc_num=1（菊花链只读1个ADC），每路2字节
     * 若后续扩展为8个ADC菊花链，将 [0]改为循环解析即可
     */
    uint16_t ch[3];
    ch[0] = ((uint16_t)spi_rx_buf0[node][0] << 8) | spi_rx_buf0[node][1];
    ch[1] = ((uint16_t)spi_rx_buf1[node][0] << 8) | spi_rx_buf1[node][1];
    ch[2] = ((uint16_t)spi_rx_buf2[node][0] << 8) | spi_rx_buf2[node][1];

    /* 写入CircularBuffer（与原逻辑保持一致：3通道×2字节） */
    cb_write(g_cb_adc, (const char*)ch, sizeof(ch));

    /* ── 推进节点索引 ───────────────────────────────────────────── */
    g_buffer_mgr.current_rx_node =
        (g_buffer_mgr.current_rx_node + 1) % DMA_SPI_RX_NODE_USED;

    /* 标记数据就绪（每5个节点通知一次，与原逻辑一致） */
    if ((g_buffer_mgr.current_rx_node % 5) == 0) {
        g_buffer_mgr.data_ready = 1;
    }

    /* 清除完成标志，准备下一轮 */
    g_spi_rx_done_flags = 0;
}

/*===========================================================================*/
/* 内部辅助：配置单路SPI的TX+RX DMA链表                                      */
/*===========================================================================*/

static void config_one_spi_dma(
    SPI_HandleTypeDef   *hspi,
    DMA_HandleTypeDef   *h_tx,   DMA_QListTypeDef *q_tx,
    DMA_NodeTypeDef     *node_tx, uint32_t dma_req_tx,
    IRQn_Type            irq_tx,
    DMA_HandleTypeDef   *h_rx,   DMA_QListTypeDef *q_rx,
    DMA_NodeTypeDef     *node_rx, uint32_t dma_req_rx,
    IRQn_Type            irq_rx,
    uint8_t (*rx_bufs)[RX_BUFFER_SIZE],
    DMA_Channel_TypeDef *ch_tx_inst,
    DMA_Channel_TypeDef *ch_rx_inst)
{
    DMA_NodeConfTypeDef node_conf = {0};
    uint32_t i;

    /* ── TX 链表配置 ─────────────────────────────────────────────── */
    HAL_DMAEx_List_ResetQ(q_tx);
    memset(q_tx, 0, sizeof(*q_tx));

    node_conf.NodeType                          = DMA_GPDMA_LINEAR_NODE;
    node_conf.Init.Request                      = dma_req_tx;
    node_conf.Init.BlkHWRequest                 = DMA_BREQ_SINGLE_BURST;
    node_conf.Init.Direction                    = DMA_MEMORY_TO_PERIPH;
    node_conf.Init.SrcInc                       = DMA_SINC_INCREMENTED;
    node_conf.Init.DestInc                      = DMA_DINC_FIXED;
    node_conf.Init.SrcDataWidth                 = DMA_SRC_DATAWIDTH_BYTE;
    node_conf.Init.DestDataWidth                = DMA_DEST_DATAWIDTH_BYTE;
    node_conf.Init.Priority                     = DMA_HIGH_PRIORITY;
    node_conf.Init.SrcBurstLength               = 1;
    node_conf.Init.DestBurstLength              = 1;
    node_conf.Init.TransferAllocatedPort        = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    node_conf.Init.TransferEventMode            = DMA_TCEM_BLOCK_TRANSFER;
    node_conf.Init.Mode                         = DMA_NORMAL;
    node_conf.DataHandlingConfig.DataExchange   = DMA_EXCHANGE_NONE;
    node_conf.DataHandlingConfig.DataAlignment  = DMA_DATA_RIGHTALIGN_ZEROPADDED;
    node_conf.TriggerConfig.TriggerPolarity     = DMA_TRIG_POLARITY_MASKED;
    node_conf.DstAddress                        = (uint32_t)&hspi->Instance->TXDR;
    node_conf.SrcAddress                        = (uint32_t)s_tx_dummy;
    node_conf.DataSize                          = RX_BUFFER_SIZE;

    HAL_DMAEx_List_BuildNode(&node_conf, &node_tx[0]);
    SCB_CleanDCache_by_Addr((uint32_t*)&node_tx[0], sizeof(node_tx[0]));
    HAL_DMAEx_List_InsertNode_Tail(q_tx, &node_tx[0]);

    /* TX DMA通道初始化 */
    h_tx->Instance                               = ch_tx_inst;
    h_tx->InitLinkedList.Priority                = DMA_HIGH_PRIORITY;
    h_tx->InitLinkedList.LinkStepMode            = DMA_LSM_FULL_EXECUTION;
    h_tx->InitLinkedList.LinkAllocatedPort       = DMA_LINK_ALLOCATED_PORT0;
    h_tx->InitLinkedList.TransferEventMode       = DMA_TCEM_LAST_LL_ITEM_TRANSFER;
    h_tx->InitLinkedList.LinkedListMode          = DMA_LINKEDLIST_NORMAL;
    HAL_DMAEx_List_Init(h_tx);
    HAL_DMAEx_List_LinkQ(h_tx, q_tx);
    __HAL_LINKDMA(hspi, hdmatx, *h_tx);
    HAL_DMA_ConfigChannelAttributes(h_tx, DMA_CHANNEL_NPRIV);
    /* TX完成无需中断，不注册回调 */
    HAL_NVIC_SetPriority(irq_tx, 2, 0);
    HAL_NVIC_EnableIRQ(irq_tx);

    /* ── RX 链表配置 ─────────────────────────────────────────────── */
    HAL_DMAEx_List_ResetQ(q_rx);
    memset(q_rx, 0, sizeof(*q_rx));

    node_conf.Init.Request                      = dma_req_rx;
    node_conf.Init.Direction                    = DMA_PERIPH_TO_MEMORY;
    node_conf.Init.SrcInc                       = DMA_SINC_FIXED;
    node_conf.Init.DestInc                      = DMA_DINC_INCREMENTED;
    node_conf.SrcAddress                        = (uint32_t)&hspi->Instance->RXDR;

    for (i = 0; i < DMA_SPI_RX_NODE_USED; i++) {
        node_conf.DstAddress = (uint32_t)&rx_bufs[i][0];
        node_conf.DataSize   = RX_BUFFER_SIZE;
        HAL_DMAEx_List_BuildNode(&node_conf, &node_rx[i]);
        SCB_CleanDCache_by_Addr((uint32_t*)&node_rx[i], sizeof(node_rx[i]));
        HAL_DMAEx_List_InsertNode_Tail(q_rx, &node_rx[i]);
    }

    /* RX DMA通道初始化 */
    h_rx->Instance                               = ch_rx_inst;
    h_rx->InitLinkedList.Priority                = DMA_HIGH_PRIORITY;
    h_rx->InitLinkedList.LinkStepMode            = DMA_LSM_FULL_EXECUTION;
    h_rx->InitLinkedList.LinkAllocatedPort       = DMA_LINK_ALLOCATED_PORT0;
    h_rx->InitLinkedList.TransferEventMode       = DMA_TCEM_LAST_LL_ITEM_TRANSFER;
    h_rx->InitLinkedList.LinkedListMode          = DMA_LINKEDLIST_NORMAL;
    HAL_DMAEx_List_Init(h_rx);
    HAL_DMAEx_List_LinkQ(h_rx, q_rx);
    __HAL_LINKDMA(hspi, hdmarx, *h_rx);
    HAL_DMA_ConfigChannelAttributes(h_rx, DMA_CHANNEL_NPRIV);

    /* ★ 注册RX完成回调，用于三路并行汇聚判断 */
    HAL_DMA_RegisterCallback(h_rx, HAL_DMA_XFER_CPLT_CB_ID, spi_dma_rx_complete_cb);
    HAL_NVIC_SetPriority(irq_rx, 2, 0);
    HAL_NVIC_EnableIRQ(irq_rx);
}

/*===========================================================================*/
/* 中断服务函数                                                               */
/*===========================================================================*/

void GPDMA1_Channel0_IRQHandler(void) { HAL_DMA_IRQHandler(&g_handle_GPDMA1_Channel0); }
void GPDMA1_Channel1_IRQHandler(void) { HAL_DMA_IRQHandler(&g_handle_GPDMA1_Channel1); }
void GPDMA1_Channel2_IRQHandler(void) { HAL_DMA_IRQHandler(&g_handle_GPDMA1_Channel2); }
void GPDMA1_Channel3_IRQHandler(void) { HAL_DMA_IRQHandler(&g_handle_GPDMA1_Channel3); }
void GPDMA1_Channel4_IRQHandler(void) { HAL_DMA_IRQHandler(&g_handle_GPDMA1_Channel4); }
void GPDMA1_Channel5_IRQHandler(void) { HAL_DMA_IRQHandler(&g_handle_GPDMA1_Channel5); }