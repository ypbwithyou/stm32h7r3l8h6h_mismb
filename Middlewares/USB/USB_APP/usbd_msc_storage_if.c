/**
  ****************************************************************************************************
  * @file        usbd_msc_storage_if.c
  * @brief       USB MSC (只读U盘) 后端实现
  *
  * 说明：
  *  - 本文件的 Read 回调在 USB 中断上下文执行(usbd_msc_scsi.c:SCSI_ProcessRead)，
  *    严禁调用 HAL_Delay/mmc_init 等耗时或依赖低优先级中断的函数；
  *  - 与本机 FatFS 的互斥由 emmc_arbitrator 负责：
  *      IsReady() 在本机操作磁盘期间返回"未就绪"，主机得到 NOT_READY 后自动重试；
  *  - 本 U 盘为只读：IsWriteProtected 恒为 1，Write 回调直接失败。
  ****************************************************************************************************
  */

#include "usbd_msc_storage_if.h"
#include "./BSP/MMC/mmc_sdcard.h"
#include "./FATFS/source/emmc_arbitrator.h"
#include <string.h>

extern HAL_MMC_CardInfoTypeDef g_sd_card_info_struct;

/* USB Mass storage Standard Inquiry Data (36 bytes) */
static int8_t s_msc_inquiry_data[STANDARD_INQUIRY_DATA_LEN] =
{
    /* LUN 0 */
    0x00,                              /* Peripheral: direct access block device */
    0x80,                              /* RMB = 1 (removable)                     */
    0x02,                              /* ANSI SCSI revision                      */
    0x02,                              /* Response format                         */
    (STANDARD_INQUIRY_DATA_LEN - 5),   /* Additional length                       */
    0x00,
    0x00,
    0x00,
    'A', 'L', 'I', 'E', 'N', 'T', 'E', 'K',  /* Vendor  : 8 bytes  */
    'e', 'M', 'M', 'C', ' ', 'D', 'i', 's',  /* Product : 16 bytes */
    'k', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    '1', '.', '0', '0'                        /* Revision: 4 bytes   */
};

/* MSC 侧独立的 32 字节对齐中转缓冲（DMA + DCache 要求）。
 * 不可与 diskio.c 的中转缓冲共用：两者分属不同上下文（本机主循环 / USB中断） */
static uint8_t s_msc_sector_buf[512] __attribute__((aligned(32)));

/**
  * @brief  初始化 LUN
  *         在 USB 中断上下文被调用，只做状态检查，不做任何初始化动作。
  *         eMMC 的初始化由本机启动流程(disk_initialize -> mmc_init)完成。
  */
static int8_t STORAGE_Init(uint8_t lun)
{
    (void)lun;
    return 0;
}

/**
  * @brief  返回容量：block_num = 总扇区数, block_size = 扇区字节数
  *         (usbd_msc_scsi.c SCSI_ReadCapacity10 会自行 -1 得到最后 LBA)
  */
static int8_t STORAGE_GetCapacity(uint8_t lun, uint32_t *block_num, uint16_t *block_size)
{
    (void)lun;

    if ((g_sd_card_info_struct.LogBlockNbr == 0U) || (g_sd_card_info_struct.LogBlockSize == 0U))
    {
        return -1; /* eMMC 尚未初始化 → 主机得到 NOT_READY，稍后重试 */
    }

    *block_num = g_sd_card_info_struct.LogBlockNbr;
    *block_size = (uint16_t)g_sd_card_info_struct.LogBlockSize;
    return 0;
}

/**
  * @brief  媒介是否就绪
  *         本机正在操作 eMMC 时返回"未就绪"，主机收到 NOT_READY sense 自动重试。
  */
static int8_t STORAGE_IsReady(uint8_t lun)
{
    (void)lun;

    if (g_sd_card_info_struct.LogBlockNbr == 0U)
    {
        return -1;
    }

    if (emmc_local_busy() != 0U)
    {
        return -1;
    }

    return 0;
}

/**
  * @brief  写保护：本 U 盘对主机只读
  */
static int8_t STORAGE_IsWriteProtected(uint8_t lun)
{
    (void)lun;
    return 1;
}

/**
  * @brief  读扇区（USB 中断上下文）
  *         SCSI 层每次最多传 1 个扇区 (MSC_MEDIA_PACKET = 512B)
  */
static int8_t STORAGE_Read(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
    (void)lun;

    if ((buf == NULL) || (blk_len == 0U))
    {
        return -1;
    }

    /* 记录主机活动：触发接管判定（仅写标记，可在中断内调用） */
    emmc_msc_touch();

    /* 兜底：本机恰在此刻占用 eMMC 时快速失败，不做任何等待
     * （USB 中断内等待 SDMMC 中断会死锁） */
    if (emmc_local_busy() != 0U)
    {
        return -1;
    }

    for (uint16_t i = 0U; i < blk_len; i++)
    {
        if (sd_read_disk(s_msc_sector_buf, blk_addr + (uint32_t)i, 1U) != 0U)
        {
            return -1;
        }
        memcpy(buf + ((uint32_t)i * 512U), s_msc_sector_buf, 512U);
    }

    return 0;
}

/**
  * @brief  写扇区：主机侧已按写保护拒绝，此处仍然拒绝（双保险）
  */
static int8_t STORAGE_Write(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
    (void)lun;
    (void)buf;
    (void)blk_addr;
    (void)blk_len;
    return -1;
}

/**
  * @brief  最大 LUN 数 - 1
  */
static int8_t STORAGE_GetMaxLun(void)
{
    return 0;
}

/* USB MSC 存储回调集合 */
USBD_StorageTypeDef USBD_MSC_fops =
{
    STORAGE_Init,
    STORAGE_GetCapacity,
    STORAGE_IsReady,
    STORAGE_IsWriteProtected,
    STORAGE_Read,
    STORAGE_Write,
    STORAGE_GetMaxLun,
    (int8_t *)s_msc_inquiry_data,
};
