#include "app_common.h"
#include "ida_config.h"
#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/TIMER/gtim.h"
#include "./BSP/ADS8319/ads8319.h"
#include "./BSP/SPI/spi.h"
#include "./BSP/SPI_DMA/spi_dma.h"
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
#include "./BSP/EXTI/exti.h"

#include "rs485_processor.h"
#include "offline_processor.h"
#include "usb_processor.h"
#include "collector_processor.h"

#include "usbd_cdc_if.h"
#include <stdio.h>
#include <stdlib.h>

#define TEST_FILE_NAME "speed_test.bin"
#define TEST_FILE_SIZE (4 * 1024 * 1024) // 4MB
#define TEST_BLOCK_SIZE (16 * 1024)      // 16KB
#define SOFT_TIME_FILE_PATH "0:/.sys_time.dat"
#define SOFT_TIME_SYNC_INTERVAL_MS 10000U
#define SOFT_TIME_DEFAULT_EPOCH 1767225600UL /* 2026-01-01 00:00:00 */

uint8_t offline_mode;

static uint32_t g_soft_time_base_epoch = SOFT_TIME_DEFAULT_EPOCH;
static uint32_t g_soft_time_base_tick = 0U;
static uint32_t g_soft_time_last_sync_tick = 0U;
static uint8_t g_soft_time_inited = 0U;
static uint64_t g_soft_time_abs_base_epoch_ns = 0ULL;
static uint64_t g_soft_time_abs_base_dwt_ns = 0ULL;
static uint8_t g_soft_time_abs_base_inited = 0U;

static uint32_t soft_time_get_epoch(void);

static uint8_t soft_time_is_leap_year(uint16_t year)
{
    return ((year % 4U == 0U && year % 100U != 0U) || (year % 400U == 0U)) ? 1U : 0U;
}

static uint8_t soft_time_days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days_tbl[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    uint8_t days = days_tbl[month - 1U];

    if ((month == 2U) && soft_time_is_leap_year(year))
    {
        days = 29U;
    }

    return days;
}

static void soft_time_set_epoch(uint32_t epoch_s)
{
    g_soft_time_base_epoch = epoch_s;
    g_soft_time_base_tick = HAL_GetTick();
    g_soft_time_inited = 1U;
}

static void soft_time_refresh_abs_ns_base(void)
{
    g_soft_time_abs_base_epoch_ns = (uint64_t)soft_time_get_epoch() * 1000000000ULL;
    g_soft_time_abs_base_dwt_ns = dwt_get_ns();
    g_soft_time_abs_base_inited = 1U;
}

static uint32_t soft_time_get_epoch(void)
{
    uint32_t now_tick;
    uint32_t elapsed_ms;

    if (g_soft_time_inited == 0U)
    {
        soft_time_set_epoch(SOFT_TIME_DEFAULT_EPOCH);
    }

    now_tick = HAL_GetTick();
    elapsed_ms = now_tick - g_soft_time_base_tick;

    return g_soft_time_base_epoch + (elapsed_ms / 1000U);
}

static void soft_time_epoch_to_calendar(uint32_t epoch_s,
                                        uint16_t *year,
                                        uint8_t *month,
                                        uint8_t *day,
                                        uint8_t *hour,
                                        uint8_t *minute,
                                        uint8_t *second)
{
    uint32_t days;
    uint32_t rem;
    uint16_t y;
    uint8_t m;
    uint32_t dim;

    days = epoch_s / 86400UL;
    rem = epoch_s % 86400UL;

    *hour = (uint8_t)(rem / 3600UL);
    rem %= 3600UL;
    *minute = (uint8_t)(rem / 60UL);
    *second = (uint8_t)(rem % 60UL);

    y = 1970U;
    while (1)
    {
        dim = soft_time_is_leap_year(y) ? 366UL : 365UL;
        if (days < dim)
        {
            break;
        }
        days -= dim;
        y++;
    }

    m = 1U;
    while (1)
    {
        dim = soft_time_days_in_month(y, m);
        if (days < dim)
        {
            break;
        }
        days -= dim;
        m++;
    }

    *year = y;
    *month = m;
    *day = (uint8_t)(days + 1UL);
}

static FRESULT soft_time_save_to_file(void)
{
    FIL fil;
    FRESULT res;
    UINT bw;
    char buf[24];
    int len;
    uint32_t epoch_s = soft_time_get_epoch();

    len = snprintf(buf, sizeof(buf), "%lu\n", (unsigned long)epoch_s);
    if (len <= 0)
    {
        return FR_INT_ERR;
    }

    res = f_open(&fil, SOFT_TIME_FILE_PATH, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK)
    {
        return res;
    }

    res = f_write(&fil, buf, (UINT)len, &bw);
    if ((res == FR_OK) && (bw != (UINT)len))
    {
        res = FR_DISK_ERR;
    }

    if (res == FR_OK)
    {
        res = f_sync(&fil);
    }

    f_close(&fil);
    return res;
}

