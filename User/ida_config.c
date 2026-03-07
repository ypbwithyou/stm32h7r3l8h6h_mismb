#include "app_common.h"
#include "ida_config.h"
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/TIMER/gtim.h"
#include "./BSP/ADS8319/ads8319.h"
#include "./BSP/SPI/spi.h"
#include "./BSP/LED/led.h"
#include "./BSP/HYPERRAM/hyperram.h"
#include "./BSP/MMC/mmc_sdcard.h"
#include "./BSP/PCA9554A/pca9554a.h"
#include "./BSP/RS485/rs485.h"
#include "./BSP/RTC/rtc.h"
#include "./MALLOC/malloc.h"
#include "./FATFS/exfuns/exfuns.h"
#include "./FATFS/source/diskio.h"
#include "./LIBS/lib_dwt/lib_dwt_timestamp.h"
#include "./LIBS/lib_usb_protocol/slidingWindowReceiver_c.h"

#include "rs485_processor.h"
#include "offline_processor.h"
#include "usb_processor.h"
#include "collector_processor.h"

#include "usbd_cdc_if.h"

/***********************************宏定义*********************************/

#define SD_CARD_TEST 0
#define FILESYSTEM_TEST 0

/***********************************函数声明*********************************/
// handle references
static void CheckWorkMode(void);
static void CheckMcuPwrStatus(void);
static void CheckMcuRunStatus(void);

static int8_t format_emmc(void);
static int8_t test_filesystem(void);

FRESULT safe_f_mount(FATFS *fs, const TCHAR *drive, BYTE opt, uint8_t max_retries);
int8_t check_filesystem_status(const TCHAR *drive);

/***********************************全局变量*********************************/
///* USBD句柄 */
// USBD_HandleTypeDef g_usbd_handle = {0};
/* 系统运行状态 */
volatile SystemStatus g_IdaSystemStatus;

/***********************************函数*********************************/
/**
 * @brief   外设初始化
 * @param   无
 * @retval  初始化结果：
 */
