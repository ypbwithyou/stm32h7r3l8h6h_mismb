/**
 ****************************************************************************************************
 * @file        dma.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       DMA驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 * 
 * 实验平台:正点原子 H7R3开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 * 
 ****************************************************************************************************
 */
#include "./BSP/TIMER/gtim.h"
#include "./BSP/DMA_LIST/dma_list.h"
#include "string.h"

/* DMA句柄 */
DMA_HandleTypeDef g_handle_GPDMA1_Channel0 = {0};
DMA_HandleTypeDef g_handle_GPDMA1_Channel1 = {0};

/* DMA链表 */
DMA_QListTypeDef g_dma_list_tx_struct = {0};
DMA_QListTypeDef g_dma_list_rx_struct = {0};

/* DMA链表节点 */
__ALIGNED(32) DMA_NodeTypeDef g_dma_list_node_tx_struct[DMA_SPI_TX_NODE_USED] = {0};
__ALIGNED(32) DMA_NodeTypeDef g_dma_list_node_rx_struct[DMA_SPI_RX_NODE_USED] = {0};

/* DMA就绪状态 */
uint8_t dma_ready = 1;

/* 发送、接收数据缓冲区 */
uint8_t spi_rx_buf0[DMA_SPI_RX_NODE_USED][RX_BUFFER_SIZE];

/* SPI1句柄 */
extern SPI_HandleTypeDef g_spi_handle[3];  /* SPI��� */

/* DMA传输完成回调函数 */
static void dma_transfer_complete_cb(DMA_HandleTypeDef *const hdma);

void tx_list_config(void)
{
    DMA_NodeConfTypeDef dma_node_conf_struct = {0};
    uint32_t node_index;
    
    /* 使能时钟 */
    __HAL_RCC_GPDMA1_CLK_ENABLE();
    
    /* 复位链表 */
    HAL_DMAEx_List_ResetQ(&g_dma_list_tx_struct);
    
    /* 配置DMA链表节点 */
    dma_node_conf_struct.NodeType = DMA_GPDMA_LINEAR_NODE;                                                  /* 节点类型 */
    dma_node_conf_struct.Init.Request = GPDMA1_REQUEST_SPI1_TX;                                             /* 通道请求 */
    dma_node_conf_struct.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;                                         /* 块硬件请求模式 */
    dma_node_conf_struct.Init.Direction = DMA_MEMORY_TO_PERIPH;                                             /* 传输方向 */
    dma_node_conf_struct.Init.SrcInc = DMA_SINC_INCREMENTED;                                                /* 传输源地址增量模式 */
    dma_node_conf_struct.Init.DestInc = DMA_DINC_FIXED;                                                     /* 传输目标地址固定模式 */
    dma_node_conf_struct.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;                                        /* 传输源数据宽度 */
    dma_node_conf_struct.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;                                      /* 传输目标数据宽度 */
    dma_node_conf_struct.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;                                       /* 优先级 */
    dma_node_conf_struct.Init.SrcBurstLength = 1;                                                           /* 传输源突发长度 */ 
    dma_node_conf_struct.Init.DestBurstLength = 1;                                                          /* 传输目标突发长度 */
    dma_node_conf_struct.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;   /* 传输端口分配 */
    dma_node_conf_struct.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;                                  /* 传输事件模式 */
    dma_node_conf_struct.Init.Mode = DMA_NORMAL;                                                            /* 传输模式 */
//    dma_node_conf_struct.Init.Mode = DMA_PFCTRL;                                                          /* 传输模式 */
    dma_node_conf_struct.DataHandlingConfig.DataExchange = DMA_EXCHANGE_NONE;                               /* 数据交换模式 */
    dma_node_conf_struct.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;                 /* 数据填充和对齐模式 */
    dma_node_conf_struct.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;                          /* 触发事件优先级 */
    dma_node_conf_struct.DstAddress = (uint32_t)&g_spi_handle[0].Instance->TXDR;                            /* 目的地址 */
    
    /* 清空链表 */
    memset(&g_dma_list_tx_struct, 0x0, sizeof(g_dma_list_tx_struct));

    for (node_index = 0; node_index < DMA_SPI_TX_NODE_USED; node_index++)
    {
        dma_node_conf_struct.SrcAddress = (uint32_t)&spi_tx_buffer[node_index];                                              /* 源地址 */ 
        dma_node_conf_struct.DataSize = RX_BUFFER_SIZE;                                                /* 数据大小 */
        
        /* 构建DMA链表节点 */
        HAL_DMAEx_List_BuildNode(&dma_node_conf_struct, &g_dma_list_node_tx_struct[node_index]);
        
        /* 添加Cache清理 - 确保DMA能读到正确的节点描述符 */
        SCB_CleanDCache_by_Addr((uint32_t*)&g_dma_list_node_tx_struct[node_index], sizeof(g_dma_list_node_tx_struct[node_index]));
        
        /* DMA链表节点插入链表 */
        HAL_DMAEx_List_InsertNode_Tail(&g_dma_list_tx_struct, &g_dma_list_node_tx_struct[node_index]);    
    }
}

