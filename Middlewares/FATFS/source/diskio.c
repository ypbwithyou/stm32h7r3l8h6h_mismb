 
#include "./MALLOC/malloc.h"
#include "./SYSTEM/usart/usart.h"
#include "./FATFS/source/ff.h"
#include "./FATFS/source/diskio.h"
#include "./BSP/NORFLASH/norflash_ex.h"
#include "./BSP/SDNAND/spi_sdnand.h"
#include "./BSP/MMC/mmc_sdcard.h"
#include <string.h>

#define DEV_EMMC 0
#define DISK_RETRY_MAX 3  /* 最大重试次数 */
#define DISK_RETRY_DLY 10 /* 重试间隔 ms  */

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
/*  对齐读（IDMA 要求4字节对齐）                                        */
/* ------------------------------------------------------------------ */
static uint8_t emmc_read_aligned(uint8_t *buf, uint32_t sector, uint32_t count)
{
    /* buf 已对齐，直接 DMA */
    if (((uint32_t)buf & 0x03) == 0)
        return sd_read_disk(buf, sector, count);

    /* 逐扇区用对齐缓冲中转 */
    static uint8_t align_buf[SECTOR_SIZE] __attribute__((aligned(4)));
    for (uint32_t i = 0; i < count; i++)
    {
        if (sd_read_disk(align_buf, sector + i, 1) != 0)
            return 1;
        memcpy(buf + i * SECTOR_SIZE, align_buf, SECTOR_SIZE);
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  对齐写                                                              */
/* ------------------------------------------------------------------ */
static uint8_t emmc_write_aligned(const uint8_t *buf, uint32_t sector, uint32_t count)
{
    if (((uint32_t)buf & 0x03) == 0)
        return sd_write_disk((uint8_t *)buf, sector, count);

    static uint8_t align_buf[SECTOR_SIZE] __attribute__((aligned(4)));
    for (uint32_t i = 0; i < count; i++)
    {
        memcpy(align_buf, buf + i * SECTOR_SIZE, SECTOR_SIZE);
        if (sd_write_disk(align_buf, sector + i, 1) != 0)
            return 1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  disk_read  带重试                                                   */
/* ------------------------------------------------------------------ */
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

        /* 读失败：等待后重新初始化再试 */
        HAL_Delay(DISK_RETRY_DLY);
        if (emmc_reinit() != 0)
            continue; /* reinit 也失败，继续下一次重试 */
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
        if (emmc_reinit() != 0)
            continue;
    }

    g_disk_status = STA_NOINIT;
    return RES_ERROR;
}

/* ------------------------------------------------------------------ */
/*  disk_ioctl                                                          */
/* ------------------------------------------------------------------ */
DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != DEV_EMMC)
        return RES_PARERR;
    if (g_disk_status & STA_NOINIT)
        return RES_NOTRDY;

    HAL_MMC_GetCardInfo(&g_sd_handle, &g_sd_card_info_struct);

    switch (cmd)
    {
    case CTRL_SYNC:
        return RES_OK;

    case GET_SECTOR_COUNT:
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

// #define SD_CARD 0   /* SD��,����Ϊ0 */
// #define NOR_FLASH 1 /* NOR Flash,����Ϊ1 */
// #define SD_NAND 2   /* SD NAND,����Ϊ2 */

// /**
//  * NOR Flash���򻮷֣����������С�������룩
//  *    ����      ��ʼ��ַ    ��С
//  *     ������ 0x00000000 0x00100000
//  * �ļ�ϵͳ�� 0x00100000 0x01800000
//  *     �ֿ��� 0x01900000 0x00604000
//  *     �û��� 0x01F04000 0x000FC000
//  */
// #define NORFLASH_SECTOR_SIZE (512)
// #define NORFLASH_SECTOR_COUNT (0x01800000 / NORFLASH_SECTOR_SIZE)
// #define NORFLASH_BLOCK_SIZE (8)
// #define NORFLASH_FATFS_BASE (0x00100000)

// // extern SD_HandleTypeDef g_sd_handle;
// extern MMC_HandleTypeDef g_sd_handle;
// extern HAL_MMC_CardInfoTypeDef g_sd_card_info_struct;

// /**
//  * @brief       ��ô���״̬
//  * @param       pdrv : ���̱��0~9
//  * @retval      ִ�н��(�μ�FATFS, DSTATUS�Ķ���)
//  */
// DSTATUS disk_status(
//     BYTE pdrv /* Physical drive nmuber to identify the drive */
// )
// {
//     // return RES_OK;
//     return 0; /* 正常状态 */
// }

// /**
//  * @brief       ��ʼ������
//  * @param       pdrv : ���̱��0~9
//  * @retval      ִ�н��(�μ�FATFS, DSTATUS�Ķ���)
//  */
// DSTATUS disk_initialize(
//     BYTE pdrv /* Physical drive nmuber to identify the drive */
// )
// {
//     uint8_t res = 0;

//     switch (pdrv)
//     {
//     case SD_CARD:         /* SD�� */
//                           //            res = sd_init();    /* SD����ʼ�� */
//         res = mmc_init(); /* SD����ʼ�� */
//         break;

//     case NOR_FLASH: /* NOR Flash */
//         norflash_ex_init();
//         break;

//     case SD_NAND:      /* SD NAND */
//         sdnand_init(); /* ��ʼ��SD NAND */
//         break;

//     default:
//         res = 1;
//     }

//     if (res)
//     {
//         return STA_NOINIT;
//     }
//     else
//     {
//         return 0;
//     }
// }

// DRESULT disk_read(
//     BYTE pdrv,    /* Physical drive nmuber to identify the drive */
//     BYTE *buff,   /* Data buffer to store read data */
//     DWORD sector, /* Sector address in LBA */
//     UINT count    /* Number of sectors to read */
// )
// {
//     uint8_t res = 0;
//     uint8_t retry = 3; /* 重试次数 */

//     if (!count)
//         return RES_PARERR;

//     switch (pdrv)
//     {
//     case SD_CARD:
//         res = sd_read_disk(buff, sector, count);

//         while (res && retry--)
//         {
//             HAL_Delay(2);

//             mmc_init();

//             res = sd_write_disk((uint8_t *)buff, sector, count);
//         }

//         break;

//     case NOR_FLASH: /* NOR Flash */
//         for (; count > 0; count--)
//         {
//             norflash_ex_read(NORFLASH_FATFS_BASE + sector * NORFLASH_SECTOR_SIZE, buff, NORFLASH_SECTOR_SIZE);
//             sector++;
//             buff += NORFLASH_SECTOR_SIZE;
//         }

//         res = 0;
//         break;

//     case SD_NAND: /* SD NAND */
//         res = sdnand_read_disk(buff, sector, count);
//         break;

//     default:
//         res = 1;
//     }

//     if (res == 0x00)
//     {
//         return RES_OK;
//     }
//     else
//     {
//         return RES_ERROR;
//     }
// }

// DRESULT disk_write(
//     BYTE pdrv,        /* Physical drive nmuber to identify the drive */
//     const BYTE *buff, /* Data to be written */
//     DWORD sector,     /* Sector address in LBA */
//     UINT count        /* Number of sectors to write */
// )
// {
//     uint8_t res = 0;
//     uint8_t retry = 3; /* 重试次数 */

//     if (!count)
//         return RES_PARERR;

//     switch (pdrv)
//     {
//     case SD_CARD:
//         res = sd_write_disk((uint8_t *)buff, sector, count);

//         while (res && retry--)
//         {
//             HAL_Delay(2);

//             mmc_init();

//             res = sd_write_disk((uint8_t *)buff, sector, count);
//         }

//         break;

//     case NOR_FLASH: /* NOR Flash */
//         for (; count > 0; count--)
//         {
//             norflash_ex_write(NORFLASH_FATFS_BASE + sector * NORFLASH_SECTOR_SIZE, (uint8_t *)buff, NORFLASH_SECTOR_SIZE);
//             sector++;
//             buff += NORFLASH_SECTOR_SIZE;
//         }

//         res = 0;
//         break;

//     case SD_NAND: /* SD NAND */
//         res = sdnand_write_disk((uint8_t *)buff, sector, count);
//         break;

//     default:
//         res = 1;
//     }

//     /* ��������ֵ��������ֵת��ff.c�ķ���ֵ */
//     if (res == 0x00)
//     {
//         return RES_OK;
//     }
//     else
//     {
//         return RES_ERROR;
//     }
// }

// /**
//  * @brief       ��ȡ�������Ʋ���
//  * @param       pdrv   : ���̱��0~9
//  * @param       ctrl   : ���ƴ���
//  * @param       buff   : ����/���ջ�����ָ��
//  * @retval      ִ�н��(�μ�FATFS, DRESULT�Ķ���)
//  */
// DRESULT disk_ioctl(
//     BYTE pdrv, /* Physical drive nmuber (0..) */
//     BYTE cmd,  /* Control code */
//     void *buff /* Buffer to send/receive control data */
// )
// {
//     DRESULT res;

//     if (pdrv == SD_CARD) /* SD�� */
//     {
//         // 使用全局变量g_sd_card_info_struct，它在mmc_init中已经初始化
//         switch (cmd)
//         {
//         case CTRL_SYNC:
//             res = RES_OK;
//             break;

//         case GET_SECTOR_SIZE:
//             *(DWORD *)buff = 512;
//             res = RES_OK;
//             break;

//         case GET_BLOCK_SIZE:
//             *(WORD *)buff = g_sd_card_info_struct.LogBlockSize;
//             res = RES_OK;
//             break;

//         case GET_SECTOR_COUNT:
//             *(DWORD *)buff = g_sd_card_info_struct.LogBlockNbr;
//             res = RES_OK;
//             break;

//         default:
//             res = RES_PARERR;
//             break;
//         }
//     }
//     else if (pdrv == NOR_FLASH) /* NOR Flash */
//     {
//         switch (cmd)
//         {
//         case CTRL_SYNC:
//             res = RES_OK;
//             break;

//         case GET_SECTOR_SIZE:
//             *(WORD *)buff = NORFLASH_SECTOR_SIZE;
//             res = RES_OK;
//             break;

//         case GET_BLOCK_SIZE:
//             *(WORD *)buff = NORFLASH_BLOCK_SIZE;
//             res = RES_OK;
//             break;

//         case GET_SECTOR_COUNT:
//             *(DWORD *)buff = NORFLASH_SECTOR_COUNT;
//             res = RES_OK;
//             break;

//         default:
//             res = RES_PARERR;
//             break;
//         }
//     }
//     else if (pdrv == SD_NAND) /* SDNAND */
//     {
//         switch (cmd)
//         {
//         case CTRL_SYNC:
//             res = RES_OK;
//             break;

//         case GET_SECTOR_SIZE:
//             *(WORD *)buff = 512;
//             res = RES_OK;
//             break;

//         case GET_BLOCK_SIZE:
//             *(WORD *)buff = sdnand_info.logic_block_size;
//             res = RES_OK;
//             break;

//         case GET_SECTOR_COUNT:
//             *(DWORD *)buff = sdnand_info.logic_block_num;
//             res = RES_OK;
//             break;

//         default:
//             res = RES_PARERR;
//             break;
//         }
//     }
//     else
//     {
//         res = RES_ERROR; /* �����Ĳ�֧�� */
//     }

//     return res;
// }