int8_t IdaDeviceInit(void)
{
    delay_init(600);       /* 初始化延时 */
    rtc_init();            /* RTC初始化 */
    led_init();            /* 初始化LED */
    hyperram_init();       /* HyperRAM初始化 */
    my_mem_init(SRAMIN);   /* 内部SRAM内存池初始化 */
    my_mem_init(SRAMEX);   /* 外部hyperram内存池初始化 */
    my_mem_init(SRAM12);   /* 初始化AHB-SRAM1~2内存池 */
    my_mem_init(SRAMDTCM); /* 初始化DTCM内存池 */
    my_mem_init(SRAMITCM); /* 初始化ITCM内存池 */

    // 启动系统滴答时钟
    ticks_timx_start();

    // 初始化DWT单元
    if (!dwt_init())
    {
        // 处理错误：DWT不可用
        LED0(OFF);
        return RET_ERROR;
    }
    // 开始计时
    dwt_start();

    // PCA9554A初始化
    PCA9554A_init();

    // RS485初始化
    rs485_init(RS485_BAUDRATE);

    // 采集模块初始化
    ads8319_spi_gpio_init(SPI1_SPI);
    ads8319_spi_gpio_init(SPI2_SPI);
    ads8319_spi_gpio_init(SPI3_SPI);
    ads8319_common_gpio_init();

    usb_init();

    delay_ms(1000);
    usb_printf("USB_CDC OK \r\n");

    /* eMMC init */
    SystemCoreClockUpdate();

    if (mmc_init() != 0)
    {
        usb_printf("[eMMC] Init failed\r\n");
        LED0(OFF);
        return RET_ERROR;
    }

    /* verify read sector 0 */
    static uint8_t sector[512] __attribute__((aligned(4)));
    DRESULT dr = disk_read(0, sector, 0, 1);
    if (dr != RES_OK)
    {
        usb_printf("[eMMC] WARNING: sector 0 read failed, code=%d\r\n", dr);
        usb_printf("[eMMC] Possible cause: hardware / clock / pull-up issue\r\n");
        LED0(OFF);
        return RET_ERROR;
    }
    usb_printf("[eMMC] Sector 0 read OK\r\n");

    /* print card info */
    HAL_MMC_CardInfoTypeDef card_info;
    if (HAL_MMC_GetCardInfo(&g_sd_handle, &card_info) == HAL_OK)
    {
        uint64_t capacity = (uint64_t)card_info.BlockNbr * card_info.BlockSize;

        usb_printf("[eMMC] === Card Info ===\r\n");
        usb_printf("[eMMC] CardType     : %lu\r\n", card_info.CardType);
        usb_printf("[eMMC] Class        : %lu\r\n", card_info.Class);
        usb_printf("[eMMC] RelCardAddr  : 0x%04lX\r\n", card_info.RelCardAdd);
        usb_printf("[eMMC] BlockNbr     : %lu\r\n", card_info.BlockNbr);
        usb_printf("[eMMC] BlockSize    : %lu bytes\r\n", card_info.BlockSize);
        usb_printf("[eMMC] LogBlockNbr  : %lu\r\n", card_info.LogBlockNbr);
        usb_printf("[eMMC] LogBlockSize : %lu bytes\r\n", card_info.LogBlockSize);
        usb_printf("[eMMC] Capacity     : %lu MB / %lu GB\r\n",
                   (uint32_t)(capacity / (1024UL * 1024UL)),
                   (uint32_t)(capacity / (1024UL * 1024UL * 1024UL)));
        usb_printf("[eMMC] ==================\r\n");
    }
    else
    {
        usb_printf("[eMMC] HAL_MMC_GetCardInfo failed\r\n");
    }

    /* print SDMMC clock */
    uint32_t sdmmc_clk = HAL_RCCEx_GetPeriphCLKFreq(RCC_PERIPHCLK_SDMMC12);
    usb_printf("[eMMC] SDMMC clock  : %lu Hz (%.1f MHz)\r\n",
               sdmmc_clk, (float)sdmmc_clk / 1000000.0f);

    /* filesystem init */
    FRESULT res = exfuns_init();
    if (res != FR_OK)
    {
        usb_printf("[FatFs] exfuns_init failed, res=%d\r\n", res);
        return RET_ERROR;
    }

    /* mount eMMC */
    res = safe_f_mount(fs[0], "0:", 1, 2);
    if (res == FR_NO_FILESYSTEM)
    {
        /* no filesystem found, format and remount */
        usb_printf("[FatFs] No filesystem, formatting...\r\n");

        if (format_emmc() != FR_OK)
        {
            usb_printf("[FatFs] Format failed\r\n");
            return RET_ERROR;
        }
        usb_printf("[FatFs] Format OK, remounting...\r\n");

        res = safe_f_mount(fs[0], "0:", 1, 2);
        if (res != FR_OK)
        {
            usb_printf("[FatFs] Remount after format failed, res=%d\r\n", res);
            return RET_ERROR;
        }
        usb_printf("[FatFs] Remount OK\r\n");
    }
    else if (res != FR_OK)
    {
        usb_printf("[FatFs] Mount failed, res=%d\r\n", res);
        check_filesystem_status("0:");
        return RET_ERROR;
    }
    else
    {
        usb_printf("[FatFs] Mount OK\r\n");
    }

    check_filesystem_status("0:");

    test_filesystem();

    return RET_OK;
}

/**
 * @brief  Format eMMC with exFAT
 * @warning All data will be erased!
 * @retval 0: success, -1: failed
 */
