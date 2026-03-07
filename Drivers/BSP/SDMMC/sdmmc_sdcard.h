/**
 ****************************************************************************************************
 * @file        sdmmc_sdcard.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       SD卡驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 * 
 * 实验平台:正点原子 H7R3开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 * 
 ****************************************************************************************************
 */

#ifndef __SDMMC_SDCARD_H
#define __SDMMC_SDCARD_H

#include "./SYSTEM/sys/sys.h"

/* SD相关定义 */
#define SD_SDMMCX                       SDMMC1
#define SD_SDMMCX_CLK_ENABLE()          do { __HAL_RCC_SDMMC1_CLK_ENABLE(); } while (0)
#define SD_SDMMCX_CLK_DISABLE()         do { __HAL_RCC_SDMMC1_CLK_DISABLE(); } while (0)
#define SD_SDMMCX_CK_GPIO_PORT          GPIOC
#define SD_SDMMCX_CK_GPIO_PIN           GPIO_PIN_12
#define SD_SDMMCX_CK_GPIO_AF            GPIO_AF11_SDMMC1
#define SD_SDMMCX_CK_GPIO_CLK_ENABLE()  do { __HAL_RCC_GPIOC_CLK_ENABLE(); } while (0)
#define SD_SDMMCX_CMD_GPIO_PORT         GPIOD
#define SD_SDMMCX_CMD_GPIO_PIN          GPIO_PIN_2
#define SD_SDMMCX_CMD_GPIO_AF           GPIO_AF11_SDMMC1
#define SD_SDMMCX_CMD_GPIO_CLK_ENABLE() do { __HAL_RCC_GPIOD_CLK_ENABLE(); } while (0)
#define SD_SDMMCX_D0_GPIO_PORT          GPIOC
#define SD_SDMMCX_D0_GPIO_PIN           GPIO_PIN_8
#define SD_SDMMCX_D0_GPIO_AF            GPIO_AF11_SDMMC1
#define SD_SDMMCX_D0_GPIO_CLK_ENABLE()  do { __HAL_RCC_GPIOC_CLK_ENABLE(); } while (0)
#define SD_SDMMCX_D1_GPIO_PORT          GPIOC
#define SD_SDMMCX_D1_GPIO_PIN           GPIO_PIN_9
#define SD_SDMMCX_D1_GPIO_AF            GPIO_AF11_SDMMC1
#define SD_SDMMCX_D1_GPIO_CLK_ENABLE()  do { __HAL_RCC_GPIOC_CLK_ENABLE(); } while (0)
#define SD_SDMMCX_D2_GPIO_PORT          GPIOC
#define SD_SDMMCX_D2_GPIO_PIN           GPIO_PIN_10
#define SD_SDMMCX_D2_GPIO_AF            GPIO_AF12_SDMMC1
#define SD_SDMMCX_D2_GPIO_CLK_ENABLE()  do { __HAL_RCC_GPIOC_CLK_ENABLE(); } while (0)
#define SD_SDMMCX_D3_GPIO_PORT          GPIOC
#define SD_SDMMCX_D3_GPIO_PIN           GPIO_PIN_11
#define SD_SDMMCX_D3_GPIO_AF            GPIO_AF11_SDMMC1
#define SD_SDMMCX_D3_GPIO_CLK_ENABLE()  do { __HAL_RCC_GPIOC_CLK_ENABLE(); } while (0)

/* SD卡操作超时时间定义 */
#define SD_TIMEOUT                      ((uint32_t)0x00100000)

/* 导出SD句柄 */
extern SD_HandleTypeDef g_sd_handle;
extern HAL_SD_CardInfoTypeDef g_sd_card_info_struct;

/* 函数声明 */
uint8_t sd_init(void);                                                  /* 初始化SD卡 */
uint8_t sd_read_disk(uint8_t *buf, uint32_t address, uint32_t count);   /* 读SD卡指定数量块的数据 */
uint8_t sd_write_disk(uint8_t *buf, uint32_t address, uint32_t count);  /* 写SD卡指定数量块的数据 */

#endif
