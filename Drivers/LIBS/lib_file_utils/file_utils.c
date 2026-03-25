/**
 ****************************************************************************************************
 * @file        file_utils.c
 * @author      ALIENTEK
 * @version     V1.0
 * @date        2026-02-12
 * @brief       File utility functions implementation
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

#include "file_utils.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "usbd_cdc_if.h"
/**
 * @brief  Get file list by extension
 * @param  dir_path: Directory path (e.g., "0:/data")
 * @param  extension: File extension (e.g., ".txt")
 * @param  file_list: File list structure to fill
 * @param  max_count: Maximum number of files to get
 * @return int8_t: 0 success, negative error code
 */
int8_t get_file_list_by_extension(const char *dir_path, const char *extension, FileList_t *file_list, uint32_t max_count)
{
    FRESULT res;
    DIR dir;
    FILINFO fno;
    uint32_t file_index = 0;
    int8_t ret = 0;

    // 验证参数
    if (dir_path == NULL || extension == NULL || file_list == NULL || max_count == 0)
    {
        return -1;
    }

    // 初始化文件列表
    file_list->count = 0;
    file_list->capacity = max_count;
    file_list->files = (FileInfo_t *)mymalloc(SRAMEX, sizeof(FileInfo_t) * max_count);
    if (file_list->files == NULL)
    {
        return -2;
    }

    // 打开目录
    res = f_opendir(&dir, dir_path);
    if (res != FR_OK)
    {
        free(file_list->files);
        file_list->files = NULL;
        return -3;
    }

    // 遍历目录
    while (1)
    {
        // 读取目录项
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0)
        {
            break; // 错误或结束
        }

        // 跳过目录
        if (fno.fattrib & AM_DIR)
        {
            continue;
        }

        // 检查文件后缀
        const char *dot = strrchr(fno.fname, '.');
        if (dot != NULL)
        {
            // 比较后缀
            if (strcasecmp(dot, extension) == 0)
            {
                // 填充文件信息
                if (file_index < max_count)
                {
                    // 构建文件路径
                    snprintf(file_list->files[file_index].path, sizeof(file_list->files[file_index].path),
                             "%s/%s", dir_path, fno.fname);

                    // 填充文件大小
                    file_list->files[file_index].size = fno.fsize;

                    // 填充创建日期和时间
                    file_list->files[file_index].create_date = fno.fdate;
                    file_list->files[file_index].create_time = fno.ftime;

                    file_index++;
                    file_list->count++;
                }
            }
        }
    }

    // 关闭目录
    f_closedir(&dir);

    return ret;
}

/**
 * @brief  Free file list
 * @param  file_list: File list structure to free
 * @return void
 */
void free_file_list(FileList_t *file_list)
{
    if (file_list != NULL && file_list->files != NULL)
    {
        myfree(SRAMEX, file_list->files);
        file_list->files = NULL;
        file_list->count = 0;
        file_list->capacity = 0;
    }
}

/**
 * @brief  Format date and time from FATFS format
 * @param  date: FATFS date format
 * @param  time: FATFS time format
 * @param  buf: Buffer to store formatted string
 * @param  buf_size: Buffer size
 * @return char*: Formatted date and time string
 */
char *format_date_time(DWORD date, DWORD time, char *buf, uint32_t buf_size)
{
    if (buf == NULL || buf_size < 20)
    {
        return NULL;
    }

    // 解析FATFS日期格式
    uint16_t year = ((date >> 9) & 0x7F) + 1980;
    uint8_t month = (date >> 5) & 0x0F;
    uint8_t day = date & 0x1F;

    // 解析FATFS时间格式
    uint8_t hour = (time >> 11) & 0x1F;
    uint8_t minute = (time >> 5) & 0x3F;
    uint8_t second = (time & 0x1F) * 2;

    // 格式化输出
    snprintf(buf, buf_size, "%04d-%02d-%02d %02d:%02d:%02d",
             year, month, day, hour, minute, second);

    return buf;
}

int32_t f_write_dma_safe(FIL *fil, const uint8_t *src, uint32_t len, UINT *bw_total)
{
#define WRITE_CHUNK_SIZE 512

    int32_t ret = FR_OK;
    uint32_t offset = 0;
    UINT bw = 0;
    *bw_total = 0;

    uint8_t dma_buf[WRITE_CHUNK_SIZE] __attribute__((aligned(4))); // 32字节对齐，确保DMA访问安全

    while (offset < len)
    {
        uint32_t chunk = ((len - offset) > WRITE_CHUNK_SIZE) ? WRITE_CHUNK_SIZE : (len - offset);

        /* 从 HyperRAM 拷贝到 AHB-SRAM */
        memcpy(dma_buf, src + offset, chunk);

        /* 刷新 D-Cache，确保 DMA 读到最新数据 */
        SCB_CleanDCache_by_Addr((uint32_t *)dma_buf, WRITE_CHUNK_SIZE);

        ret = f_write(fil, dma_buf, chunk, &bw);
        if (ret != FR_OK)
        {
            usb_printf("f_write error at offset %lu: %d", offset, ret);
            break;
        }

        *bw_total += bw;
        offset += chunk;

        if (bw < chunk)
        {
            ret = FR_DENIED;
            break;
        }
    }

    return ret;
}