static int8_t format_emmc(void)
{
    FRESULT fres;
    MKFS_PARM opt;
    DSTATUS status;
    DWORD sector_count = 0;

    /* work buffer: at least FF_MAX_SS, must be aligned */
    static BYTE work_buffer[4096] __attribute__((aligned(4)));

    usb_printf("[FatFs] === Format eMMC ===\r\n");
    usb_printf("[FatFs] WARNING: All data will be erased!\r\n");

    /* Step1: disk initialize */
    status = disk_initialize(0);
    if (status != 0)
    {
        usb_printf("[FatFs] disk_initialize failed, status=0x%02X\r\n", status);
        return -1;
    }

    /* Step2: get sector count */
    DRESULT dres = disk_ioctl(0, GET_SECTOR_COUNT, &sector_count);
    if (dres != RES_OK)
    {
        usb_printf("[FatFs] GET_SECTOR_COUNT failed, res=%d\r\n", dres);
        return -1;
    }
    usb_printf("[FatFs] Sector count : %lu\r\n", sector_count);
    usb_printf("[FatFs] Capacity     : %.2f GB\r\n",
               (float)sector_count * 512.0f / 1024.0f / 1024.0f / 1024.0f);

    /* Step3: build mkfs parameters */
    memset(&opt, 0, sizeof(opt));

    opt.fmt = FM_EXFAT; /* exFAT for 16GB eMMC */
    opt.n_fat = 1;
    opt.align = 1024; /* align to eMMC erase block: 1024 sectors = 512KB */

    /* AU size: follow Microsoft exFAT recommended table */
    if (sector_count <= 532480)
        opt.au_size = 4096; /*  < 260MB :  4KB */
    else if (sector_count <= 16777216)
        opt.au_size = 32768; /*  <   8GB : 32KB */
    else
        opt.au_size = 131072; /* >= 8GB  :128KB */

    usb_printf("[FatFs] Format type  : exFAT\r\n");
    usb_printf("[FatFs] AU size      : %lu bytes\r\n", opt.au_size);
    usb_printf("[FatFs] Align        : %lu sectors\r\n", opt.align);

    /* Step4: unmount before format */
    f_mount(NULL, "0:", 0);

    /* Step5: format */
    usb_printf("[FatFs] Formatting, please wait...\r\n");
    fres = f_mkfs("0:", &opt, work_buffer, sizeof(work_buffer));
    if (fres != FR_OK)
    {
        usb_printf("[FatFs] f_mkfs failed, res=%d", fres);
        switch (fres)
        {
        case FR_DISK_ERR:
            usb_printf(" (FR_DISK_ERR)\r\n");
            break;
        case FR_NOT_READY:
            usb_printf(" (FR_NOT_READY)\r\n");
            break;
        case FR_WRITE_PROTECTED:
            usb_printf(" (FR_WRITE_PROTECTED)\r\n");
            break;
        case FR_INVALID_DRIVE:
            usb_printf(" (FR_INVALID_DRIVE)\r\n");
            break;
        case FR_MKFS_ABORTED:
            usb_printf(" (FR_MKFS_ABORTED)\r\n");
            break;
        default:
            usb_printf(" (unknown)\r\n");
            break;
        }
        return -1;
    }
    usb_printf("[FatFs] Format OK\r\n");

    /* Step6: remount */
    HAL_Delay(100);
    fres = f_mount(fs[0], "0:", 1);
    if (fres != FR_OK)
    {
        usb_printf("[FatFs] Remount failed, res=%d\r\n", fres);
        return -1;
    }

    usb_printf("[FatFs] Remount OK\r\n");
    usb_printf("[FatFs] === Format complete ===\r\n");
    return 0;
}

// /**
//  *
//  * @warning 这会删除所有数据！
//  */
// static int8_t format_sd_card(void)
// {
//     FRESULT fres;  /* FatFs函数返回结果 */
//     MKFS_PARM opt; /* 格式化参数 */
//     const uint8_t work_buffer[512];

//     printf("\n=== 格式化SD卡 ===\n");
//     printf("警告：这将删除所有数据！\n");

//     /* 配置格式化参数 */
//     memset(&opt, 0, sizeof(opt));
//     opt.fmt = FM_FAT32; /* 文件系统类型：FAT32 */
//     opt.n_fat = 1;      /* FAT表数量：1 */
//     opt.align = 0;      /* 对齐：自动 */
//     opt.n_root = 0;     /* 根目录条目数：0（FAT32自动计算） */

//     /* 获取SD卡信息 */
//     DSTATUS status = disk_initialize(0);
//     if (status != 0)
//     {
//         printf("磁盘初始化失败\n");
//         return -1;
//     }

//     DWORD sector_count = 0;
//     DRESULT res = disk_ioctl(0, GET_SECTOR_COUNT, &sector_count);
//     if (res != RES_OK)
//     {
//         printf("获取扇区数量失败\n");
//         return -1;
//     }

//     printf("SD卡总扇区数: %u\n", sector_count);
//     printf("总容量: %.2f GB\n", (float)sector_count * 512.0f / 1024.0f / 1024.0f / 1024.0f);