static uint8_t soft_time_load_from_file(void)
{
    FIL fil;
    FRESULT res;
    UINT br;
    char buf[24];
    uint32_t epoch_s;
    char *endptr = NULL;

    res = f_open(&fil, SOFT_TIME_FILE_PATH, FA_READ);
    if (res != FR_OK)
    {
        return 0U;
    }

    res = f_read(&fil, buf, (UINT)(sizeof(buf) - 1U), &br);
    f_close(&fil);
    if (res != FR_OK)
    {
        return 0U;
    }

    buf[br] = '\0';
    epoch_s = (uint32_t)strtoul(buf, &endptr, 10);

    if (endptr == buf)
    {
        return 0U;
    }

    if (epoch_s < 315532800UL) /* 1980-01-01 */
    {
        return 0U;
    }

    soft_time_set_epoch(epoch_s);
    return 1U;
}

static void soft_time_init_after_mount(void)
{
    if (soft_time_load_from_file() == 0U)
    {
        soft_time_set_epoch(SOFT_TIME_DEFAULT_EPOCH);
        (void)soft_time_save_to_file();
    }

    g_soft_time_last_sync_tick = HAL_GetTick();
    soft_time_refresh_abs_ns_base();
}

static void soft_time_periodic_sync(void)
{
    uint32_t now_tick = HAL_GetTick();
    if ((now_tick - g_soft_time_last_sync_tick) >= SOFT_TIME_SYNC_INTERVAL_MS)
    {
        (void)soft_time_save_to_file();
        g_soft_time_last_sync_tick = now_tick;
    }
}

DWORD get_fattime(void)
{
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;

    soft_time_epoch_to_calendar(soft_time_get_epoch(), &year, &month, &day, &hour, &minute, &second);

    if (year < 1980U)
    {
        year = 1980U;
        month = 1U;
        day = 1U;
        hour = 0U;
        minute = 0U;
        second = 0U;
    }
    else if (year > 2107U)
    {
        year = 2107U;
        month = 12U;
        day = 31U;
        hour = 23U;
        minute = 59U;
        second = 58U;
    }

    return ((DWORD)(year - 1980U) << 25) |
           ((DWORD)month << 21) |
           ((DWORD)day << 16) |
           ((DWORD)hour << 11) |
           ((DWORD)minute << 5) |
           ((DWORD)second >> 1);
}

int8_t SoftTimeSyncFromNanoSecond(int64_t nano_second)
{
    uint64_t epoch_s;
    FRESULT res;

    if (nano_second <= 0)
    {
        return RET_ERROR;
    }

    epoch_s = (uint64_t)nano_second / 1000000000ULL;
    if (epoch_s < 315532800ULL) /* 1980-01-01 */
    {
        return RET_ERROR;
    }

    if (epoch_s > 0xFFFFFFFFULL)
    {
        epoch_s = 0xFFFFFFFFULL;
    }

    soft_time_set_epoch((uint32_t)epoch_s);
    g_soft_time_last_sync_tick = HAL_GetTick();
    soft_time_refresh_abs_ns_base();

    res = soft_time_save_to_file();
    return (res == FR_OK) ? RET_OK : RET_ERROR;
}

uint32_t SoftTimeGetEpochSecond(void)
{
    return soft_time_get_epoch();
}

uint64_t SoftTimeGetEpochNanosecond(void)
{
    uint64_t now_dwt_ns;
    if (g_soft_time_abs_base_inited == 0U)
    {
        soft_time_refresh_abs_ns_base();
    }

    now_dwt_ns = dwt_get_ns();
    return g_soft_time_abs_base_epoch_ns + (now_dwt_ns - g_soft_time_abs_base_dwt_ns);
}

