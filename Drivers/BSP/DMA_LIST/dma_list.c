 
#include "./BSP/DMA/dma.h"

/* DMA句柄 */
DMA_HandleTypeDef g_dma_handle = {0};

/* DMA链表 */
DMA_QListTypeDef g_dma_qlist_struct = {0};

/* DMA链表节点 */
DMA_NodeTypeDef g_dma_node_struct[DMA_MAX_NODE] = {0};

/* DMA链表节点使用量 */
uint32_t node_used = 0;

/* DMA就绪状态 */
uint8_t dma_ready = 1;

/* UART句柄 */
extern UART_HandleTypeDef g_uart1_handle;

/* DMA传输完成回调函数 */
static void dma_transfer_complete_cb(DMA_HandleTypeDef *const hdma);

/**
 * @brief   初始化DMA
 * @param   bufaddr: 缓冲区地址缓冲区的指针
 * @param   bufsize: 缓冲区大小缓冲区的指针
 * @param   bufnum: 缓冲区数量
 * @retval  无
 */
void dma_init(uint32_t *bufaddr, uint32_t *bufsize, uint32_t bufnum)
{
    DMA_NodeConfTypeDef dma_node_conf_struct = {0};
    uint32_t node_index;
    
    if (bufnum > (sizeof(g_dma_node_struct) / sizeof(g_dma_node_struct[0])))
    {
        node_used = sizeof(g_dma_node_struct) / sizeof(g_dma_node_struct[0]);
    }
    else
    {
        node_used = bufnum;
    }
    
    /* 使能时钟 */
    __HAL_RCC_GPDMA1_CLK_ENABLE();
    
    /* 复位链表 */
    HAL_DMAEx_List_ResetQ(&g_dma_qlist_struct);
    
    /* 配置DMA链表节点 */
    dma_node_conf_struct.NodeType = DMA_GPDMA_LINEAR_NODE;                                                  /* 节点类型 */
    dma_node_conf_struct.Init.Request = GPDMA1_REQUEST_USART1_TX;                                           /* 通道请求 */
    dma_node_conf_struct.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;                                         /* 块硬件请求模式 */
    dma_node_conf_struct.Init.Direction = DMA_MEMORY_TO_PERIPH;                                             /* 传输方向 */
    dma_node_conf_struct.Init.SrcInc = DMA_SINC_INCREMENTED;                                                /* 传输源地址增量模式 */
    dma_node_conf_struct.Init.DestInc = DMA_DINC_FIXED;                                                     /* 传输目标地址增量模式 */
    dma_node_conf_struct.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;                                        /* 传输源数据宽度 */
    dma_node_conf_struct.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;                                      /* 传输目标数据宽度 */
    dma_node_conf_struct.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;                                       /* 优先级 */
    dma_node_conf_struct.Init.SrcBurstLength = 1;                                                           /* 传输源突发长度 */ 
    dma_node_conf_struct.Init.DestBurstLength = 1;                                                          /* 传输目标突发长度 */
    dma_node_conf_struct.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;   /* 传输端口分配 */
    dma_node_conf_struct.Init.TransferEventMode = DMA_TCEM_LAST_LL_ITEM_TRANSFER;                           /* 传输事件模式 */
    dma_node_conf_struct.Init.Mode = DMA_NORMAL;                                                            /* 传输模式 */
    dma_node_conf_struct.DataHandlingConfig.DataExchange = DMA_EXCHANGE_NONE;                               /* 数据交换模式 */
    dma_node_conf_struct.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;                 /* 数据填充和对齐模式 */
    dma_node_conf_struct.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;                          /* 触发事件优先级 */
    dma_node_conf_struct.DstAddress = (uint32_t)&g_uart1_handle.Instance->TDR;                              /* 目的地址 */
    for (node_index = 0; node_index < node_used; node_index++)
    {
        dma_node_conf_struct.SrcAddress = bufaddr[node_index];                                              /* 源地址 */
        dma_node_conf_struct.DataSize = bufsize[node_index];                                                /* 数据大小 */
        
        /* 构建DMA链表节点 */
        HAL_DMAEx_List_BuildNode(&dma_node_conf_struct, &g_dma_node_struct[node_index]);
        
        /* DMA链表节点插入链表 */
        HAL_DMAEx_List_InsertNode_Tail(&g_dma_qlist_struct, &g_dma_node_struct[node_index]);
    }
    
    /* 初始化链表模式DMA */
    g_dma_handle.Instance = GPDMA1_Channel0;
    g_dma_handle.InitLinkedList.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;                                     /* 优先级 */
    g_dma_handle.InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;                                      /* 步进模式 */
    g_dma_handle.InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;                               /* 端口分配 */
    g_dma_handle.InitLinkedList.TransferEventMode = DMA_TCEM_LAST_LL_ITEM_TRANSFER;                         /* 触发事件模式 */
    g_dma_handle.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_NORMAL;                                     /* 链表传输模式 */
    HAL_DMAEx_List_Init(&g_dma_handle);
    
    /* 关联DMA与DMA链表 */
    HAL_DMAEx_List_LinkQ(&g_dma_handle, &g_dma_qlist_struct);
    
    /* 关联外设与DMA */
    __HAL_LINKDMA(&g_uart1_handle, hdmatx, g_dma_handle);
    
    /* 配置通道属性 */
    HAL_DMA_ConfigChannelAttributes(&g_dma_handle, DMA_CHANNEL_NPRIV);
    
    /* 注册DMA传输完成回调函数 */
    HAL_DMA_RegisterCallback(&g_dma_handle, HAL_DMA_XFER_CPLT_CB_ID, dma_transfer_complete_cb);
    
    /* 配置中断优先级并使能中断 */
    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);
}

/**
 * @brief   DMA传输完成回调函数
 * @param   无
 * @retval  无
 */
void dma_start_transfer(void)
{
    if (dma_ready == 1)
    {
        dma_ready = 0;
        
        /* 开启中断模式的DMA链表传输 */
        HAL_DMAEx_List_Start_IT(&g_dma_handle);
        
        /* 使能UART的DMA发送 */
        ATOMIC_SET_BIT(g_uart1_handle.Instance->CR3, USART_CR3_DMAT);
    }
}

/**
 * @brief   DMA传输完成回调函数
 * @param   hdma: DMA句柄指针
 * @retval  无
 */
static void dma_transfer_complete_cb(DMA_HandleTypeDef *const hdma)
{
    if (hdma->Instance == GPDMA1_Channel0)
    {
        dma_ready = 1;
    }
}

/**
 * @brief   GPDMA1 Channel0中断服务函数
 * @param   无
 * @retval  无
 */
void GPDMA1_Channel0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_dma_handle);
}
