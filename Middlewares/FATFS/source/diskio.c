#include "./MALLOC/malloc.h"
#include "./SYSTEM/usart/usart.h"
#include "./FATFS/source/ff.h"
#include "./FATFS/source/diskio.h"
#include "./BSP/NORFLASH/norflash_ex.h"
#include "./BSP/SDNAND/spi_sdnand.h"
#include "./BSP/MMC/mmc_sdcard.h"
#include <string.h>

#define DEV_EMMC       0
#define DISK_RETRY_MAX 3    /* 最大重试次数          */
#define DISK_RETRY_DLY 10   /* 重试间隔 ms           */

/* ★ mmc_sdcard.c 已要求 32 字节 Cache Line 对齐
 *   此处对齐要求从 4 字节升级到 32 字节                */
#define DMA_ALIGN      32U
#define IS_ALIGNED(p)  (((uint32_t)(p) % DMA_ALIGN) == 0U)
/* 未对齐缓冲中转时，按多扇区批量搬运，避免 512B 逐扇区严重降速 */
/* A/B测试建议：
 * - 16U = 8KB（更保守，RAM占用更小）
 * - 32U = 16KB（吞吐可能更高，但会显著增加静态RAM占用） */
#define EMMC_BOUNCE_SECTORS 16U /* 16 * 512 = 8KB */

static DSTATUS g_disk_status = STA_NOINIT;

/* ------------------------------------------------------------------ */
/*  内部：重新初始化 eMMC（重试时用）                                   */
/* ------------------------------------------------------------------ */
static uint8_t emmc_reinit(void)
{
    HAL_MMC_DeInit(&g_sd_handle);
    HAL_Delay(20);
    return mmc_init();
}

/* ------------------------------------------------------------------ */
/*  disk_initialize                                                     */
/* ------------------------------------------------------------------ */
DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != DEV_EMMC)
        return STA_NOINIT;

    for (uint8_t retry = 0; retry < DISK_RETRY_MAX; retry++)
    {
        if (mmc_init() == 0)
        {
            g_disk_status = 0;
            return 0;
        }
        HAL_Delay(DISK_RETRY_DLY);
    }

    g_disk_status = STA_NOINIT;
    return g_disk_status;
}

/* ------------------------------------------------------------------ */
/*  disk_status                                                         */
/* ------------------------------------------------------------------ */
DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != DEV_EMMC)
        return STA_NOINIT;
    return g_disk_status;
}

/* ------------------------------------------------------------------ */
/*  对齐读
 *  DMA + DCache 要求 buf 32 字节对齐。
 *  若上层（FatFS）传入的 buf 不满足对齐，用静态中转缓冲逐扇区拷贝。
 *  中转缓冲声明为静态，避免在栈上分配大数组；
 *  ★ 注意：这里只有一个中转缓冲，不可重入，裸机单任务下安全。
 * ------------------------------------------------------------------ */
