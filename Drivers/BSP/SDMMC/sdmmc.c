/**
 ****************************************************************************************************
 * @file        sdmmc.c
 * @author      ALIENTEK
 * @version     V1.0
 * @date        2026-02-13
 * @brief       SDMMC初始化和配置
 * @license     Copyright (c) 2020-2032, ALIENTEK
 ****************************************************************************************************
 * @attention
 * 
 * Platform: ALIENTEK H7R3 Development Board
 * Website: www.yuanzige.com
 * Forum: www.openedv.com
 * Company: www.alientek.com
 * Store: openedv.taobao.com
 * 
 ****************************************************************************************************
 */

#include "./BSP/SDMMC/sdmmc.h"

MMC_HandleTypeDef hmmc;
DMA_HandleTypeDef hdma_sdmmc1_rx;
DMA_HandleTypeDef hdma_sdmmc1_tx;

volatile uint8_t mmc_read_complete = 0;
volatile uint8_t mmc_write_complete = 0;
volatile uint8_t mmc_error = 0;

/**
 * @brief 配置SDMMC GPIO引脚
 */
void SDMMC_GPIOConfig(void)
{
    RCC_PeriphCLKInitTypeDef rcc_periph_clk_init_struct = {0};
    GPIO_InitTypeDef gpio_init_struct = {0};
    
    /* 配置时钟 */
    rcc_periph_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_SDMMC12;
    rcc_periph_clk_init_struct.Sdmmc12ClockSelection = RCC_SDMMC12CLKSOURCE_PLL2T;
    HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init_struct);
    
    /* 使能时钟 */
    SD_SDMMCX_CLK_ENABLE();
    SD_SDMMCX_CK_GPIO_CLK_ENABLE();
    SD_SDMMCX_CMD_GPIO_CLK_ENABLE();
    SD_SDMMCX_D0_GPIO_CLK_ENABLE();
    SD_SDMMCX_D1_GPIO_CLK_ENABLE();
    SD_SDMMCX_D2_GPIO_CLK_ENABLE();
    SD_SDMMCX_D3_GPIO_CLK_ENABLE();
    
    /* 初始化CK引脚 */
    gpio_init_struct.Pin = SD_SDMMCX_CK_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_struct.Alternate = SD_SDMMCX_CK_GPIO_AF;
    HAL_GPIO_Init(SD_SDMMCX_CK_GPIO_PORT, &gpio_init_struct);
    
    /* 初始化CMD引脚 */
    gpio_init_struct.Pin = SD_SDMMCX_CMD_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_struct.Alternate = SD_SDMMCX_CMD_GPIO_AF;
    HAL_GPIO_Init(SD_SDMMCX_CMD_GPIO_PORT, &gpio_init_struct);
    
    /* 初始化D0引脚 */
    gpio_init_struct.Pin = SD_SDMMCX_D0_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_struct.Alternate = SD_SDMMCX_D0_GPIO_AF;
    HAL_GPIO_Init(SD_SDMMCX_D0_GPIO_PORT, &gpio_init_struct);
    
    /* 初始化D1引脚 */
    gpio_init_struct.Pin = SD_SDMMCX_D1_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_struct.Alternate = SD_SDMMCX_D1_GPIO_AF;
    HAL_GPIO_Init(SD_SDMMCX_D1_GPIO_PORT, &gpio_init_struct);
    
    /* 初始化D2引脚 */
    gpio_init_struct.Pin = SD_SDMMCX_D2_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_struct.Alternate = SD_SDMMCX_D2_GPIO_AF;
    HAL_GPIO_Init(SD_SDMMCX_D2_GPIO_PORT, &gpio_init_struct);
    
    /* 初始化D3引脚 */
    gpio_init_struct.Pin = SD_SDMMCX_D3_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_struct.Alternate = SD_SDMMCX_D3_GPIO_AF;
    HAL_GPIO_Init(SD_SDMMCX_D3_GPIO_PORT, &gpio_init_struct);
}

/**
 * @brief 初始化SDMMC
 */
