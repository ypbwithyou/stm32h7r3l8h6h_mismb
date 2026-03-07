/**
 ****************************************************************************************************
 * @file        sd_test.h
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

#ifndef __SD_TEST_H__
#define __SD_TEST_H__

#include "stdint.h"

#ifdef __cplusplus
 extern "C" {
#endif

/**
 * @brief 初始化FATFS文件系统
 * @return int32_t: 0成功，负数错误码
 */
int32_t FATFS_Init(void);

/**
 * @brief 格式化SD卡
 * @return int32_t: 0成功，负数错误码
 */
int32_t FATFS_Format(void);

/**
 * @brief 写入测试数据
 * @return int32_t: 0成功，负数错误码
 */
int32_t SD_Test_Write(void);

/**
 * @brief 读取测试数据
 * @return int32_t: 0成功，负数错误码
 */
int32_t SD_Test_Read(void);

/**
 * @brief 验证测试数据
 * @return int32_t: 0成功，负数错误码
 */
int32_t SD_Test_Verify(void);

/**
 * @brief 清理测试文件
 * @return int32_t: 0成功，负数错误码
 */
int32_t SD_Test_Cleanup(void);

/**
 * @brief 启动SD卡测试
 * @return int32_t: 0成功，负数错误码
 */
int32_t SD_Test_Start(void);

/**
 * @brief SD卡测试函数
 */
void SD_Test_Run(void);

#ifdef __cplusplus
}
#endif

#endif /* __SD_TEST_H__ */
