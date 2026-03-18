#include "./BSP/MMC/mmc_sdcard.h"
#include "./SYSTEM/delay/delay.h"
#include "string.h"
#include "stdio.h"
#include "./LIBS/lib_dwt/lib_dwt_timestamp.h"
#include "usbd_cdc_if.h"

MMC_HandleTypeDef        g_sd_handle;
HAL_MMC_CardInfoTypeDef  g_sd_card_info_struct;

static volatile uint8_t  g_mmc_rx_done = 0;
static volatile uint8_t  g_mmc_tx_done = 0;
static volatile uint8_t  g_mmc_error   = 0;

#define MMC_LOG(fmt, ...) usb_printf("[eMMC] "      fmt "\r\n", ##__VA_ARGS__)
#define MMC_ERR(fmt, ...) usb_printf("[eMMC][ERR] " fmt "\r\n", ##__VA_ARGS__)

/* ================================================================== */
/*  异步状态机                                                          */
/* ================================================================== */

static volatile MmcAsyncState s_mmc_state = MMC_IDLE;

/**
 * @brief  轮询 eMMC 异步操作状态，不阻塞，在主循环中反复调用
 * @return 当前状态
 */
MmcAsyncState sd_disk_poll(void)
{
    /* 终态直接返回，无需重复检查 */
    if (s_mmc_state == MMC_IDLE  ||
        s_mmc_state == MMC_DONE  ||
        s_mmc_state == MMC_ERROR)
        return s_mmc_state;

    /* ★ 错误优先：ErrorCallback 与 TxCpltCallback 可能同时置位
     *   若先检查 done_flag 会误判为成功，因此错误必须最先处理 */
    if (g_mmc_error)
    {
        g_mmc_error   = 0;
        g_mmc_rx_done = 0;
        g_mmc_tx_done = 0;
        s_mmc_state   = MMC_ERROR;
        return MMC_ERROR;
    }

    /* DMA 中断尚未触发，本次直接返回，不阻塞 */
    if (s_mmc_state == MMC_RX_BUSY && !g_mmc_rx_done) return MMC_RX_BUSY;
    if (s_mmc_state == MMC_TX_BUSY && !g_mmc_tx_done) return MMC_TX_BUSY;

    /* DMA 完成，等待卡回到 TRANSFER 状态
     * 未就绪则本次返回，下次再查，不死等 */
    if (HAL_MMC_GetCardState(&g_sd_handle) != HAL_MMC_CARD_TRANSFER)
        return s_mmc_state;

    /* 全部完成 */
    g_mmc_rx_done = 0;
    g_mmc_tx_done = 0;
    s_mmc_state   = MMC_DONE;
    return MMC_DONE;
}

/**
 * @brief  将状态机复位为 IDLE，在 DONE 或 ERROR 之后、下次操作之前调用
 */
void sd_disk_reset(void)
{
    s_mmc_state = MMC_IDLE;
}

/* ================================================================== */
/*  初始化                                                              */
/* ================================================================== */

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
    g_sd_handle.Instance                 = SD_SDMMCX;
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
    usb_printf("MMC init OK, size: %llu MB\n", card_size / (1024ULL * 1024ULL));

    return 0;
}

/* ================================================================== */
/*  中断 & 回调                                                         */
/* ================================================================== */

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

/**
 * @brief  HAL_MMC_Abort 完成回调
 *         超时强制中止 DMA 后，清零所有标志，避免残留状态污染下次操作
 */
void HAL_MMC_AbortCallback(MMC_HandleTypeDef *hmmc)
{
    g_mmc_rx_done = 0;
    g_mmc_tx_done = 0;
    g_mmc_error   = 0;
    /* s_mmc_state 由调用方在 Abort 后通过 sd_disk_reset() 置回 IDLE */
}

/* ================================================================== */
/*  异步发起接口（立即返回，不等待）                                      */
/* ================================================================== */

/**
 * @brief  异步发起 DMA 读操作
 * @note   buf 必须 32 字节对齐（Cache Line 对齐），否则 Invalidate 会污染相邻内存
 * @return 0=成功发起  1=忙或参数错误
 */