//     /* 自动选择簇大小 */
//     if (sector_count > 66600)
//     {                       /* > 32MB */
//         opt.au_size = 4096; /* 4KB簇大小 */
//     }
//     if (sector_count > 532480)
//     {                       /* > 260MB */
//         opt.au_size = 8192; /* 8KB簇大小 */
//     }
//     if (sector_count > 16777216)
//     {                        /* > 8GB */
//         opt.au_size = 16384; /* 16KB簇大小 */
//     }

//     printf("使用簇大小: %u 字节\n", opt.au_size);

//     /* 执行格式化 */
//     printf("正在格式化...\n");
//     fres = f_mkfs("0:", &opt, (void *)work_buffer, sizeof(work_buffer));
//     //    fres = f_mkfs("0:", &opt, NULL, 0);

//     if (fres == FR_OK)
//     {
//         printf("格式化成功！\n");

//         /* 重新挂载 */
//         f_mount(NULL, "0:", 0); // 卸载
//         HAL_Delay(100);
//         fres = f_mount(fs[0], "0:", 1);

//         if (fres == FR_OK)
//         {
//             printf("格式化后重新挂载成功！\n");
//             return 0;
//         }
//         else
//         {
//             printf("重新挂载失败: %d\n", fres);
//             return -1;
//         }
//     }
//     else
//     {
//         printf("格式化失败: %d\n", fres);

//         if (fres == FR_DISK_ERR)
//         {
//             printf("磁盘错误\n");
//         }
//         else if (fres == FR_NOT_READY)
//         {
//             printf("磁盘未准备好\n");
//         }
//         else if (fres == FR_WRITE_PROTECTED)
//         {
//             printf("磁盘写保护\n");
//         }
//         else if (fres == FR_INVALID_DRIVE)
//         {
//             printf("无效驱动器\n");
//         }
//         else if (fres == FR_MKFS_ABORTED)
//         {
//             printf("格式化被中止\n");
//         }

//         return -1;
//     }
// }

/**
 * @brief 创建测试文件和目录，验证文件系统功能
 */