static uint8_t emmc_read_aligned(uint8_t *buf, uint32_t sector, uint32_t count)
{
    if (IS_ALIGNED(buf))
        return sd_read_disk(buf, sector, count);

    /* buf 未对齐：按多扇区批量中转，减少 sd_read_disk 调用次数 */
    static uint8_t align_buf[SECTOR_SIZE * EMMC_BOUNCE_SECTORS] __attribute__((aligned(32)));
    uint32_t remain = count;
    uint32_t done = 0U;
    while (remain > 0U)
    {
        uint32_t chunk = (remain > EMMC_BOUNCE_SECTORS) ? EMMC_BOUNCE_SECTORS : remain;
        uint32_t bytes = chunk * SECTOR_SIZE;
        if (sd_read_disk(align_buf, sector + done, chunk) != 0)
            return 1;
        memcpy(buf + done * SECTOR_SIZE, align_buf, bytes);
        done += chunk;
        remain -= chunk;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  对齐写                                                              */
/* ------------------------------------------------------------------ */
static uint8_t emmc_write_aligned(const uint8_t *buf, uint32_t sector, uint32_t count)
{
    if (IS_ALIGNED(buf))
        return sd_write_disk((uint8_t *)buf, sector, count);

    /* buf 未对齐：按多扇区批量中转，减少 sd_write_disk 调用次数 */
    static uint8_t align_buf[SECTOR_SIZE * EMMC_BOUNCE_SECTORS] __attribute__((aligned(32)));
    uint32_t remain = count;
    uint32_t done = 0U;
    while (remain > 0U)
    {
        uint32_t chunk = (remain > EMMC_BOUNCE_SECTORS) ? EMMC_BOUNCE_SECTORS : remain;
        uint32_t bytes = chunk * SECTOR_SIZE;
        memcpy(align_buf, buf + done * SECTOR_SIZE, bytes);
        if (sd_write_disk(align_buf, sector + done, chunk) != 0)
            return 1;
        done += chunk;
        remain -= chunk;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  disk_read  带重试
 *  重试逻辑：操作失败 → 等待 → reinit → 再试
 *  全部失败后将 g_disk_status 置为 STA_NOINIT，
 *  FatFS 后续操作会因 disk_status 检查而快速失败，避免反复重试。
 * ------------------------------------------------------------------ */
DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != DEV_EMMC)
        return RES_PARERR;
    if (g_disk_status & STA_NOINIT)
        return RES_NOTRDY;
    if (count == 0)
        return RES_PARERR;

    for (uint8_t retry = 0; retry < DISK_RETRY_MAX; retry++)
    {
        if (emmc_read_aligned((uint8_t *)buff, (uint32_t)sector, (uint32_t)count) == 0)
            return RES_OK;

        HAL_Delay(DISK_RETRY_DLY);
        emmc_reinit(); /* reinit 失败也继续下一轮重试 */
    }

    g_disk_status = STA_NOINIT;
    return RES_ERROR;
}

/* ------------------------------------------------------------------ */
/*  disk_write  带重试                                                  */
/* ------------------------------------------------------------------ */
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != DEV_EMMC)
        return RES_PARERR;
    if (g_disk_status & STA_NOINIT)
        return RES_NOTRDY;
    if (g_disk_status & STA_PROTECT)
        return RES_WRPRT;
    if (count == 0)
        return RES_PARERR;

    for (uint8_t retry = 0; retry < DISK_RETRY_MAX; retry++)
    {
        if (emmc_write_aligned(buff, (uint32_t)sector, (uint32_t)count) == 0)
            return RES_OK;

        HAL_Delay(DISK_RETRY_DLY);
        emmc_reinit();
    }

    g_disk_status = STA_NOINIT;
    return RES_ERROR;
}

/* ------------------------------------------------------------------ */
/*  disk_ioctl
 *  ★ CTRL_SYNC：FatFS 要求此命令确保数据真正落盘。
 *    原来直接返回 RES_OK，eMMC 可能还在内部缓存中。
 *    改为等待卡进入 TRANSFER 状态，确认写入完成。
 * ------------------------------------------------------------------ */
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != DEV_EMMC)
        return RES_PARERR;
    if (g_disk_status & STA_NOINIT)
        return RES_NOTRDY;

    switch (cmd)
    {
    case CTRL_SYNC:
    {
        /* 等待 eMMC 完成内部写操作，超时返回错误 */
        uint32_t timeout = SD_TIMEOUT;
        while (HAL_MMC_GetCardState(&g_sd_handle) != HAL_MMC_CARD_TRANSFER)
        {
            HAL_Delay(1);
            if (--timeout == 0)
                return RES_ERROR;
        }
        return RES_OK;
    }

    case GET_SECTOR_COUNT:
        /* ★ 原来每次 ioctl 都调 GetCardInfo，只有需要时才刷新 */
        HAL_MMC_GetCardInfo(&g_sd_handle, &g_sd_card_info_struct);
        *(LBA_t *)buff = (LBA_t)g_sd_card_info_struct.LogBlockNbr;
        return RES_OK;

    case GET_SECTOR_SIZE:
        *(WORD *)buff = (WORD)g_sd_card_info_struct.LogBlockSize;
        return RES_OK;

    case GET_BLOCK_SIZE:
        /* KLMAG1JETD-B041: 擦除单元 512KB = 1024 扇区 */
        *(DWORD *)buff = 1024UL;
        return RES_OK;

    case CTRL_TRIM:
        return RES_OK;

    case MMC_GET_TYPE:
        *(BYTE *)buff = (BYTE)g_sd_card_info_struct.CardType;
        return RES_OK;

    case MMC_GET_CSD:
        memcpy(buff, &g_sd_handle.CSD, sizeof(g_sd_handle.CSD));
        return RES_OK;

    case MMC_GET_CID:
        memcpy(buff, &g_sd_handle.CID, sizeof(g_sd_handle.CID));
        return RES_OK;

    default:
        return RES_PARERR;
    }
}
