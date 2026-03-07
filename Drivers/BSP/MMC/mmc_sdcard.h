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
 
/* 超时时间(ms) */
#define SD_TIMEOUT 500
#define SECTOR_SIZE 512

/* 导出SD句柄 */
extern MMC_HandleTypeDef g_sd_handle;
extern HAL_MMC_CardInfoTypeDef g_sd_card_info_struct;

/* 函数声明 */
uint8_t mmc_init(void);                                                  /* 初始化SD卡 */
uint8_t sd_read_disk(uint8_t *buf, uint32_t address, uint32_t count);   /* 读SD卡指定数量块的数据 */
uint8_t sd_write_disk(uint8_t *buf, uint32_t address, uint32_t count);  /* 写SD卡指定数量块的数据 */

int8_t sd_card_test_rw(void);
void sd_get_status(void);

#endif
