/**
 ****************************************************************************************************
 * @file        rtc.h
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       RTC��������
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

#ifndef __RTC_H
#define __RTC_H

#include "./SYSTEM/sys/sys.h"

/* �������� */
uint32_t rtc_read_bkr(uint32_t bkrx);                                               /* ��ȡ�󱸼Ĵ��� */
void rtc_write_bkr(uint32_t bkrx, uint32_t data);                                   /* д�󱳼Ĵ��� */
uint8_t rtc_init(void);  
void set_rtc_time(uint8_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t minute, uint8_t second);                                                           /* ��ʼ��RTC */
uint8_t rtc_set_time(uint8_t hour, uint8_t minute, uint8_t second, uint8_t ampm);   /* ����RTCʱ����Ϣ */
uint8_t rtc_set_date(uint8_t year, uint8_t month, uint8_t date, uint8_t week);      /* ����RTC������Ϣ */
void rtc_get_time(uint8_t *hour, uint8_t *minute, uint8_t *second, uint8_t *ampm);  /* ��ȡRTCʱ����Ϣ */
void rtc_get_date(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *week);     /* ��ȡRTC������Ϣ */
uint8_t rtc_get_week(uint16_t year, uint8_t month, uint8_t date);                   /* ������ת���� */
void rtc_set_alarm(uint8_t week, uint8_t hour, uint8_t minute, uint8_t second);     /* ����RTC����ʱ����Ϣ */
void rtc_set_wakeup(uint8_t clock, uint8_t count);                                  /* ����RTC�����Ի����ж� */

// 获取万年历时间并输出对应的64位秒值
uint64_t rtc_get_timestamp_s(void);
// 获取万年历时间并输出对应的64位毫秒值
uint64_t rtc_get_timestamp_ms(void);
// 获取万年历时间并输出对应的64位微秒值
uint64_t rtc_get_timestamp_us(void);

#endif
