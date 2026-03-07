/**
 ****************************************************************************************************
 * @file        hyperram.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       HyperRAM驱动代码
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

#ifndef __HYPERRAM_H
#define __HYPERRAM_H

#include "./SYSTEM/sys/sys.h"

/* HyperRAM相关定义 */
#define HYPERRAM_BASE_ADDR XSPI2_BASE

/* 函数声明 */
uint8_t hyperram_init(void);        /* 初始化HyperRAM */
uint32_t hyperram_get_size(void);   /* 获取HyperRAM容量 */

#endif