uint8_t sd_read_disk_async(uint8_t *buf, uint32_t address, uint32_t count)
{
    if (s_mmc_state != MMC_IDLE)
        return 1;

    /* buf 必须 32 字节对齐 */
		// assert(((uint32_t)buf % 32U) == 0U);

    /* ★ Invalidate 放在 DMA 发起之前：
     *   让 CPU 后续访问 buf 时强制从内存读，不会读到旧 Cache 数据
     *   DMA 完成后无需再次 Invalidate */
    SCB_InvalidateDCache_by_Addr((uint32_t *)buf, count * SECTOR_SIZE);

    g_mmc_rx_done = 0;
    g_mmc_error   = 0;

    if (HAL_MMC_ReadBlocks_DMA(&g_sd_handle, buf, address, count) != HAL_OK)
        return 1;

    s_mmc_state = MMC_RX_BUSY;
    return 0;
}

/**
 * @brief  异步发起 DMA 写操作
 * @note   buf 必须 32 字节对齐
 * @return 0=成功发起  1=忙或参数错误
 */
uint8_t sd_write_disk_async(uint8_t *buf, uint32_t address, uint32_t count)
{
    if (s_mmc_state != MMC_IDLE)
        return 1;

    /* buf 必须 32 字节对齐 */
    // assert(((uint32_t)buf % 32U) == 0U);

    /* ★ Clean（不是 Invalidate）放在 DMA 发起之前：
     *   把 Cache 中的脏数据刷回内存，DMA 才能读到最新数据
     *   写操作不需要 Invalidate，Invalidate 会丢弃 CPU 还没写完的数据 */
    SCB_CleanDCache_by_Addr((uint32_t *)buf, count * SECTOR_SIZE);

    g_mmc_tx_done = 0;
    g_mmc_error   = 0;

    if (HAL_MMC_WriteBlocks_DMA(&g_sd_handle, buf, address, count) != HAL_OK)
        return 1;

    s_mmc_state = MMC_TX_BUSY;
    return 0;
}

/* ================================================================== */
/*  同步接口（FatFS diskio 调用，内部轮询不死等）                         */
/* ================================================================== */

/**
 * @brief  同步读（阻塞式，内部使用非阻塞轮询替代 HAL_Delay 死等）
 *         超时后调用 HAL_MMC_Abort 中止 DMA，防止下次操作时总线冲突
 */
uint8_t sd_read_disk(uint8_t *buf, uint32_t address, uint32_t count)
{
    if (sd_read_disk_async(buf, address, count) != 0)
        return 1;

    uint32_t t = HAL_GetTick();
    while (1)
    {
        MmcAsyncState st = sd_disk_poll();
        if (st == MMC_DONE)
        {
            sd_disk_reset();
            return 0;
        }
        if (st == MMC_ERROR)
        {
            sd_disk_reset();
            MMC_ERR("read error");
            return 1;
        }
        if (HAL_GetTick() - t > SD_TIMEOUT)
        {
            /* ★ 超时必须先 Abort，再 reset
             *   否则 DMA 可能仍在运行，下次发起操作会与之冲突 */
            HAL_MMC_Abort(&g_sd_handle);
            sd_disk_reset();
            MMC_ERR("read timeout");
            return 1;
        }
        /* 不加 HAL_Delay：去掉 1ms 强制延迟，让采集中断、其他中断照常响应 */
    }
}

/**
 * @brief  同步写（阻塞式，内部使用非阻塞轮询替代 HAL_Delay 死等）
 */
uint8_t sd_write_disk(uint8_t *buf, uint32_t address, uint32_t count)
{
    if (sd_write_disk_async(buf, address, count) != 0)
        return 1;

    uint32_t t = HAL_GetTick();
    while (1)
    {
        MmcAsyncState st = sd_disk_poll();
        if (st == MMC_DONE)
        {
            sd_disk_reset();
            return 0;
        }
        if (st == MMC_ERROR)
        {
            sd_disk_reset();
            MMC_ERR("write error");
            return 1;
        }
        if (HAL_GetTick() - t > SD_TIMEOUT)
        {
            HAL_MMC_Abort(&g_sd_handle);
            sd_disk_reset();
            MMC_ERR("write timeout");
            return 1;
        }
    }
}

/* ================================================================== */
/*  状态查询                                                            */
/* ================================================================== */

void sd_get_status(void)
{
    HAL_MMC_GetCardInfo(&g_sd_handle, &g_sd_card_info_struct);
}

/* ================================================================== */
/*  MSP：时钟 / GPIO / 中断配置                                         */
/* ================================================================== */