void sd_file_speed_test(void)
{
    FIL file;
    FRESULT res;
    UINT bw, br;

    uint8_t *buffer = (uint8_t *)mymalloc(SRAMEX, TEST_BLOCK_SIZE);

    uint32_t total_write = 0;
    uint32_t total_read = 0;

    uint64_t t_start;
    uint64_t t_end;

    usb_printf("\r\n===== SD File Speed Test =====\r\n");

    /* 初始化测试数据 */
    for (uint32_t i = 0; i < TEST_BLOCK_SIZE; i++)
        buffer[i] = i;

    /*--------------------------------*/
    /* 写入测试 */
    /*--------------------------------*/
    usb_printf("Writing %d MB...\r\n", TEST_FILE_SIZE / 1024 / 1024);

    res = f_open(&file, TEST_FILE_NAME, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK)
    {
        usb_printf("File open failed: %d\r\n", res);
        return;
    }

    t_start = dwt_get_us();

    while (total_write < TEST_FILE_SIZE)
    {
        res = f_write(&file, buffer, TEST_BLOCK_SIZE, &bw);
        if (res != FR_OK || bw != TEST_BLOCK_SIZE)
        {
            usb_printf("Write error\r\n");
            break;
        }

        total_write += bw;
    }

    f_sync(&file);
    f_close(&file);

    t_end = dwt_get_us();

    float write_time = (t_end - t_start) / 1000000.0f;
    float write_speed = (float)total_write / 1024.0f / 1024.0f / write_time;

    usb_printf("Write time: %.3f s\r\n", write_time);
    usb_printf("Write speed: %.2f MB/s\r\n", write_speed);

    /*--------------------------------*/
    /* 读取测试 */
    /*--------------------------------*/
    usb_printf("\r\nReading file...\r\n");

    res = f_open(&file, TEST_FILE_NAME, FA_READ);
    if (res != FR_OK)
    {
        usb_printf("File open failed\r\n");
        return;
    }

    t_start = dwt_get_us();

    while (1)
    {
        res = f_read(&file, buffer, TEST_BLOCK_SIZE, &br);
        if (res != FR_OK)
        {
            usb_printf("Read error\r\n");
            break;
        }

        if (br == 0)
            break;

        total_read += br;
    }

    f_close(&file);

    t_end = dwt_get_us();

    float read_time = (t_end - t_start) / 1000000.0f;
    float read_speed = (float)total_read / 1024.0f / 1024.0f / read_time;

    usb_printf("Read time: %.3f s\r\n", read_time);
    usb_printf("Read speed: %.2f MB/s\r\n", read_speed);

    usb_printf("===== Test Finished =====\r\n\r\n");
}

/***********************************宏定义*********************************/

#define SD_CARD_TEST 0
#define FILESYSTEM_TEST 0

/***********************************函数声明*********************************/
// handle references
static void CheckWorkMode(void);
static void CheckMcuPwrStatus(void);
static void CheckMcuRunStatus(void);

static int8_t format_emmc(void);
 

FRESULT safe_f_mount(FATFS *fs, const TCHAR *drive, BYTE opt, uint8_t max_retries);
int8_t check_filesystem_status(const TCHAR *drive);

/***********************************全局变量*********************************/

uint8_t g_offline_mode; // 离线模式：0--待机，1--离线运行

