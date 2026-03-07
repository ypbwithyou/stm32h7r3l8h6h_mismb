/**
 ****************************************************************************************************
 * @file        norflash_ex.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       NOR Flash驱动代码
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

#ifndef __NORFLASH_EX_H
#define __NORFLASH_EX_H

#include "./SYSTEM/sys/sys.h"

/* 函数声明 */
uint8_t norflash_ex_init(void);                                                 /* 初始化NOR Flash */
uint8_t norflash_ex_write(uint32_t address, uint8_t *data, uint32_t length);    /* 写NOR Flash */
uint8_t norflash_ex_read(uint32_t address, uint8_t *data, uint32_t length);     /* 读NOR Flash */
uint8_t norflash_ex_erase_sector(uint32_t address);                             /* 扇区擦除NOR Flash */

#endif /* __NORFLASH_EX_H */