void HAL_MMC_MspInit(MMC_HandleTypeDef *hsd)
{
    RCC_PeriphCLKInitTypeDef rcc_periph_clk_init_struct = {0};
    GPIO_InitTypeDef         gpio_init_struct            = {0};

    if (hsd->Instance == SDMMC1)
    {
        rcc_periph_clk_init_struct.PeriphClockSelection  = RCC_PERIPHCLK_SDMMC12;
        rcc_periph_clk_init_struct.Sdmmc12ClockSelection = RCC_SDMMC12CLKSOURCE_PLL2T;
        HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init_struct);

        SD_SDMMCX_CLK_ENABLE();
        SD_SDMMCX_CK_GPIO_CLK_ENABLE();
        SD_SDMMCX_CMD_GPIO_CLK_ENABLE();
        SD_SDMMCX_D0_GPIO_CLK_ENABLE();
        SD_SDMMCX_D1_GPIO_CLK_ENABLE();
        SD_SDMMCX_D2_GPIO_CLK_ENABLE();
        SD_SDMMCX_D3_GPIO_CLK_ENABLE();

        gpio_init_struct.Mode      = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull      = GPIO_PULLUP;
        gpio_init_struct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;

        gpio_init_struct.Pin       = SD_SDMMCX_CK_GPIO_PIN;
        gpio_init_struct.Alternate = SD_SDMMCX_CK_GPIO_AF;
        HAL_GPIO_Init(SD_SDMMCX_CK_GPIO_PORT, &gpio_init_struct);

        gpio_init_struct.Pin       = SD_SDMMCX_CMD_GPIO_PIN;
        gpio_init_struct.Alternate = SD_SDMMCX_CMD_GPIO_AF;
        HAL_GPIO_Init(SD_SDMMCX_CMD_GPIO_PORT, &gpio_init_struct);

        gpio_init_struct.Pin       = SD_SDMMCX_D0_GPIO_PIN;
        gpio_init_struct.Alternate = SD_SDMMCX_D0_GPIO_AF;
        HAL_GPIO_Init(SD_SDMMCX_D0_GPIO_PORT, &gpio_init_struct);

        gpio_init_struct.Pin       = SD_SDMMCX_D1_GPIO_PIN;
        gpio_init_struct.Alternate = SD_SDMMCX_D1_GPIO_AF;
        HAL_GPIO_Init(SD_SDMMCX_D1_GPIO_PORT, &gpio_init_struct);

        gpio_init_struct.Pin       = SD_SDMMCX_D2_GPIO_PIN;
        gpio_init_struct.Alternate = SD_SDMMCX_D2_GPIO_AF;
        HAL_GPIO_Init(SD_SDMMCX_D2_GPIO_PORT, &gpio_init_struct);

        gpio_init_struct.Pin       = SD_SDMMCX_D3_GPIO_PIN;
        gpio_init_struct.Alternate = SD_SDMMCX_D3_GPIO_AF;
        HAL_GPIO_Init(SD_SDMMCX_D3_GPIO_PORT, &gpio_init_struct);

        HAL_NVIC_SetPriority(SDMMC1_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(SDMMC1_IRQn);
    }
}

void HAL_MMC_MspDeInit(MMC_HandleTypeDef *hsd)
{
    if (hsd->Instance == SDMMC1)
    {
        SD_SDMMCX_CLK_DISABLE();

        HAL_GPIO_DeInit(SD_SDMMCX_CK_GPIO_PORT,  SD_SDMMCX_CK_GPIO_PIN);
        HAL_GPIO_DeInit(SD_SDMMCX_CMD_GPIO_PORT, SD_SDMMCX_CMD_GPIO_PIN);
        HAL_GPIO_DeInit(SD_SDMMCX_D0_GPIO_PORT,  SD_SDMMCX_D0_GPIO_PIN);
        HAL_GPIO_DeInit(SD_SDMMCX_D1_GPIO_PORT,  SD_SDMMCX_D1_GPIO_PIN);
        HAL_GPIO_DeInit(SD_SDMMCX_D2_GPIO_PORT,  SD_SDMMCX_D2_GPIO_PIN);
        HAL_GPIO_DeInit(SD_SDMMCX_D3_GPIO_PORT,  SD_SDMMCX_D3_GPIO_PIN);

        HAL_NVIC_DisableIRQ(SDMMC1_IRQn);
    }
}