/* 在全局区紧挨着放哨兵 */
volatile uint32_t g_sentinel_before = 0xDEADBEEF;
volatile SystemStatus g_IdaSystemStatus;
volatile uint32_t g_sentinel_after = 0xDEADBEEF;

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
    external_io_init();

    // RS485初始化
    rs485_init(RS485_BAUDRATE);

    // 采集模块初始化
    ads8319_spi_gpio_init(SPI1_SPI);
    ads8319_spi_gpio_init(SPI2_SPI);
    ads8319_spi_gpio_init(SPI3_SPI);
    ads8319_common_gpio_init();
    
    /* 初始化SPI DMA */
    spi_dma_init();

    usb_init();

    delay_ms(1000);
    usb_printf("USB_CDC OK \r\n");

    /* eMMC init */
    SystemCoreClockUpdate();

    // if (mmc_init() != 0)
    // {
    //     usb_printf("[eMMC] Init failed\r\n");
    //     LED0(OFF);
    //     return RET_ERROR;
    // }
    // ida_config.c 里替换为：
    DSTATUS ds = disk_initialize(0);
    if (ds != 0)
    {
        usb_printf("[eMMC] disk_initialize failed, status=%d\r\n", ds);
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
    // format_emmc();
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

    soft_time_init_after_mount();
    usb_device_info_reload_from_file();
    check_filesystem_status("0:");

    OfflineRecordInit();

    test_filesystem();

    rs485_subdev_scan_once();
 
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

/**
 * @brief 创建测试文件和目录，验证文件系统功能
 */
  int8_t test_filesystem(void)
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

    /* 3. Re-open file and read data */
    fres = f_open(&fil, "0:/OfflineCfgSchedule.txt", FA_READ);
    if (fres == FR_OK)
    {
        usb_printf("File opened successfully for reading\n");

        /* Read data */
        fres = f_read(&fil, read_buffer, sizeof(read_buffer) - 1, &bytes_read);
        if (fres == FR_OK)
        {
            read_buffer[bytes_read] = '\0'; /* Null-terminate the string */
            usb_printf("Read %u bytes successfully\n", bytes_read);
            usb_printf("File content:\n%s\n", read_buffer);
        }
        else
        {
            usb_printf("Failed to read data: %d\n", fres);
        }

        /* Close file */
        f_close(&fil);
    }
    else
    {
        usb_printf("Failed to open file for reading: %d\n", fres);
        return -1;
    }

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

        g_dev_info.fTotalDiskSapce = total_mb * 1024.0;
        g_dev_info.fFreeDiskSpace = free_mb * 1024.0;

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
    static bool led_state = 0; // 0: 灭，1: 亮

    t_now = HAL_GetTick();

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
        t_off = t_now - t_last;
        if (t_off > 500)
        {
            t_last = t_now;
            led_state = !led_state; // 切换状态
        }
        if (g_IdaSystemStatus.st_dev_record.record_status == RECORD_RUN)
        {
            // 离线记录中，绿灯闪烁
            if (led_state)
            {
                PCA9554_Set(PCA9544A_STS_G(read_val));
            }
            else
            {
                PCA9554_Set(PCA9544A_STS_I(read_val));
            }
        }
        else
        { // 离线运行未记录，蓝灯闪烁
            if (led_state)
            {
                PCA9554_Set(PCA9544A_STS_B(read_val));
            }
            else
            {
                PCA9554_Set(PCA9544A_STS_I(read_val));
            }
        }
    }
    else if ((g_IdaSystemStatus.st_dev_run.run_flag == 1) && (g_IdaSystemStatus.st_dev_record.record_status != RECORD_RUN))
    {
        // 已运行但未记录时绿灯常亮
        PCA9554_Set(PCA9544A_STS_G(read_val));
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

    g_tx_packet = (uint8_t *)mymalloc(SRAMEX, (sizeof(FrameHeadInfo) +
                                               sizeof(uint32_t) +
                                               sizeof(UserDataHeadInfo) +
                                               sizeof(ArmBackFrameHeader) +
                                               ADC_CH_TOTAL * BLOCK_LEN * sizeof(short) + sizeof(uint32_t) +
                                               sizeof(uint32_t) + 256));

    if (collect_cb_init_all(ADC_CB_SIZE_PER_CH) != RET_OK)
    {
        printf("collect_cb_init_all err.\n");
        return 0;
    }

    // 系统运行参数初始化
    SystemStatusInit();

    // 离线初始化
    SysRunStatusInit();
 
    uint8_t *frame_copy = (uint8_t *)mymalloc(SRAMEX, SWR_BUFFER_SIZE);
    if (!frame_copy)
    {
        usb_printf("Failed to allocate frame_copy buffer\n");
        return RET_ERROR;
    }

    while (1)
    {

        if (g_sentinel_before != 0xDEADBEEF ||
            g_sentinel_after != 0xDEADBEEF)
        {
            // 哨兵被踩 → 内存溢出确认
            __BKPT(0); // 触发断点
        }

        // ----------------------------------------------------------------------
        t_now = HAL_GetTick();

        t_off = t_now - t_last;
        // 系统指示灯控制
        if (t_off > 100)
        {
            t_last = t_now;
            CheckMcuPwrStatus();
            CheckMcuRunStatus();
            LED0_TOGGLE();
        }

        soft_time_periodic_sync();

        // ---------------------------emmc--------------------
        MmcAsyncState st = sd_disk_poll();
        if (st == MMC_DONE || st == MMC_ERROR)
        {
            sd_disk_reset();
        }

        ExternalIO_Process();

        rs485_processor_poll();

        offline_processor(g_offline_mode);

        // ─── 最高优先级：接收到的完整协议帧 ───
        if (g_slidingWindow_receiver.frame_flag)
        {
            // 主循环外定义，只分配一次
            uint32_t flen = g_slidingWindow_receiver.frame_len_current;

            // 1. 拷贝到静态缓冲
            memcpy(frame_copy,
                   g_slidingWindow_receiver.buffer + g_slidingWindow_receiver.frame_start_pos,
                   flen);

            // 2. 清标志
            g_slidingWindow_receiver.frame_flag = 0;

            // 3. 处理
            on_frame(frame_copy, flen);
        }

        USB_Display_All(g_IdaSystemStatus.st_dev_run.run_flag);

        IdaProcessor();
    }

    // 虽然这里不会执行到，但为了代码完整性添加释放
    myfree(SRAMEX, frame_copy);
    return ret;
}