int32_t SDMMC_Init(void)
{
    int32_t ret = 0;
    
    // 配置时钟
    SDMMC_ClockConfig();
    
    // 配置GPIO
    SDMMC_GPIOConfig();
    
    // MMC配置
    hmmc.Instance = SDMMC1;
    hmmc.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
    hmmc.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
    hmmc.Init.BusWide = SDMMC_BUS_WIDE_1B;
    hmmc.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
    hmmc.Init.ClockDiv = 255; // 低速初始化
    
    // 初始化MMC
    if (HAL_MMC_Init(&hmmc) != HAL_OK)
    {
        ret = -2;
        goto cleanup;
    }
    
    // 提高时钟频率
    hmmc.Init.ClockDiv = 0; // 高速运行
    if (HAL_MMC_Init(&hmmc) != HAL_OK)
    {
        ret = -3;
        goto cleanup;
    }
    
    // 配置4位总线
//    if (HAL_MMC_ConfigWideBusOperation(&hmmc, SDMMC_BUS_WIDE_4B) != HAL_OK)
//    {
//        ret = -4;
//        goto cleanup;
//    }
    
    // 获取卡信息
    HAL_MMC_CardInfoTypeDef cardInfo;
    if (HAL_MMC_GetCardInfo(&hmmc, &cardInfo) != HAL_OK)
    {
        ret = -5;
        goto cleanup;
    }
    
//    printf("MMC卡初始化成功\n");
//    printf("卡容量: %llu MB\n", cardInfo.LogBlockNbr * cardInfo.LogBlockSize / (1024 * 1024));
//    printf("扇区大小: %d bytes\n", cardInfo.LogBlockSize);
    
cleanup:
    if (ret != 0)
    {
//        printf("MMC卡初始化失败: %d\n", ret);
    }
    
    return ret;
}

/**
 * @brief SDMMC MSP初始化
 */