static int8_t test_filesystem(void)
{
    FRESULT fres; /* FatFs function result */
    FIL fil;
    UINT bytes_written, bytes_read;
    char buffer[100];
    char read_buffer[100];

    usb_printf("\n=== Filesystem Functionality Test ===\n");

    /* 1. Create directory */
    fres = f_mkdir("0:/TestDir");
    if (fres == FR_OK)
    {
        usb_printf("Directory created successfully: /TestDir\n");
    }
    else if (fres == FR_EXIST)
    {
        usb_printf("Directory already exists: /TestDir\n");
    }
    else
    {
        usb_printf("Failed to create directory: %d\n", fres);
        return -1;
    }

    /* 2. Create file and write data */
    fres = f_open(&fil, "0:/TestDir/test.txt", FA_CREATE_ALWAYS | FA_WRITE);
    if (fres == FR_OK)
    {
        usb_printf("File created successfully: test.txt\n");

        /* Write test data */
        const char *text = "Hello, FatFS on STM32H7!\nThis is a test file.\n";
        fres = f_write(&fil, text, strlen(text), &bytes_written);

        if (fres == FR_OK && bytes_written == strlen(text))
        {
            usb_printf("Data written successfully, %u bytes\n", bytes_written);
        }
        else
        {
            usb_printf("Failed to write data: %d\n", fres);
            f_close(&fil);
            return -1;
        }

        /* Close file */
        fres = f_close(&fil);
        if (fres != FR_OK)
        {
            usb_printf("Failed to close file after write: %d\n", fres);
            return -1;
        }
    }
    else
    {
        usb_printf("Failed to create/open file: %d\n", fres);
        return -1;
    }

    // /* 3. Re-open file and read data */
    // fres = f_open(&fil, "0:/OfflineCfgSchedule.txt", FA_READ);
    // if (fres == FR_OK)
    // {
    //     usb_printf("File opened successfully for reading\n");

    //     /* Read data */
    //     fres = f_read(&fil, read_buffer, sizeof(read_buffer) - 1, &bytes_read);
    //     if (fres == FR_OK)
    //     {
    //         read_buffer[bytes_read] = '\0'; /* Null-terminate the string */
    //         usb_printf("Read %u bytes successfully\n", bytes_read);
    //         usb_printf("File content:\n%s\n", read_buffer);
    //     }
    //     else
    //     {
    //         usb_printf("Failed to read data: %d\n", fres);
    //     }

    //     /* Close file */
    //     f_close(&fil);
    // }
    // else
    // {
    //     usb_printf("Failed to open file for reading: %d\n", fres);
    //     return -1;
    // }

    /* 4. List directory contents */
    DIR dir;
    FILINFO fno;
    usb_printf("\nDirectory listing: /TestDir\n");

    fres = f_opendir(&dir, "0:/RecordDataFiles");
    if (fres == FR_OK)
    {
        while (1)
        {
            fres = f_readdir(&dir, &fno);
            if (fres != FR_OK || fno.fname[0] == 0)
            {
                break;
            }

            if (fno.fattrib & AM_DIR)
            {
                usb_printf("  [DIR]  %s\n", fno.fname);
            }
            else
            {
                usb_printf("  [FILE] %s (%llu bytes)\n", fno.fname, fno.fsize);
            }
        }
        f_closedir(&dir);
    }
    else
    {
        usb_printf("Failed to open directory: %d\n", fres);
    }

    /* 5. Get filesystem information */
    DWORD free_clusters, total_clusters;
    FATFS *fs_ptr;
    fres = f_getfree("0:", &free_clusters, &fs_ptr);

    if (fres == FR_OK)
    {
        total_clusters = fs_ptr->n_fatent - 2; /* Subtract 2 reserved clusters */
        uint32_t sector_size = 512;            /* Assuming 512-byte sector */

        uint64_t total_mb = (uint64_t)total_clusters * fs_ptr->csize * sector_size / 1024 / 1024;
        uint64_t free_mb = (uint64_t)free_clusters * fs_ptr->csize * sector_size / 1024 / 1024;

        usb_printf("\nFilesystem Information:\n");
        usb_printf("  Total clusters: %lu\n", total_clusters);
        usb_printf("  Free clusters:  %lu\n", free_clusters);
        usb_printf("  Used clusters:  %lu\n", total_clusters - free_clusters);
        usb_printf("  Total capacity: %llu MB\n", total_mb);
        usb_printf("  Free space:     %llu MB\n", free_mb);
        if (total_clusters > 0)
        {
            usb_printf("  Usage:          %.1f%%\n",
                       (float)(total_clusters - free_clusters) * 100.0f / total_clusters);
        }
    }
    else
    {
        usb_printf("Failed to get filesystem free space: %d\n", fres);
    }

    usb_printf("\nFilesystem test completed!\n");

    return 0;
}

/**
 * @brief  Safe mount with retry and reinit
 * @param  fs          FATFS object pointer
 * @param  drive       Drive path e.g. "0:"
 * @param  opt         Mount option (0:lazy, 1:immediate)
 * @param  max_retries Max retry count
 * @retval FR_OK: success, other: FatFs error code
 */
FRESULT safe_f_mount(FATFS *fs, const TCHAR *drive, BYTE opt, uint8_t max_retries)
{
    FRESULT res = FR_NOT_READY;

    for (uint8_t retry = 0; retry < max_retries; retry++)
    {
        usb_printf("[FatFs] Mount attempt %u/%u, drive=%s\r\n",
                   retry + 1, max_retries, drive);

        res = f_mount(fs, drive, opt);
        if (res == FR_OK)
        {
            usb_printf("[FatFs] Mount OK\r\n");
            return FR_OK;
        }

        usb_printf("[FatFs] Mount failed, res=%d", res);
        switch (res)
        {
        case FR_NO_FILESYSTEM:
            usb_printf(" (FR_NO_FILESYSTEM)\r\n");
            break;
        case FR_DISK_ERR:
            usb_printf(" (FR_DISK_ERR)\r\n");
            break;
        case FR_NOT_READY:
            usb_printf(" (FR_NOT_READY)\r\n");
            break;
        case FR_INVALID_DRIVE:
            usb_printf(" (FR_INVALID_DRIVE)\r\n");
            break;
        default:
            usb_printf(" (unknown)\r\n");
            break;
        }

        /* reinit eMMC */
        usb_printf("[FatFs] Reinitializing eMMC...\r\n");
        HAL_MMC_DeInit(&g_sd_handle);
        HAL_Delay(20);

        if (mmc_init() != 0)
        {
            usb_printf("[FatFs] eMMC reinit failed\r\n");
        }
        else
        {
            usb_printf("[FatFs] eMMC reinit OK, "
                       "blocks=%lu size=%lu\r\n",
                       g_sd_card_info_struct.LogBlockNbr,
                       g_sd_card_info_struct.LogBlockSize);
        }

        /* unmount stale fs object before next attempt */
        f_mount(NULL, drive, 0);
        HAL_Delay(100);
    }

    usb_printf("[FatFs] Mount failed after %u retries, last res=%d\r\n",
               max_retries, res);
    return res;
}

