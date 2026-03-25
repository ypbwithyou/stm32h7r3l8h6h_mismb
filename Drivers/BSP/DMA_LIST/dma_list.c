 
#include "./BSP/DMA/dma.h"

/* DMA��� */
DMA_HandleTypeDef g_dma_handle = {0};

/* DMA���� */
DMA_QListTypeDef g_dma_qlist_struct = {0};

/* DMA�����ڵ� */
DMA_NodeTypeDef g_dma_node_struct[DMA_MAX_NODE] = {0};

/* DMA�����ڵ�ʹ���� */
uint32_t node_used = 0;

/* DMA����״̬ */
uint8_t dma_ready = 1;

/* UART��� */
extern UART_HandleTypeDef g_uart1_handle;

/* DMA������ɻص����� */
static void dma_transfer_complete_cb(DMA_HandleTypeDef *const hdma);

/**
 * @brief   ��ʼ��DMA
 * @param   bufaddr: ��������ַ��������ָ��
 * @param   bufsize: ��������С��������ָ��
 * @param   bufnum: ����������
 * @retval  ��
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
    
    /* ʹ��ʱ�� */
    __HAL_RCC_GPDMA1_CLK_ENABLE();
    
    /* ��λ���� */
    HAL_DMAEx_List_ResetQ(&g_dma_qlist_struct);
    
    /* ����DMA�����ڵ� */
    dma_node_conf_struct.NodeType = DMA_GPDMA_LINEAR_NODE;                                                  /* �ڵ����� */
    dma_node_conf_struct.Init.Request = GPDMA1_REQUEST_USART1_TX;                                           /* ͨ������ */
    dma_node_conf_struct.Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;                                         /* ��Ӳ������ģʽ */
    dma_node_conf_struct.Init.Direction = DMA_MEMORY_TO_PERIPH;                                             /* ���䷽�� */
    dma_node_conf_struct.Init.SrcInc = DMA_SINC_INCREMENTED;                                                /* ����Դ��ַ����ģʽ */
    dma_node_conf_struct.Init.DestInc = DMA_DINC_FIXED;                                                     /* ����Ŀ���ַ����ģʽ */
    dma_node_conf_struct.Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;                                        /* ����Դ���ݿ��� */
    dma_node_conf_struct.Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;                                      /* ����Ŀ�����ݿ��� */
    dma_node_conf_struct.Init.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;                                       /* ���ȼ� */
    dma_node_conf_struct.Init.SrcBurstLength = 1;                                                           /* ����Դͻ������ */ 
    dma_node_conf_struct.Init.DestBurstLength = 1;                                                          /* ����Ŀ��ͻ������ */
    dma_node_conf_struct.Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;   /* ����˿ڷ��� */
    dma_node_conf_struct.Init.TransferEventMode = DMA_TCEM_LAST_LL_ITEM_TRANSFER;                           /* �����¼�ģʽ */
    dma_node_conf_struct.Init.Mode = DMA_NORMAL;                                                            /* ����ģʽ */
    dma_node_conf_struct.DataHandlingConfig.DataExchange = DMA_EXCHANGE_NONE;                               /* ���ݽ���ģʽ */
    dma_node_conf_struct.DataHandlingConfig.DataAlignment = DMA_DATA_RIGHTALIGN_ZEROPADDED;                 /* �������Ͷ���ģʽ */
    dma_node_conf_struct.TriggerConfig.TriggerPolarity = DMA_TRIG_POLARITY_MASKED;                          /* �����¼����ȼ� */
    dma_node_conf_struct.DstAddress = (uint32_t)&g_uart1_handle.Instance->TDR;                              /* Ŀ�ĵ�ַ */
    for (node_index = 0; node_index < node_used; node_index++)
    {
        dma_node_conf_struct.SrcAddress = bufaddr[node_index];                                              /* Դ��ַ */
        dma_node_conf_struct.DataSize = bufsize[node_index];                                                /* ���ݴ�С */
        
        /* ����DMA�����ڵ� */
        HAL_DMAEx_List_BuildNode(&dma_node_conf_struct, &g_dma_node_struct[node_index]);
        
        /* DMA�����ڵ�������� */
        HAL_DMAEx_List_InsertNode_Tail(&g_dma_qlist_struct, &g_dma_node_struct[node_index]);
    }
    
    /* ��ʼ������ģʽDMA */
    g_dma_handle.Instance = GPDMA1_Channel0;
    g_dma_handle.InitLinkedList.Priority = DMA_LOW_PRIORITY_LOW_WEIGHT;                                     /* ���ȼ� */
    g_dma_handle.InitLinkedList.LinkStepMode = DMA_LSM_FULL_EXECUTION;                                      /* ����ģʽ */
    g_dma_handle.InitLinkedList.LinkAllocatedPort = DMA_LINK_ALLOCATED_PORT0;                               /* �˿ڷ��� */
    g_dma_handle.InitLinkedList.TransferEventMode = DMA_TCEM_LAST_LL_ITEM_TRANSFER;                         /* �����¼�ģʽ */
    g_dma_handle.InitLinkedList.LinkedListMode = DMA_LINKEDLIST_NORMAL;                                     /* ��������ģʽ */
    HAL_DMAEx_List_Init(&g_dma_handle);
    
    /* ����DMA��DMA���� */
    HAL_DMAEx_List_LinkQ(&g_dma_handle, &g_dma_qlist_struct);
    
    /* ����������DMA */
    __HAL_LINKDMA(&g_uart1_handle, hdmatx, g_dma_handle);
    
    /* ����ͨ������ */
    HAL_DMA_ConfigChannelAttributes(&g_dma_handle, DMA_CHANNEL_NPRIV);
    
    /* ע��DMA������ɻص����� */
    HAL_DMA_RegisterCallback(&g_dma_handle, HAL_DMA_XFER_CPLT_CB_ID, dma_transfer_complete_cb);
    
    /* �����ж����ȼ���ʹ���ж� */
    HAL_NVIC_SetPriority(GPDMA1_Channel0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(GPDMA1_Channel0_IRQn);
}

/**
 * @brief   DMA������ɻص�����
 * @param   ��
 * @retval  ��
 */
void dma_start_transfer(void)
{
    if (dma_ready == 1)
    {
        dma_ready = 0;
        
        /* �����ж�ģʽ��DMA�������� */
        HAL_DMAEx_List_Start_IT(&g_dma_handle);
        
        /* ʹ��UART��DMA���� */
        ATOMIC_SET_BIT(g_uart1_handle.Instance->CR3, USART_CR3_DMAT);
    }
}

/**
 * @brief   DMA������ɻص�����
 * @param   hdma: DMA���ָ��
 * @retval  ��
 */
static void dma_transfer_complete_cb(DMA_HandleTypeDef *const hdma)
{
    if (hdma->Instance == GPDMA1_Channel0)
    {
        dma_ready = 1;
    }
}

/**
 * @brief   GPDMA1 Channel0中断处理函数 (UART DMA)
 * @param   无
 * @retval  无
 */
void GPDMA1_Channel0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_dma_handle);
}
