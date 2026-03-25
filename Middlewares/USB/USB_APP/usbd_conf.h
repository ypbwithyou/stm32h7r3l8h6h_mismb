/**
 ****************************************************************************************************
 * @file        usbd_conf.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       USB设备配置文件
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

#ifndef __USBD_CONF_H
#define __USBD_CONF_H

#include "./SYSTEM/sys/sys.h"
#include <string.h>
#include <stdio.h>

#define USBD_MAX_NUM_INTERFACES     1U
#define USBD_MAX_NUM_CONFIGURATION  1U
#define USBD_MAX_STR_DESC_SIZ       512U
#define USBD_DEBUG_LEVEL            0U
#define USBD_LPM_ENABLED            0U
#define USBD_SELF_POWERED           1U

/* CDC类配置 */
#define USBD_CDC_INTERVAL           2000U

/* 设备ID定义 */
#define DEVICE_HS                   0

/* 内存管理相关宏定义 */
#define USBD_malloc                 (void *)USBD_static_malloc
#define USBD_free                   USBD_static_free
#define USBD_memset                 memset

/* 调试相关宏定义 */
#if (USBD_DEBUG_LEVEL > 0U)
#define USBD_UsrLog(...)    do {                        \
                                printf(__VA_ARGS__);    \
                                printf("\n");           \
                            } while (0)
#else
#define USBD_UsrLog(...)    do {} while (0)
#endif
#if (USBD_DEBUG_LEVEL > 1U)
#define USBD_ErrLog(...)    do {                        \
                                printf("ERROR: ") ;     \
                                printf(__VA_ARGS__);    \
                                printf("\n"); \
                            } while (0)
#else
#define USBD_ErrLog(...)    do {} while (0)
#endif
#if (USBD_DEBUG_LEVEL > 2U)
#define USBD_DbgLog(...)    do {                        \
                                printf("DEBUG : ") ;    \
                                printf(__VA_ARGS__);    \
                                printf("\n");           \
                            } while (0)
#else
#define USBD_DbgLog(...)    do {} while (0)
#endif

/* 导出相关变量 */
extern uint8_t g_usb_conn_state;

/* 函数声明 */
void *USBD_static_malloc(uint32_t size);
void USBD_static_free(void *p);

#endif