// /**
//  * @brief 安全挂载文件系统
//  * @param drive 驱动器号
//  * @param fs 文件系统对象
//  * @param max_retries 最大重试次数
//  * @return FRESULT 结果
//  */
// FRESULT safe_f_mount(FATFS *fs, const TCHAR *drive, BYTE opt, uint8_t max_retries)
// {
//     FRESULT res;
//     uint8_t retry = 0;

//     while (retry < max_retries)
//     {
//         printf("第 %d 次尝试挂载文件系统...\n", retry + 1);

//         // 检查SD卡状态
//         printf("挂载前SD卡状态: ");
//         sd_get_status();

//         res = f_mount(fs, drive, opt);
//         if (res == FR_OK)
//         {
//             return res;
//         }

//         retry++;
//         printf("挂载文件系统失败: %d, 重试 %d/%d\n", res, retry, max_retries);

//         // 重新初始化SD卡
//         if (mmc_init() != 0)
//         {
//             printf("SD卡重新初始化失败\n");
//         }
//         else
//         {
//             printf("SD卡重新初始化成功\n");
//             // 检查初始化后的SD卡状态
//             printf("重新初始化后SD卡状态: ");
//             sd_get_status();
//         }

//         HAL_Delay(100);
//     }

//     return res;
// }

// /**
//  * @brief 检查文件系统状态
//  * @param drive 驱动器号
//  * @return 0 正常, -1 异常
//  */
// int8_t check_filesystem_status(const TCHAR *drive)
// {
//     FRESULT res;
//     DWORD free_clusters, total_clusters;
//     FATFS *fs_ptr;

//     res = f_getfree(drive, &free_clusters, &fs_ptr);
//     if (res != FR_OK)
//     {
//         printf("检查文件系统失败: %d\n", res);
//         return -1;
//     }

//     total_clusters = fs_ptr->n_fatent - 2;
//     printf("文件系统状态: 总簇数=%u, 空闲簇数=%u\n", total_clusters, free_clusters);

//     return 0;
// }

/**
 * @brief  Check filesystem status and print capacity info
 * @param  drive  Drive path e.g. "0:"
 * @retval 0: OK, -1: failed
 */
int8_t check_filesystem_status(const TCHAR *drive)
{
    FRESULT res;
    DWORD free_clusters;
    FATFS *fs_ptr;

    res = f_getfree(drive, &free_clusters, &fs_ptr);
    if (res != FR_OK)
    {
        usb_printf("[FatFs] f_getfree failed, res=%d", res);
        switch (res)
        {
        case FR_NO_FILESYSTEM:
            usb_printf(" (FR_NO_FILESYSTEM)\r\n");
            break;
        case FR_DISK_ERR:
            usb_printf(" (FR_DISK_ERR)\r\n");
            break;
        case FR_NOT_READY:
            usb_printf(" (FR_NOT_READY)\r\n");
            break;
        default:
            usb_printf(" (unknown)\r\n");
            break;
        }
        return -1;
    }

    /* exFAT: n_fatent includes reserved entries, subtract 2 for valid clusters */
    DWORD total_clusters = fs_ptr->n_fatent - 2;
    DWORD used_clusters = total_clusters - free_clusters;

    /* cluster size in bytes */
    DWORD cluster_size = fs_ptr->csize * 512UL; /* csize: sectors per cluster */

    uint64_t total_bytes = (uint64_t)total_clusters * cluster_size;
    uint64_t free_bytes = (uint64_t)free_clusters * cluster_size;
    uint64_t used_bytes = (uint64_t)used_clusters * cluster_size;

    usb_printf("[FatFs] === Filesystem Status ===\r\n");
    usb_printf("[FatFs] Cluster size  : %lu bytes\r\n", cluster_size);
    usb_printf("[FatFs] Total clusters: %lu\r\n", total_clusters);
    usb_printf("[FatFs] Used clusters : %lu\r\n", used_clusters);
    usb_printf("[FatFs] Free clusters : %lu\r\n", free_clusters);
    usb_printf("[FatFs] Total space   : %lu MB\r\n", (uint32_t)(total_bytes / 1024 / 1024));
    usb_printf("[FatFs] Used space    : %lu MB\r\n", (uint32_t)(used_bytes / 1024 / 1024));
    usb_printf("[FatFs] Free space    : %lu MB\r\n", (uint32_t)(free_bytes / 1024 / 1024));
    usb_printf("[FatFs] ==============================\r\n");

    return 0;
}

