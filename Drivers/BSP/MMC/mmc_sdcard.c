#include "./BSP/MMC/mmc_sdcard.h"
#include "./SYSTEM/delay/delay.h"
#include "string.h"
#include "stdio.h"
#include "./LIBS/lib_dwt/lib_dwt_timestamp.h"
#include "usbd_cdc_if.h"

MMC_HandleTypeDef g_sd_handle;
HAL_MMC_CardInfoTypeDef g_sd_card_info_struct;

static volatile uint8_t g_mmc_rx_done = 0;
static volatile uint8_t g_mmc_tx_done = 0;
static volatile uint8_t g_mmc_error = 0;

#define MMC_LOG(fmt, ...) usb_printf("[eMMC] " fmt "\r\n", ##__VA_ARGS__)
#define MMC_ERR(fmt, ...) usb_printf("[eMMC][ERR] " fmt "\r\n", ##__VA_ARGS__)

/* ------------------------------------------------------------------ */
/*  初始化                                                              */
/* ------------------------------------------------------------------ */
uint8_t mmc_init(void)
{
     static uint8_t init_done = 0;

    if (init_done)
    {
        if (HAL_MMC_GetCardState(&g_sd_handle) == HAL_MMC_CARD_TRANSFER)
            return 0;

        HAL_MMC_DeInit(&g_sd_handle);
        init_done = 0;
    }

    usb_printf("MMC init start\n");

    __HAL_RCC_SDMMC1_FORCE_RESET();
    HAL_Delay(20);
    __HAL_RCC_SDMMC1_RELEASE_RESET();
    HAL_Delay(100);

    /* 第一步：先以 1-bit 模式初始化 */
    g_sd_handle.Instance = SD_SDMMCX;
    g_sd_handle.Init.ClockEdge           = SDMMC_CLOCK_EDGE_RISING;
    g_sd_handle.Init.ClockPowerSave      = SDMMC_CLOCK_POWER_SAVE_DISABLE;
    g_sd_handle.Init.BusWide             = SDMMC_BUS_WIDE_1B;
    g_sd_handle.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
    g_sd_handle.Init.ClockDiv            = 2;

    if (HAL_MMC_Init(&g_sd_handle) != HAL_OK)
    {
        MMC_ERR("HAL_MMC_Init (1-bit) failed");
        return 1;
    }

    /* 等待卡进入 TRANSFER 状态 */
    uint32_t timeout = SD_TIMEOUT;
    while (HAL_MMC_GetCardState(&g_sd_handle) != HAL_MMC_CARD_TRANSFER)
    {
        HAL_Delay(1);
        if (--timeout == 0)
        {
            MMC_ERR("Card not ready after 1-bit init, timeout");
            return 1;
        }
    }

    /* 第二步：切换到 4-bit 模式 */
    if (HAL_MMC_ConfigWideBusOperation(&g_sd_handle, SDMMC_BUS_WIDE_4B) != HAL_OK)
    {
        MMC_ERR("ConfigWideBusOperation (4-bit) failed");
        return 1;
    }

    /* 等待切换完成 */
    timeout = SD_TIMEOUT;
    while (HAL_MMC_GetCardState(&g_sd_handle) != HAL_MMC_CARD_TRANSFER)
    {
        HAL_Delay(1);
        if (--timeout == 0)
        {
            MMC_ERR("Card not ready after 4-bit switch, timeout");
            return 1;
        }
    }

    HAL_MMC_GetCardInfo(&g_sd_handle, &g_sd_card_info_struct);

    init_done = 1;

    uint64_t card_size = (uint64_t)g_sd_card_info_struct.LogBlockNbr *
                         (uint64_t)g_sd_card_info_struct.LogBlockSize;
    usb_printf("MMC init OK, size: %llu MB\n", card_size / (1024 * 1024));

    return 0;
}

// /* ------------------------------------------------------------------ */
// /*  MSP                                                                 */
// /* ------------------------------------------------------------------ */
// void HAL_MMC_MspInit(MMC_HandleTypeDef *hmmc)
// {
//     GPIO_InitTypeDef gpio_init_struct = {0};

