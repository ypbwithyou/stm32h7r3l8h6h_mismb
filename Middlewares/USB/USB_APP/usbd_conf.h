/**
 ****************************************************************************************************
 * @file        usbd_conf.h
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       USB�豸�����ļ�
 * @license     Copyright (c) 2020-2032, �������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 * 
 * ʵ��ƽ̨:����ԭ�� H7R3������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 * 
 ****************************************************************************************************
 */

#ifndef __USBD_CONF_H
#define __USBD_CONF_H

#include "./SYSTEM/sys/sys.h"
#include <string.h>
#include <stdio.h>

#define USBD_MAX_NUM_INTERFACES     3U     /* CDC(2) + MSC(1) */
#define USBD_MAX_NUM_CONFIGURATION  1U
#define USBD_MAX_STR_DESC_SIZ       512U
#define USBD_DEBUG_LEVEL            0U
#define USBD_LPM_ENABLED            0U
#define USBD_SELF_POWERED           1U

/* Composite device: CDC virtual COM port + MSC read-only U-disk */
#define USE_USBD_COMPOSITE          1
#define USBD_CMPSIT_ACTIVATE_CDC    1
#define USBD_CMPSIT_ACTIVATE_MSC    1
#define USBD_COMPOSITE_USE_IAD      1
#define USBD_CMPST_MAX_CONFDESC_SZ  300U

/* CDC������ */
#define USBD_CDC_INTERVAL           2000U

/* �豸ID���� */
#define DEVICE_HS                   0

/* �ڴ������غ궨�� */
#define USBD_malloc                 (void *)USBD_static_malloc
#define USBD_free                   USBD_static_free
#define USBD_memset                 memset

/* ������غ궨�� */
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

/* ������ر��� */
extern uint8_t g_usb_conn_state;

/* �������� */
void *USBD_static_malloc(uint32_t size);
void USBD_static_free(void *p);

#endif