void rx_list_config(void)
{
    DMA_NodeConfTypeDef dma_node_conf_struct = {0};
    uint32_t node_index;
    
    /* 使能时钟 */
    __HAL_RCC_GPDMA1_CLK_ENABLE();
    
    /* 复位链表 */
    HAL_DMAEx_List_ResetQ(&g_dma_list_rx_struct);
    
    /* 配置DMA链表节点 */
    dma_node_conf_struct.NodeType = DMA_GPDMA_LINEAR_NODE;                                                  /* 节点类型 */
    dma_node_conf_struct.Init.Request = GPDMA1_REQUEST_SPI1_RX;                                             /* 通道请求 */
    dma_node_conf_struct.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;                                         /* 块硬件请求模式 */
    dma_node_conf_struct.Init.Direction = DMA_PERIPH_TO_MEMORY;                                             /* 传输方向 */
    dma_node_conf_struct.Init.SrcInc = DMA_SINC_FIXED;                                                      /* 传输源地址固定模式 */
    dma_node_conf_struct.Init.DestInc = DMA_DINC_INCREMENTED;                                               /* 传输目标地址增量模式 */
    dma_node_conf_struct.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;                                        /* 传输源数据宽度 */
    dma_node_conf_struct.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;                                      /* 传输目标数据宽度 */
    dma_node_conf_struct.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;                                       /* 优先级 */
    dma_node_conf_struct.Init.SrcBurstLength = 1;                                                           /* 传输源突发长度 */ 
    dma_node_conf_struct.Init.DestBurstLength = 1;                                                          /* 传输目标突发长度 */
    dma_node_conf_struct.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;   /* 传输端口分配 */
    dma_node_conf_struct.Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;                                  /* 传输事件模式 */
    dma_node_conf_struct.Init.Mode = DMA_NORMAL;                                                            /* 传输模式 */
//    dma_node_conf_struct.Init.Mode = DMA_PFCTRL;                                                            /* 传输模式 */
    dma_node_conf_struct.DataHandlingConfig.DataExchange = DMA_EXCHANGE_NONE;                               /* 数据交换模式 */
    dma_node_conf_struct.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;                 /* 数据填充和对齐模式 */
    dma_node_conf_struct.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;                          /* 触发事件优先级 */
    dma_node_conf_struct.SrcAddress = (uint32_t)&g_spi_handle[0].Instance->RXDR;                            /* 源地址 */

    /* 清空链表 */
    memset(&g_dma_list_rx_struct, 0x0, sizeof(g_dma_list_rx_struct));
    
    for (node_index = 0; node_index < DMA_SPI_RX_NODE_USED; node_index++)
    {
        dma_node_conf_struct.DstAddress = (uint32_t)&spi_rx_buf0[node_index][0];                        /* 目的地址 */
        dma_node_conf_struct.DataSize = RX_BUFFER_SIZE;                                                /* 数据大小 */
        
        /* 构建DMA链表节点 */
        HAL_DMAEx_List_BuildNode(&dma_node_conf_struct, &g_dma_list_node_rx_struct[node_index]);
        
        /* 添加Cache清理 */
        SCB_CleanDCache_by_Addr((uint32_t*)&g_dma_list_node_rx_struct[node_index], sizeof(g_dma_list_node_rx_struct[node_index]));
        
        /* DMA链表节点插入链表 */
        HAL_DMAEx_List_InsertNode_Tail(&g_dma_list_rx_struct, &g_dma_list_node_rx_struct[node_index]);    
    }
}

/**
 * @brief   初始化DMA
 * @param   bufaddr: 缓冲区地址缓冲区的指针
 * @param   bufsize: 缓冲区大小缓冲区的指针
 * @param   bufnum: 缓冲区数量
 * @retval  无
 */
