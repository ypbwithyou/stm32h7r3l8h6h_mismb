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
    g_sd_handle.Init.ClockEdge = SDMMC_CLOCK_EDGE_RISING;
    g_sd_handle.Init.ClockPowerSave = SDMMC_CLOCK_POWER_SAVE_DISABLE;
    g_sd_handle.Init.BusWide = SDMMC_BUS_WIDE_1B;
    g_sd_handle.Init.HardwareFlowControl = SDMMC_HARDWARE_FLOW_CONTROL_ENABLE;
    g_sd_handle.Init.ClockDiv = 2;

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

    uint8_t ret = mmc_wait_dma_done(&g_mmc_rx_done);

    // ← 加这行！
    SCB_InvalidateDCache_by_Addr((uint32_t *)buf, count * SECTOR_SIZE);

    return ret;
}

uint8_t sd_write_disk(uint8_t *buf, uint32_t address, uint32_t count)
{
    g_mmc_tx_done = 0;
    g_mmc_error = 0;

    // ← 加这行！
    SCB_InvalidateDCache_by_Addr((uint32_t *)buf, count * SECTOR_SIZE);

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