static void CheckWorkMode(void)
{
    ;
}
/**
 * @brief   系统电源状态指示灯输出控制
 * @param   无
 * @retval  无
 */
static void CheckMcuPwrStatus(void)
{
    SysPwrLED_Output();
}
/**
 * @brief   系统运行指示灯输出控制
 * @param   无
 * @retval  无
 */
static void CheckMcuRunStatus(void)
{
    static uint16_t i = 0;
    uint8_t ret = PCA9554A_OK;
    uint8_t read_val = 0;
    static uint32_t t_last = 0;
    uint32_t t_now = 0;
    uint32_t t_off = 0;

    t_now = ticks_timx_get_counter();

    ret = PCA9554_Get(&read_val);
    if (ret != PCA9554A_OK)
    {
        // printf("PCA9554A_ReadReg err.\n");
    }

    if ((g_IdaSystemStatus.st_dev_run.run_flag != 1) && (g_IdaSystemStatus.st_dev_link.link_status != USB_CONNECTED))
    {
        // 未连接且未运行时红灯常亮
        PCA9554_Set(PCA9544A_STS_R(read_val));
    }
    else if ((g_IdaSystemStatus.st_dev_run.run_flag != 1) && (g_IdaSystemStatus.st_dev_link.link_status == USB_CONNECTED))
    {
        // 已连接但未运行时蓝灯常亮
        PCA9554_Set(PCA9544A_STS_B(read_val));
    }
    else if ((g_IdaSystemStatus.st_dev_run.run_flag == 1) && (g_IdaSystemStatus.st_dev_link.link_status != USB_CONNECTED))
    {
        // 未连接但已运行时蓝灯闪烁（离线运行）
        t_off = t_now - t_last;
        if (t_off > 500)
        {
            t_last = t_now;
            PCA9554_Set(PCA9544A_STS_B(read_val));
        }
        else
        {
            PCA9554_Set(PCA9544A_STS_I(read_val));
        }
    }
    else if ((g_IdaSystemStatus.st_dev_run.run_flag == 1) && (g_IdaSystemStatus.st_dev_record.record_status != RECORD_RUN))
    {
        // 已运行但未记录时绿灯常亮
        PCA9554_Set(PCA9544A_STS_G(read_val));
    }
    else if ((g_IdaSystemStatus.st_dev_run.run_flag == 1) && (g_IdaSystemStatus.st_dev_record.record_status == RECORD_RUN))
    {
        // 已运行且在记录中时绿灯闪烁（离线记录）
        t_off = t_now - t_last;
        if (t_off > 100)
        {
            t_last = t_now;
            PCA9554_Set(PCA9544A_STS_G(read_val));
        }
        else
        {
            PCA9554_Set(PCA9544A_STS_I(read_val));
        }
    }
}
// rs485写子卡设备信息测试
void WriteSubDevicelnfo_test(void)
{
    SubDevicelnfo subden_info;

    strcpy((char *)&subden_info.SerialNumber[0], "MIRA3102501001");
    strcpy((char *)&subden_info.DeviceName[0], Name_Mini_SliceAccel);
    strcpy((char *)&subden_info.Version[0], "1.0.0.0_20260114");
    subden_info.DeviceType = Mini_SliceAccel;
    subden_info.SlotId = 1;
    subden_info.Sensitivity = 3; // 3: 12.5g

    int8_t ret = rs485_send_frame(1, 0x02, (uint8_t *)&subden_info, sizeof(SubDevicelnfo));
    if (ret != RET_OK)
    {
        usb_printf("rs485_send_frame err.\n");
    }
}
/**
 * @brief   核心处理函数
 * @param   无
 * @retval  处理结果
 */