void dma_list_rtx_init(void)
{
    tx_list_config();
    
    /* 初始化链表模式DMA */
    g_handle_GPDMA1_Channel0.Instance = GPDMA1_Channel0;
    g_handle_GPDMA1_Channel0.InitLinkedList.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;                                     /* 优先级 */
    g_handle_GPDMA1_Channel0.InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;                                      /* 步进模式 */
    g_handle_GPDMA1_Channel0.InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;                               /* 端口分配 */
//    g_handle_GPDMA1_Channel0.InitLinkedList.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;                         /* 触发事件模式 */
    g_handle_GPDMA1_Channel0.InitLinkedList.TransferEventMode = DMA_TCEM_LAST_LL_ITEM_TRANSFER;                         /* 触发事件模式 */
    g_handle_GPDMA1_Channel0.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_NORMAL;                                     /* 链表传输模式 */
//    g_handle_GPDMA1_Channel0.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_CIRCULAR;                                     /* 链表传输模式 */
    
    HAL_DMAEx_List_Init(&g_handle_GPDMA1_Channel0);
    
    /* 关联DMA与DMA链表 */
    HAL_DMAEx_List_LinkQ(&g_handle_GPDMA1_Channel0, &g_dma_list_tx_struct);
    
    /* 关联外设与DMA */
    __HAL_LINKDMA(&g_spi_handle[0], hdmatx, g_handle_GPDMA1_Channel0);
    
    /* 配置通道属性 */
    HAL_DMA_ConfigChannelAttributes(&g_handle_GPDMA1_Channel0, DMA_CHANNEL_NPRIV);
    
    /* 注册DMA传输完成回调函数 */
    HAL_DMA_RegisterCallback(&g_handle_GPDMA1_Channel0, HAL_DMA_XFER_CPLT_CB_ID, dma_transfer_complete_cb);
    
    /* 配置中断优先级并使能中断 */
    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);

    

    rx_list_config();
    
    /* 初始化链表模式DMA */
    g_handle_GPDMA1_Channel1.Instance = GPDMA1_Channel1;
    g_handle_GPDMA1_Channel1.InitLinkedList.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;                                     /* 优先级 */
    g_handle_GPDMA1_Channel1.InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;                                      /* 步进模式 */
    g_handle_GPDMA1_Channel1.InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;                               /* 端口分配 */
//    g_handle_GPDMA1_Channel1.InitLinkedList.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;                              /* 触发事件模式 */
    g_handle_GPDMA1_Channel1.InitLinkedList.TransferEventMode = DMA_TCEM_LAST_LL_ITEM_TRANSFER;                         /* 触发事件模式 */
    g_handle_GPDMA1_Channel1.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_NORMAL;                                     /* 链表传输模式 */
//    g_handle_GPDMA1_Channel1.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_CIRCULAR;                                 /* 链表循环传输模式 */
    
    HAL_DMAEx_List_Init(&g_handle_GPDMA1_Channel1);
    
    /* 关联DMA与DMA链表 */
    HAL_DMAEx_List_LinkQ(&g_handle_GPDMA1_Channel1, &g_dma_list_rx_struct);
    
    /* 关联外设与DMA */
    __HAL_LINKDMA(&g_spi_handle[0], hdmarx, g_handle_GPDMA1_Channel1);
    
    /* 配置通道属性 */
    HAL_DMA_ConfigChannelAttributes(&g_handle_GPDMA1_Channel1, DMA_CHANNEL_NPRIV);
    
    /* 注册DMA传输完成回调函数 */
    HAL_DMA_RegisterCallback(&g_handle_GPDMA1_Channel1, HAL_DMA_XFER_CPLT_CB_ID, dma_transfer_complete_cb);
    
    /* 配置中断优先级并使能中断 */
    HAL_NVIC_SetPriority(GPDMA1_Channel1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel1_IRQn);
}

/**
 * @brief   DMA传输完成回调函数
 * @param   无
 * @retval  无
 */
