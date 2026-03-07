/**
 ****************************************************************************************************
 * @file        sdmmc.h
 * @author      ALIENTEK
 * @version     V1.0
 * @date        2026-02-13
 * @brief       SDMMC初始化和配置
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

#ifndef __SDMMC_H__
#define __SDMMC_H__

#include "stdint.h"
//#include "stm32h7xx_hal.h"
#include "./SYSTEM/sys/sys.h"

#ifdef __cplusplus
 extern "C" {
#endif

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


/**
 * @brief 初始化SDMMC
 * @return int32_t: 0成功，负数错误码
 */
int32_t SDMMC_Init(void);

/**
 * @brief 读取SD卡数据
 * @param sector: 起始扇区
 * @param count: 扇区数量
 * @param buffer: 数据缓冲区
 * @return int32_t: 0成功，负数错误码
 */
int32_t SDMMC_ReadBlocks(uint32_t sector, uint32_t count, uint8_t *buffer);

/**
 * @brief 写入SD卡数据
 * @param sector: 起始扇区
 * @param count: 扇区数量
 * @param buffer: 数据缓冲区
 * @return int32_t: 0成功，负数错误码
 */
int32_t SDMMC_WriteBlocks(uint32_t sector, uint32_t count, uint8_t *buffer);

/**
 * @brief 配置SDMMC时钟
 */
void SDMMC_ClockConfig(void);

/**
 * @brief 配置SDMMC GPIO引脚
 */
void SDMMC_GPIOConfig(void);

/* 外部变量 */
extern MMC_HandleTypeDef hmmc;
extern DMA_HandleTypeDef hdma_sdmmc1_rx;
extern DMA_HandleTypeDef hdma_sdmmc1_tx;

extern volatile uint8_t mmc_read_complete;
extern volatile uint8_t mmc_write_complete;
extern volatile uint8_t mmc_error;

#ifdef __cplusplus
}
#endif

#endif /* __SDMMC_H__ */
