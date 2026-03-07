/**
 ****************************************************************************************************
 * @file        sd_test.c
 * @author      ALIENTEK
 * @version     V1.0
 * @date        2026-02-13
 * @brief       SD卡读写测试程序
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

#include "sd_test.h"
#include "sdmmc.h"
#include "./FATFS/source/ff.h"
#include "stdio.h"
#include "main.h"

#define TEST_FILE_NAME "0:/test_file.txt"
#define TEST_DATA_SIZE 1024

FATFS fs;
FIL file;
UINT bytes_written, bytes_read;
FRESULT res;

uint8_t test_data[TEST_DATA_SIZE] __attribute__((aligned(4)));
uint8_t read_data[TEST_DATA_SIZE] __attribute__((aligned(4)));

/**
 * @brief 初始化FATFS文件系统
 */
int32_t FATFS_Init(void)
{
    // 挂载文件系统
    res = f_mount(&fs, "0:", 1);
    if (res != FR_OK)
    {
        printf("文件系统挂载失败: %d\n", res);
        return -1;
    }
    
    printf("文件系统挂载成功\n");
    return 0;
}

/**
 * @brief 格式化SD卡
 */
int32_t FATFS_Format(void)
{
    // 格式化SD卡
    res = f_mkfs("0:", FM_ANY, 0, NULL, 0);
    if (res != FR_OK)
    {
        printf("SD卡格式化失败: %d\n", res);
        return -1;
    }
    
    printf("SD卡格式化成功\n");
    return 0;
}

/**
 * @brief 写入测试数据
 */
int32_t SD_Test_Write(void)
{
    // 准备测试数据
    for (uint32_t i = 0; i < TEST_DATA_SIZE; i++)
    {
        test_data[i] = i % 256;
    }
    
    // 创建文件
    res = f_open(&file, TEST_FILE_NAME, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK)
    {
        printf("文件创建失败: %d\n", res);
        return -1;
    }
    
    // 写入数据
    res = f_write(&file, test_data, TEST_DATA_SIZE, &bytes_written);
    if (res != FR_OK)
    {
        printf("数据写入失败: %d\n", res);
        f_close(&file);
        return -2;
    }
    
    printf("数据写入成功: %u bytes\n", bytes_written);
    
    // 关闭文件
    f_close(&file);
    return 0;
}

/**
 * @brief 读取测试数据
 */
int32_t SD_Test_Read(void)
{
    // 打开文件
    res = f_open(&file, TEST_FILE_NAME, FA_READ);
    if (res != FR_OK)
    {
        printf("文件打开失败: %d\n", res);
        return -1;
    }
    
    // 读取数据
    res = f_read(&file, read_data, TEST_DATA_SIZE, &bytes_read);
    if (res != FR_OK)
    {
        printf("数据读取失败: %d\n", res);
        f_close(&file);
        return -2;
    }
    
    printf("数据读取成功: %u bytes\n", bytes_read);
    
    // 关闭文件
    f_close(&file);
    return 0;
}

/**
 * @brief 验证测试数据
 */
int32_t SD_Test_Verify(void)
{
    // 验证数据
    for (uint32_t i = 0; i < TEST_DATA_SIZE; i++)
    {
        if (read_data[i] != test_data[i])
        {
            printf("数据验证失败: 位置 %u, 期望 0x%02X, 实际 0x%02X\n", i, test_data[i], read_data[i]);
            return -1;
        }
    }
    
    printf("数据验证成功\n");
    return 0;
}

/**
 * @brief 清理测试文件
 */
int32_t SD_Test_Cleanup(void)
{
    // 删除测试文件
    res = f_unlink(TEST_FILE_NAME);
    if (res != FR_OK)
    {
        printf("测试文件删除失败: %d\n", res);
        return -1;
    }
    
    printf("测试文件删除成功\n");
    return 0;
}

/**
 * @brief SD卡测试函数
 */
void SD_Test_Run(void)
{
    int32_t ret;
    
    printf("SD卡测试开始\n");
    
    // 1. 初始化SDMMC
    ret = SDMMC_Init();
    if (ret != 0)
    {
        printf("SDMMC初始化失败，测试终止\n");
        return;
    }
    
    // 2. 初始化FATFS
    ret = FATFS_Init();
    if (ret != 0)
    {
        // 尝试格式化SD卡
        printf("尝试格式化SD卡...\n");
        ret = FATFS_Format();
        if (ret != 0)
        {
            printf("SD卡格式化失败，测试终止\n");
            return;
        }
        
        // 重新挂载
        ret = FATFS_Init();
        if (ret != 0)
        {
            printf("文件系统挂载失败，测试终止\n");
            return;
        }
    }
    
    // 3. 写入测试数据
    ret = SD_Test_Write();
    if (ret != 0)
    {
        printf("写入测试失败，测试终止\n");
        return;
    }
    
    // 4. 读取测试数据
    ret = SD_Test_Read();
    if (ret != 0)
    {
        printf("读取测试失败，测试终止\n");
        return;
    }
    
    // 5. 验证测试数据
    ret = SD_Test_Verify();
    if (ret != 0)
    {
        printf("数据验证失败，测试终止\n");
        return;
    }
    
    // 6. 清理测试文件
    ret = SD_Test_Cleanup();
    if (ret != 0)
    {
        printf("清理测试文件失败\n");
    }
    
    printf("SD卡测试完成，所有测试通过！\n");
}

/**
 * @brief 启动SD卡测试
 */
int32_t SD_Test_Start(void)
{
    // 直接运行测试函数
    SD_Test_Run();
    return 0;
}