void dma_start_transfer(void)
{
    /* 循环模式下只需要启动一次 */
    if (dma_ready) {
        /* 1. 先停止之前的传输（确保状态干净） */
        
        /* 2. 清理SPI状态标志 */
        __HAL_SPI_CLEAR_EOTFLAG(&g_spi_handle[0]);
        __HAL_SPI_CLEAR_TXTFFLAG(&g_spi_handle[0]);
        
        /* 3. 配置SPI的DMA请求使能 */
        ATOMIC_SET_BIT(g_spi_handle[0].Instance->CFG1, SPI_CFG1_RXDMAEN | SPI_CFG1_TXDMAEN);
        
        /* 4. 使能SPI（但不开始传输） */
        SET_BIT(g_spi_handle[0].Instance->CR1, SPI_CR1_SPE);
        
        /* 5. 启动DMA传输 - 循环模式会自动重复 */
        HAL_DMAEx_List_Start_IT(&g_handle_GPDMA1_Channel1);  // 先启动RX
        HAL_DMAEx_List_Start_IT(&g_handle_GPDMA1_Channel0);  // 再启动TX
        
        /* 6. 最后触发SPI传输开始 */
        SET_BIT(g_spi_handle[0].Instance->CR1, SPI_CR1_CSTART);        
        
        // dma_ready = 0;  // 重置标志
    }
}
/* 缓冲区管理结构 */
//typedef struct {
//    volatile uint32_t current_rx_node;    // 当前接收节点
//    volatile uint32_t current_tx_node;    // 当前发送节点  
//    volatile uint32_t processed_node;     // 已处理的节点
//    volatile uint8_t data_ready;          // 数据就绪标志
//} BufferManager_t;

BufferManager_t g_buffer_mgr = {0};

void dma_list_data_init(void)
{
    memset(&g_buffer_mgr, 0, sizeof(g_buffer_mgr));
}
/**
 * @brief   DMA传输完成回调函数
 * @param   hdma: DMA句柄指针
 * @retval  无
 */
static void dma_transfer_complete_cb(DMA_HandleTypeDef *const hdma)
{
    /* 在循环模式下，每次传输完成都会进入这个回调 */
    static uint32_t transfer_tx_count = 0;
    static uint32_t transfer_rx_count = 0;
    static uint32_t error_count = 0;
    static uint32_t rx_count_last = 0;
    uint32_t current_node = 0;
    uint16_t adc_data_1[8];
    
    /* 检查DMA错误标志 */
    if (__HAL_DMA_GET_FLAG(hdma, DMA_FLAG_DTE)) {
        // 传输错误
        // 可以在这里添加错误处理
        error_count++;
        return;
    }
    
    // 传输完成
    if (hdma->Instance == GPDMA1_Channel0) 
    {
        transfer_tx_count++;
        // printf("TX DMA Complete\n");
//        if (transfer_tx_count >= 1000) {  
//            transfer_tx_count = 0;
//        }
    }
    if (hdma->Instance == GPDMA1_Channel1) 
    {   
        // 处理接收到的数据
        ads8319_stop_transfer();
        
        current_node = g_buffer_mgr.current_rx_node;

        /* Cache一致性处理 */
        SCB_InvalidateDCache_by_Addr((uint32_t*)&spi_rx_buf0[current_node][0], RX_BUFFER_SIZE);

        /* 切换到下一个接收缓冲区 */
        g_buffer_mgr.current_rx_node = (g_buffer_mgr.current_rx_node + 1) % DMA_SPI_RX_NODE_USED;

        /* 设置数据就绪标志 */
        if ((g_buffer_mgr.current_rx_node != rx_count_last) && ((g_buffer_mgr.current_rx_node % 5) == 0)) {
            g_buffer_mgr.data_ready = 1;
        }

        // /* 接收数据移位处理 */
        // extract_adc_data_from_buffer(&spi_rx_buf0[current_node][0], adc_data_1);
        // uint32_t stop_time = ticks_timx_get_counter();    
        
        // rx_count_last = g_buffer_mgr.current_rx_node;
        
        transfer_rx_count++;
        // 可以根据transfer_count判断传输了多少次
//        if (transfer_rx_count >= 1000) {  
//            transfer_rx_count = 0;
//        }
    }
}

/**
 * @brief   获取当前可用的接收数据
 * @param   node_index: 返回数据所在的节点索引
 * @retval  1-有新数据, 0-无新数据
 */
uint8_t dma_get_ready_data(uint32_t *node_index)
{
    if (g_buffer_mgr.data_ready) {
        /* 计算刚刚完成的节点（current_rx_node的前一个） */
        uint32_t ready_node = (g_buffer_mgr.current_rx_node == 0) ? 
                             (DMA_SPI_RX_NODE_USED - 1) : (g_buffer_mgr.current_rx_node - 1);
        *node_index = ready_node;
        g_buffer_mgr.processed_node = ready_node;
        g_buffer_mgr.data_ready = 0;
        return 1;
    }
    return 0;
}

/**
 * @brief   GPDMA1 Channel0中断服务函数
 * @param   无
 * @retval  无
 */
void GPDMA1_Channel0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_handle_GPDMA1_Channel0);
}

void GPDMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_handle_GPDMA1_Channel1);
}