int8_t app_processor(void)
{
    SlidingWindowReceiver_C receiver;
    uint8_t usb_rx_buf[USBD_CDC_RX_BUF_SIZE];
    uint16_t data_len = 0;
    int8_t ret = RET_OK;
    uint8_t rs485buf[RS485_RX_BUF_LEN];
    uint8_t len = 0;
    uint8_t pending_frames = 0;
    uint32_t t_last = 0;
    uint32_t t_now = 0;
    uint32_t t_off = 0;

    // 初始化采集数据缓存Buffer
    g_cb_adc = collect_cb_init(SPI_NUM * ADC_DATA_LEN * SPI_CH_ADC_MAX * BLOCK_LEN);
    if (!g_cb_adc)
    {
        usb_printf("collect_cb_init err.\n");
        return 0;
    }

    // 滑动窗口初始化
    // SWR_Init(&receiver, on_frame, NULL);

    // 系统运行参数初始化
    SystemStatusInit();

    // 离线初始化
    SysRunStatusInit();

    //    // 写子卡设备信息测试
    //    WriteSubDevicelnfo_test();
    //
    //    // 获取BASE卡设备信息（设备信息预期不符则使用默认设备信息）
    //    GetDeviceInfo(&g_dev_info);

    //    // 轮询查询子卡设备信息
    //    for (uint8_t i=0; i<SUBDEV_NUM_MAX; i++) {
    //       rs485_send_frame(i+1, 0x01, NULL, 0);
    //    }
    //    t_last = ticks_timx_get_counter();
    //    while (t_off < 1000) {
    //       /* 处理所有待处理的帧 */
    //       pending_frames = rs485_get_pending_frames();
    //
    //       for (uint8_t i = 0; i < pending_frames; i++) {
    //           /* 从队列中获取数据帧 */
    //           if (rs485_recv_data(rs485buf, &len) == 0 && len > 0) {
    //               /* 解析数据帧 */
    //               rs485_parse_frame(rs485buf, len);
    //               /* 可选：数据回环测试（自动添加帧头帧尾） */
    //    //                rs485_send_data(rs485buf, len);
    //           }
    //       }
    //       t_now = ticks_timx_get_counter();
    //       t_off = t_now - t_last;
    //    }

    while (1)
    {
        t_now = ticks_timx_get_counter();

        t_off = t_now - t_last;
        // 系统指示灯控制
        if (t_off > 500 * 1000)
        {
            t_last = t_now;
            CheckMcuPwrStatus();
            CheckMcuRunStatus();
            LED0_TOGGLE();
        }

        //       // START事件处理（事件发生时执行一次离线计划表offline_processor，计划表执行期间的重复事件视为同一次）
        //       if (g_IdaSystemStatus.st_dev_offline.start_flag == 1) {
        //           offline_processor(g_IdaSystemStatus.st_dev_offline.start_flag);
        //       }

        // offline_processor(1);

        // USB通信数据处理
        // USB_CDC_Receive_From_Queue(usb_rx_buf, &data_len);
        // if (data_len > 0)
        // {
        //     /* 数据回显（可根据需要修改为业务逻辑） */
        //     SWR_ProcessBytes(&receiver, usb_rx_buf, data_len);
        // }
        // ─── 最高优先级：接收到的完整协议帧 ───
        if (g_slidingWindow_receiver.frame_flag)
        {
            uint32_t len = g_slidingWindow_receiver.frame_len_current;
            on_frame(g_slidingWindow_receiver.buffer, len);
            g_slidingWindow_receiver.frame_flag = 0;
        }

        // USB_Display_All(g_IdaSystemStatus.st_dev_run.run_flag && g_IdaSystemStatus.st_dev_mode.work_mode != WORKMODE_ONLINE);

        IdaProcessor();
    }
    return ret;
}