//     SD_SDMMCX_CLK_ENABLE();
//     SD_SDMMCX_CK_GPIO_CLK_ENABLE();
//     SD_SDMMCX_CMD_GPIO_CLK_ENABLE();
//     SD_SDMMCX_D0_GPIO_CLK_ENABLE();
//     SD_SDMMCX_D1_GPIO_CLK_ENABLE();
//     SD_SDMMCX_D2_GPIO_CLK_ENABLE();
//     SD_SDMMCX_D3_GPIO_CLK_ENABLE();

//     gpio_init_struct.Mode = GPIO_MODE_AF_PP;
//     gpio_init_struct.Pull = GPIO_PULLUP;
//     gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

//     gpio_init_struct.Pin = SD_SDMMCX_CK_GPIO_PIN;
//     gpio_init_struct.Alternate = SD_SDMMCX_CK_GPIO_AF;
//     HAL_GPIO_Init(SD_SDMMCX_CK_GPIO_PORT, &gpio_init_struct);

//     gpio_init_struct.Pin = SD_SDMMCX_CMD_GPIO_PIN;
//     gpio_init_struct.Alternate = SD_SDMMCX_CMD_GPIO_AF;
//     HAL_GPIO_Init(SD_SDMMCX_CMD_GPIO_PORT, &gpio_init_struct);

//     gpio_init_struct.Pin = SD_SDMMCX_D0_GPIO_PIN;
//     gpio_init_struct.Alternate = SD_SDMMCX_D0_GPIO_AF;
//     HAL_GPIO_Init(SD_SDMMCX_D0_GPIO_PORT, &gpio_init_struct);

//     gpio_init_struct.Pin = SD_SDMMCX_D1_GPIO_PIN;
//     gpio_init_struct.Alternate = SD_SDMMCX_D1_GPIO_AF;
//     HAL_GPIO_Init(SD_SDMMCX_D1_GPIO_PORT, &gpio_init_struct);

//     gpio_init_struct.Pin = SD_SDMMCX_D2_GPIO_PIN;
//     gpio_init_struct.Alternate = SD_SDMMCX_D2_GPIO_AF;
//     HAL_GPIO_Init(SD_SDMMCX_D2_GPIO_PORT, &gpio_init_struct);

//     gpio_init_struct.Pin = SD_SDMMCX_D3_GPIO_PIN;
//     gpio_init_struct.Alternate = SD_SDMMCX_D3_GPIO_AF;
//     HAL_GPIO_Init(SD_SDMMCX_D3_GPIO_PORT, &gpio_init_struct);

//     HAL_NVIC_SetPriority(SDMMC1_IRQn, 2, 0);
//     HAL_NVIC_EnableIRQ(SDMMC1_IRQn);
// }

// void HAL_MMC_MspDeInit(MMC_HandleTypeDef *hmmc)
// {
//     HAL_NVIC_DisableIRQ(SDMMC1_IRQn);
//     SD_SDMMCX_CLK_DISABLE();
//     HAL_GPIO_DeInit(SD_SDMMCX_CK_GPIO_PORT, SD_SDMMCX_CK_GPIO_PIN);
//     HAL_GPIO_DeInit(SD_SDMMCX_CMD_GPIO_PORT, SD_SDMMCX_CMD_GPIO_PIN);
//     HAL_GPIO_DeInit(SD_SDMMCX_D0_GPIO_PORT, SD_SDMMCX_D0_GPIO_PIN);
//     HAL_GPIO_DeInit(SD_SDMMCX_D1_GPIO_PORT, SD_SDMMCX_D1_GPIO_PIN);
//     HAL_GPIO_DeInit(SD_SDMMCX_D2_GPIO_PORT, SD_SDMMCX_D2_GPIO_PIN);
//     HAL_GPIO_DeInit(SD_SDMMCX_D3_GPIO_PORT, SD_SDMMCX_D3_GPIO_PIN);
// }