void HAL_MMC_MspInit(MMC_HandleTypeDef* hmmc)
{
    if (hmmc->Instance == SDMMC1)
    {
        // 使能SDMMC时钟
        __HAL_RCC_SDMMC1_CLK_ENABLE();
        
        // 配置GPIO
        SDMMC_GPIOConfig();
        
        // 配置DMA接收
        hdma_sdmmc1_rx.Instance = DMA2_Stream6;
        hdma_sdmmc1_rx.Init.Request = DMA_REQUEST_SDMMC1;
        hdma_sdmmc1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
        hdma_sdmmc1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_sdmmc1_rx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_sdmmc1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
        hdma_sdmmc1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
        hdma_sdmmc1_rx.Init.Mode = DMA_NORMAL;
        hdma_sdmmc1_rx.Init.Priority = DMA_PRIORITY_HIGH;
        hdma_sdmmc1_rx.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
        hdma_sdmmc1_rx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
        hdma_sdmmc1_rx.Init.MemBurst = DMA_MBURST_INC4;
        hdma_sdmmc1_rx.Init.PeriphBurst = DMA_PBURST_INC4;
        
        if (HAL_DMA_Init(&hdma_sdmmc1_rx) != HAL_OK)
        {
            Error_Handler();
        }
        
        // 关联DMA接收
        __HAL_LINKDMA(hmmc, hdmarx, hdma_sdmmc1_rx);
        
        // 配置DMA发送
        hdma_sdmmc1_tx.Instance = DMA2_Stream7;
        hdma_sdmmc1_tx.Init.Request = DMA_REQUEST_SDMMC1;
        hdma_sdmmc1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_sdmmc1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_sdmmc1_tx.Init.MemInc = DMA_MINC_ENABLE;
        hdma_sdmmc1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
        hdma_sdmmc1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
        hdma_sdmmc1_tx.Init.Mode = DMA_NORMAL;
        hdma_sdmmc1_tx.Init.Priority = DMA_PRIORITY_HIGH;
        hdma_sdmmc1_tx.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
        hdma_sdmmc1_tx.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
        hdma_sdmmc1_tx.Init.MemBurst = DMA_MBURST_INC4;
        hdma_sdmmc1_tx.Init.PeriphBurst = DMA_PBURST_INC4;
        
        if (HAL_DMA_Init(&hdma_sdmmc1_tx) != HAL_OK)
        {
            Error_Handler();
        }
        
        // 关联DMA发送
        __HAL_LINKDMA(hmmc, hdmatx, hdma_sdmmc1_tx);
        
        // 配置中断
        HAL_NVIC_SetPriority(SDMMC1_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(SDMMC1_IRQn);
        
        HAL_NVIC_SetPriority(DMA2_Stream6_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(DMA2_Stream6_IRQn);
        
        HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 6, 0);
        HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
    }
}

/**
 * @brief SDMMC MSP去初始化
 */
void HAL_MMC_MspDeInit(MMC_HandleTypeDef* hmmc)
{
    if (hmmc->Instance == SDMMC1)
    {
        // 禁用中断
        HAL_NVIC_DisableIRQ(SDMMC1_IRQn);
        HAL_NVIC_DisableIRQ(DMA2_Stream6_IRQn);
        HAL_NVIC_DisableIRQ(DMA2_Stream7_IRQn);
        
        // 去初始化DMA
        HAL_DMA_DeInit(hmmc->hdmarx);
        HAL_DMA_DeInit(hmmc->hdmatx);
        
        // 禁用时钟
        __HAL_RCC_SDMMC1_CLK_DISABLE();
        
        // 复位GPIO
        HAL_GPIO_DeInit(GPIOC, GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12);
        HAL_GPIO_DeInit(GPIOD, GPIO_PIN_2);
    }
}

/**
 * @brief 等待MMC操作完成
 */
static bool wait_mmc_operation_complete(uint32_t timeout_ms)
{
    uint32_t start_tick = HAL_GetTick();
    
    while (1)
    {
        if (mmc_read_complete || mmc_write_complete || mmc_error)
        {
            return true;
        }
        
        if ((HAL_GetTick() - start_tick) > timeout_ms)
        {
            return false; // 超时
        }
        
        // 短暂延时
        HAL_Delay(1);
    }
}

/**
 * @brief 读取MMC卡数据
 */
int32_t SDMMC_ReadBlocks(uint32_t sector, uint32_t count, uint8_t *buffer)
{
    mmc_read_complete = 0;
    mmc_error = 0;
    
    if (HAL_MMC_ReadBlocks_DMA(&hmmc, buffer, sector, count) != HAL_OK)
    {
        return -1;
    }
    
    // 等待操作完成
    if (!wait_mmc_operation_complete(5000))
    {
        return -2; // 超时
    }
    
    if (mmc_error)
    {
        return -3; // 错误
    }
    
    return 0;
}

/**
 * @brief 写入MMC卡数据
 */
int32_t SDMMC_WriteBlocks(uint32_t sector, uint32_t count, uint8_t *buffer)
{
    mmc_write_complete = 0;
    mmc_error = 0;
    
    if (HAL_MMC_WriteBlocks_DMA(&hmmc, buffer, sector, count) != HAL_OK)
    {
        return -1;
    }
    
    // 等待操作完成
    if (!wait_mmc_operation_complete(5000))
    {
        return -2; // 超时
    }
    
    if (mmc_error)
    {
        return -3; // 错误
    }
    
    return 0;
}

/**
 * @brief MMC卡读取完成回调
 */
void HAL_MMC_RxCpltCallback(MMC_HandleTypeDef *hmmc)
{
    mmc_read_complete = 1;
}

/**
 * @brief MMC卡写入完成回调
 */
void HAL_MMC_TxCpltCallback(MMC_HandleTypeDef *hmmc)
{
    mmc_write_complete = 1;
}

/**
 * @brief MMC卡错误回调
 */
void HAL_MMC_ErrorCallback(MMC_HandleTypeDef *hmmc)
{
    mmc_error = 1;
}

/**
 * @brief SDMMC1中断处理
 */
void SDMMC1_IRQHandler(void)
{
    HAL_MMC_IRQHandler(&hmmc);
}

/**
 * @brief DMA2 Stream6中断处理
 */
void DMA2_Stream6_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_sdmmc1_rx);
}

/**
 * @brief DMA2 Stream7中断处理
 */
void DMA2_Stream7_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_sdmmc1_tx);
}
