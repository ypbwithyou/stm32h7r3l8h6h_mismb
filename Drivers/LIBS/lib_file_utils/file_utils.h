/**
 ****************************************************************************************************
 * @file        file_utils.h
 * @author      ALIENTEK
 * @version     V1.0
 * @date        2026-02-12
 * @brief       File utility functions
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

#ifndef __FILE_UTILS_H__
#define __FILE_UTILS_H__

#include "stdint.h"
#include "./FATFS/source/ff.h"
#include "./MALLOC/malloc.h"

/**
 * @brief  File information structure
 */
typedef struct
{
    char path[256];    // 文件路径
    uint32_t size;     // 文件大小（字节）
    DWORD create_date; // 创建日期
    DWORD create_time; // 创建时间
} FileInfo_t;

/**
 * @brief  File list structure
 */
typedef struct
{
    FileInfo_t *files; // 文件信息数组
    uint32_t count;    // 文件数量
    uint32_t capacity; // 数组容量
} FileList_t;

/**
 * @brief  Get file list by extension
 * @param  dir_path: Directory path (e.g., "0:/data")
 * @param  extension: File extension (e.g., ".txt")
 * @param  file_list: File list structure to fill
 * @param  max_count: Maximum number of files to get
 * @return int8_t: 0 success, negative error code
 */
int8_t get_file_list_by_extension(const char *dir_path, const char *extension, FileList_t *file_list, uint32_t max_count);

/**
 * @brief  Free file list
 * @param  file_list: File list structure to free
 * @return void
 */
void free_file_list(FileList_t *file_list);

/**
 * @brief  Format date and time from FATFS format
 * @param  date: FATFS date format
 * @param  time: FATFS time format
 * @param  buf: Buffer to store formatted string
 * @param  buf_size: Buffer size
 * @return char*: Formatted date and time string
 */
char *format_date_time(DWORD date, DWORD time, char *buf, uint32_t buf_size);

int32_t f_write_dma_safe(FIL *fil, const uint8_t *src, uint32_t len, UINT *bw_total);

#endif /* __FILE_UTILS_H__ */