/* ------------------------------------------------------------------ */
/*  中断 & 回调（无日志）                                               */
/* ------------------------------------------------------------------ */
void SDMMC1_IRQHandler(void)
{
    HAL_MMC_IRQHandler(&g_sd_handle);
}

void HAL_MMC_RxCpltCallback(MMC_HandleTypeDef *hmmc)
{
    g_mmc_rx_done = 1;
}

void HAL_MMC_TxCpltCallback(MMC_HandleTypeDef *hmmc)
{
    g_mmc_tx_done = 1;
}

void HAL_MMC_ErrorCallback(MMC_HandleTypeDef *hmmc)
{
    g_mmc_error = 1;
}

/* ------------------------------------------------------------------ */
/*  内部等待函数                                                        */
/* ------------------------------------------------------------------ */
static uint8_t mmc_wait_dma_done(volatile uint8_t *done_flag)
{
    uint32_t timeout = SD_TIMEOUT;

    while (*done_flag == 0)
    {
        HAL_Delay(1);
        if (g_mmc_error)
        {
            g_mmc_error = 0;
            return 1;
        }
        if (--timeout == 0)
            return 1;
    }
    *done_flag = 0;

    timeout = SD_TIMEOUT;
    while (HAL_MMC_GetCardState(&g_sd_handle) != HAL_MMC_CARD_TRANSFER)
    {
        HAL_Delay(1);
        if (--timeout == 0)
            return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  读写接口（无日志）                                                  */
/* ------------------------------------------------------------------ */
uint8_t sd_read_disk(uint8_t *buf, uint32_t address, uint32_t count)
{
    g_mmc_rx_done = 0;
    g_mmc_error = 0;

    if (HAL_MMC_ReadBlocks_DMA(&g_sd_handle, buf, address, count) != HAL_OK)
        return 1;

    return mmc_wait_dma_done(&g_mmc_rx_done);
}

uint8_t sd_write_disk(uint8_t *buf, uint32_t address, uint32_t count)
{
    g_mmc_tx_done = 0;
    g_mmc_error = 0;

    if (HAL_MMC_WriteBlocks_DMA(&g_sd_handle, buf, address, count) != HAL_OK)
        return 1;

    return mmc_wait_dma_done(&g_mmc_tx_done);
}

/* ------------------------------------------------------------------ */
/*  状态查询（无日志）                                                  */
/* ------------------------------------------------------------------ */
void sd_get_status(void)
{
    HAL_MMC_GetCardInfo(&g_sd_handle, &g_sd_card_info_struct);
}

// --------------------------------------------------------------------
// /* MMC句柄 */
// MMC_HandleTypeDef g_sd_handle = {0};
// HAL_MMC_CardInfoTypeDef g_sd_card_info_struct = {0};

// /* IDMA相关标志 */
// volatile uint8_t g_mmc_idma_rx_complete = 0;
// volatile uint8_t g_mmc_idma_tx_complete = 0;
// volatile uint8_t g_mmc_idma_error = 0;

// /**
//  * @brief   初始化SD卡
//  * @param   无
//  * @retval  无
//  */
// // uint8_t mmc_init(void)
// // {
// //     // 确保SystemCoreClock已正确更新
// //     SystemCoreClockUpdate();

// //     g_sd_handle.Instance = SDMMC1;
// //     g_sd_handle.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;                       /* 时钟相位 */
// //     g_sd_handle.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;           /* 节能模式 */
// //     g_sd_handle.Init.BusWide = SDMMC_BUS_WIDE_1B;                               /* 宽总线模式 */
// //     g_sd_handle.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE; /* 硬件流控 */
// //     g_sd_handle.Init.ClockDiv = SDMMC_INIT_CLK_DIV;                             /* 时钟分频系数 */

// //     // 1.初始化
// //     if (HAL_MMC_Init(&g_sd_handle) != HAL_OK) return 1;

// //     // 2. 获取卡信息
// //     if (HAL_MMC_GetCardInfo(&g_sd_handle, &g_sd_card_info_struct) != HAL_OK) return 1;

// //     // 3. 配置 4 位总线宽度
// //     if (HAL_MMC_ConfigWideBusOperation(&g_sd_handle, SDMMC_BUS_WIDE_4B) != HAL_OK) return 1;

// //     // 4. 配置高速模式（内部会重新设置时钟分频为高速）
// //     if (HAL_MMC_ConfigSpeedBusOperation(&g_sd_handle, SDMMC_SPEED_MODE_HIGH) != HAL_OK) return 1;

// //     return 0;
// // }

// /**
//  * @brief 初始化 SD 卡并尝试启用 4-bit + 高速模式
//  * @return 0 表示成功，非 0 表示失败
//  */
// uint8_t mmc_init(void)
// {
//     // static uint8_t init_done = 0;

//     // if (init_done)
//     // {
//     //     if (HAL_MMC_GetCardState(&g_sd_handle) == HAL_MMC_CARD_TRANSFER)
//     //         return 0;
//     // }

//     // init_done = 1;

//     // usb_printf("MMC init start\n");

//     __HAL_RCC_SDMMC1_FORCE_RESET();
//     HAL_Delay(20);
//     __HAL_RCC_SDMMC1_RELEASE_RESET();
//     HAL_Delay(100);

//     HAL_StatusTypeDef status;
//     uint32_t freq_khz;

//     g_sd_handle.Instance = SDMMC1;
//     g_sd_handle.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;                       /* 时钟相位 */
//     g_sd_handle.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;           /* 节能模式 */
//     g_sd_handle.Init.BusWide = SDMMC_BUS_WIDE_1B;                               /* 宽总线模式 */
//     g_sd_handle.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_DISABLE; /* 硬件流控 */
//     g_sd_handle.Init.ClockDiv = SDMMC_INIT_CLK_DIV;                             /* 时钟分频系数 */

//     // 1. 基本初始化（CubeMX 生成的部分）
//     status = HAL_MMC_Init(&g_sd_handle);
//     if (status != HAL_OK)
//     {
//         usb_printf("HAL_MMC_Init failed: %d\n", status);
//         return 1;
//     }

//     // 2. 卡上电与识别（必须先完成）
//     status = HAL_MMC_ConfigWideBusOperation(&g_sd_handle, SDMMC_BUS_WIDE_4B); // 先用 1-bit 识别
//     if (status != HAL_OK)
//     {
//         usb_printf("Initial 1-bit mode failed: %d\n", status);
//         return 1;
//     }

//     status = HAL_MMC_InitCard(&g_sd_handle);
//     if (status != HAL_OK)
//     {
//         usb_printf("HAL_MMC_InitCard failed: %d\n", status);
//         return 1;
//     }

//     // 4. 尝试启用高速模式
//     status = HAL_MMC_ConfigSpeedBusOperation(&g_sd_handle, SDMMC_SPEED_MODE_AUTO);
//     if (status == HAL_OK)
//     {
//         usb_printf("High speed mode enabled\n");
//     }
//     else
//     {
//         usb_printf("High speed mode failed: %d (using default speed)\n", status);
//     }

//     //    // 5. 获取并打印实际工作频率（非常关键的诊断信息）
//     //    freq_khz = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SDMMC);
//     //    usb_printf("SDMMC peripheral clock: %lu kHz\n", freq_khz / 1000UL);

//     // 如果使用的是 SDMMC 时钟分频，可进一步读取实际分频值
//     // 但 HAL 库未直接暴露，可通过调试器查看 SDMMC->CLKCR 寄存器

//     return 0;
// }

/**
 * @brief   HAL库SD初始化MSP函数
 * @param   hsd: MMC句柄
 * @retval  无
 */
void HAL_MMC_MspInit(MMC_HandleTypeDef *hsd)
{
    RCC_PeriphCLKInitTypeDef rcc_periph_clk_init_struct = {0};
    GPIO_InitTypeDef gpio_init_struct = {0};

    if (hsd->Instance == SDMMC1)
    {
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

        /* 配置SDMMC中断 */
        HAL_NVIC_SetPriority(SDMMC1_IRQn, 5, 0);
        HAL_NVIC_EnableIRQ(SDMMC1_IRQn);
    }
}

/**
 * @brief   HAL库SD反初始化MSP函数
 * @param   无
 * @retval  无
 */
void HAL_MMC_MspDeInit(MMC_HandleTypeDef *hsd)
{
    if (hsd->Instance == SDMMC1)
    {
        /* 失能时钟 */
        SD_SDMMCX_CLK_DISABLE();

        /* 反初始化引脚 */
        HAL_GPIO_DeInit(SD_SDMMCX_CK_GPIO_PORT, SD_SDMMCX_CK_GPIO_PIN);
        HAL_GPIO_DeInit(SD_SDMMCX_CMD_GPIO_PORT, SD_SDMMCX_CMD_GPIO_PIN);
        HAL_GPIO_DeInit(SD_SDMMCX_D0_GPIO_PORT, SD_SDMMCX_D0_GPIO_PIN);
        HAL_GPIO_DeInit(SD_SDMMCX_D1_GPIO_PORT, SD_SDMMCX_D1_GPIO_PIN);
        HAL_GPIO_DeInit(SD_SDMMCX_D2_GPIO_PORT, SD_SDMMCX_D2_GPIO_PIN);
        HAL_GPIO_DeInit(SD_SDMMCX_D3_GPIO_PORT, SD_SDMMCX_D3_GPIO_PIN);

        /* 失能中断 */
        HAL_NVIC_DisableIRQ(SDMMC1_IRQn);
    }
}

// /**
//  * @brief   等待IDMA操作完成
//  * @param   timeout_ms: 超时时间(毫秒)
//  * @retval  0: 成功, 1: 超时
//  */
// static uint8_t wait_idma_complete(uint32_t timeout_ms)
// {
//     uint32_t start_time = HAL_GetTick();
//     uint32_t counter = 0;

//     while (1)
//     {
//         if (g_mmc_idma_rx_complete || g_mmc_idma_tx_complete || g_mmc_idma_error)
//         {
//             printf("IDMA操作完成，耗时: %d ms\n", HAL_GetTick() - start_time);
//             return 0;
//         }

//         if ((HAL_GetTick() - start_time) > timeout_ms)
//         {
//             printf("IDMA操作超时，超时时间: %d ms\n", timeout_ms);
//             return 1;
//         }

//         // 使用忙等待代替delay_ms，避免中断嵌套问题
//         for (counter = 0; counter < 1000; counter++)
//         {
//             __NOP();
//         }
//     }
// }

// /**
//  * @brief   读SD卡指定数量块的数据
//  * @param   buf: 数据保存的起始地址
//  * @param   address: 块地址
//  * @param   count: 块数量
//  * @retval  读取结果
//  * @arg     0: 读成功
//  * @arg     1: 读失败
//  */
// uint8_t sd_read_disk(uint8_t *buf, uint32_t address, uint32_t count)
// {
//     // 重置标志
//     g_mmc_idma_rx_complete = 0;
//     g_mmc_idma_error = 0;

//     if (HAL_MMC_ReadBlocks_DMA(&g_sd_handle, buf, address, count) != HAL_OK)
//     {
//         return 1;
//     }

//     // 等待IDMA操作完成
//     if (wait_idma_complete(SD_TIMEOUT) != 0)
//     {
//         return 1;
//     }

//     if (g_mmc_idma_error)
//     {
//         return 1;
//     }

//     return 0;
// }

// /**
//  * @brief   写SD卡指定数量块的数据
//  * @param   buf: 数据保存的起始地址
//  * @param   address: 块地址
//  * @param   count: 块数量
//  * @retval  写入结果
//  * @arg     0: 成功
//  * @arg     1: 失败
//  */
// uint8_t sd_write_disk(uint8_t *buf, uint32_t address, uint32_t count)
// {
//     // 重置标志
//     g_mmc_idma_tx_complete = 0;
//     g_mmc_idma_error = 0;

//     if (HAL_MMC_WriteBlocks_DMA(&g_sd_handle, buf, address, count) != HAL_OK)
//     {
//         return 1;
//     }

//     // 等待IDMA操作完成
//     if (wait_idma_complete(SD_TIMEOUT) != 0)
//     {
//         return 1;
//     }

//     if (g_mmc_idma_error)
//     {
//         return 1;
//     }

//     return 0;
// }

// /**
//  * @brief MMC IDMA接收完成回调
//  */
// void HAL_MMC_RxCpltCallback(MMC_HandleTypeDef *hmmc)
// {
//     g_mmc_idma_rx_complete = 1;
// }

// /**
//  * @brief MMC IDMA发送完成回调
//  */
// void HAL_MMC_TxCpltCallback(MMC_HandleTypeDef *hmmc)
// {
//     g_mmc_idma_tx_complete = 1;
// }

// /**
//  * @brief MMC IDMA错误回调
//  */
// void HAL_MMC_ErrorCallback(MMC_HandleTypeDef *hmmc)
// {
//     g_mmc_idma_error = 1;
// }

// /**
//  * @brief SDMMC1中断处理函数
//  */
// void SDMMC1_IRQHandler(void)
// {
//     HAL_MMC_IRQHandler(&g_sd_handle);
// }

// /**
//  * @brief 测试SD卡读写功能
//  */
// int8_t sd_card_test_rw(void)
// {
//     HAL_StatusTypeDef hal_status;
//     uint8_t write_buffer[512] = {0};
//     uint8_t read_buffer[512] = {0};
//     uint32_t test_sector = 0x1000; /* 测试扇区，避开重要区域 */

//     /* 准备测试数据 */
//     for (int i = 0; i < 512; i++)
//     {
//         write_buffer[i] = i % 512;
//     }
//     /* 清除读取缓冲区 */
//     memset(read_buffer, 0, sizeof(read_buffer));

//     //    uint64_t t_start = dwt_get_us();
//     //    uint32_t t_start = HAL_GetTick();
//     /* 写入测试数据 */
//     hal_status = sd_write_disk(write_buffer, test_sector, 1);
//     if (hal_status != HAL_OK)
//     {
//         //        return -1;
//     }

//     /* 读取测试数据 */
//     hal_status = sd_read_disk(read_buffer, test_sector, 1);
//     //    uint64_t t_stop = dwt_get_us();
//     //    uint32_t t_stop = HAL_GetTick();
//     if (hal_status != HAL_OK)
//     {
//         //        printf("读取测试失败: %d\n", hal_status);
//         //        return -1;
//     }

//     /* 验证数据 */
//     if (memcmp(write_buffer, read_buffer, 512) == 0)
//     {
//         //        printf("SD卡读写测试通过！\n");
//     }
//     else
//     {
//         //        printf("SD卡数据验证失败\n");
//     }

//     /* 检查SD卡状态 */
//     printf("测试完成后SD卡状态: ");
//     sd_get_status();

//     return 0;
// }

// /**
//  * @brief 获取SD卡状态
//  */
// void sd_get_status(void)
// {
//     HAL_MMC_CardStateTypeDef card_state;
//     card_state = HAL_MMC_GetCardState(&g_sd_handle);

//     switch (card_state)
//     {
//     case HAL_MMC_CARD_TRANSFER:
//         //            printf("SD卡状态: 传输模式\n");
//         break;
//     case HAL_MMC_CARD_DISCONNECTED:
//         //            printf("SD卡状态: 断开连接\n");
//         break;
//     case HAL_MMC_CARD_PROGRAMMING:
//         //            printf("SD卡状态: 编程中\n");
//         break;
//     case HAL_MMC_CARD_RECEIVING:
//         //            printf("SD卡状态: 接收数据\n");
//         break;
//     case HAL_MMC_CARD_ERROR:
//         //            printf("SD卡状态: 错误\n");
//         break;
//     default:
//         //            printf("SD卡状态: 未知(%d)\n", card_state);
//         break;
//     }
// }
