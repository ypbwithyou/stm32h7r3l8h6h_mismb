/**
 ****************************************************************************************************
 * @file        usbd_msc_storage.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       USB MSC存储接口(SD卡)
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

#include "usbd_msc_storage.h"
#include "./BSP/MMC/mmc_sdcard.h"

#define STORAGE_LUN_NBR                  1U
#define STORAGE_BLK_NBR                  0x100000U    /* 约2GB的SD卡 */
#define STORAGE_BLK_SIZ                  0x200U       /* 512字节/块 */

extern MMC_HandleTypeDef g_sd_handle;
extern HAL_MMC_CardInfoTypeDef g_sd_card_info_struct;

int8_t STORAGE_Init(uint8_t lun);
int8_t STORAGE_GetCapacity(uint8_t lun, uint32_t *block_num, uint16_t *block_size);
int8_t STORAGE_IsReady(uint8_t lun);
int8_t STORAGE_IsWriteProtected(uint8_t lun);
int8_t STORAGE_Read(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
int8_t STORAGE_Write(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
int8_t STORAGE_GetMaxLun(void);

int8_t STORAGE_Inquirydata[] =  
{
  /* LUN 0 */
  0x00,
  0x80,
  0x02,
  0x02,
  (STANDARD_INQUIRY_DATA_LEN - 5),
  0x00,
  0x00,
  0x00,
  'A', 'L', 'I', 'E', 'N', 'T', 'E', 'K', 
  'S', 'D', ' ', 'C', 'a', 'r', 'd', ' ',
  ' ', ' ', ' ', ' ',
  '1', '.', '0', '0',
};

USBD_StorageTypeDef USBD_MSC_Template_fops =
{
  STORAGE_Init,
  STORAGE_GetCapacity,
  STORAGE_IsReady,
  STORAGE_IsWriteProtected,
  STORAGE_Read,
  STORAGE_Write,
  STORAGE_GetMaxLun,
  STORAGE_Inquirydata,
};

int8_t STORAGE_Init(uint8_t lun)
{
    UNUSED(lun);
    
    if (mmc_init() != 0)
    {
        return -1;
    }
    
    return 0;
}

int8_t STORAGE_GetCapacity(uint8_t lun, uint32_t *block_num, uint16_t *block_size)
{
    UNUSED(lun);
    
    *block_num = g_sd_card_info_struct.LogBlockNbr;
    *block_size = g_sd_card_info_struct.LogBlockSize;
    
    return 0;
}

int8_t STORAGE_IsReady(uint8_t lun)
{
    UNUSED(lun);
    
    if (g_sd_handle.ErrorCode != HAL_MMC_ERROR_NONE)
    {
        return -1;
    }
    
    return 0;
}

int8_t STORAGE_IsWriteProtected(uint8_t lun)
{
    UNUSED(lun);
    return 0;
}

int8_t STORAGE_Read(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
    UNUSED(lun);
    
    if (sd_read_disk(buf, blk_addr, blk_len) != 0)
    {
        return -1;
    }
    
    return 0;
}

int8_t STORAGE_Write(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
    UNUSED(lun);
    
    if (sd_write_disk(buf, blk_addr, blk_len) != 0)
    {
        return -1;
    }
    
    return 0;
}

int8_t STORAGE_GetMaxLun(void)
{
    return (STORAGE_LUN_NBR - 1);
}
