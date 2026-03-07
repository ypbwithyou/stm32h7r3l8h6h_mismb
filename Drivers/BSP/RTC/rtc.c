/**
 ****************************************************************************************************
 * @file        rtc.c
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

#include "./BSP/RTC/rtc.h"
#include "./BSP/LED/led.h"
#include <stdio.h>
#include "./LIBS/lib_dwt/lib_dwt_timestamp.h"

RTC_HandleTypeDef g_rtc_handle = {0};   /* RTC��� */

static uint64_t calculate_timestamp(uint8_t year, uint8_t month, uint8_t date, 
                             uint8_t hour, uint8_t minute, uint8_t second);

/**
 * @brief   ��ȡ�󱸼Ĵ���
 * @param   bkrx: �󱸼Ĵ������
 * @retval  �󱸼Ĵ���ֵ
 */
uint32_t rtc_read_bkr(uint32_t bkrx)
{
    HAL_PWR_EnableBkUpAccess();
    return HAL_RTCEx_BKUPRead(&g_rtc_handle, bkrx);
}

/**
 * @brief   д�󱳼Ĵ���
 * @param   bkrx: �󱸼Ĵ������
 * @param   data: �󱸼Ĵ���ֵ
 * @retval  ��
 */
void rtc_write_bkr(uint32_t bkrx, uint32_t data)
{
    HAL_PWR_EnableBkUpAccess();
    HAL_RTCEx_BKUPWrite(&g_rtc_handle, bkrx, data);
}

/**
 * @brief   ��ʼ��RTC
 * @param   ��
 * @retval  ��ʼ�����
 * @arg     0: ��ʼ���ɹ�
 * @arg     1: ��ʼ��ʧ��
 */
uint8_t rtc_init(void)
{
    uint32_t flag;
    
    /* ����RTC */
    g_rtc_handle.Instance = RTC;
    g_rtc_handle.Init.HourFormat = RTC_HOURFORMAT_24;
    g_rtc_handle.Init.AsynchPrediv = 0x7F;
    g_rtc_handle.Init.SynchPrediv = 0xFF;
    g_rtc_handle.Init.OutPut = RTC_OUTPUT_DISABLE;
    g_rtc_handle.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
    g_rtc_handle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    g_rtc_handle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    
    /* �Ӻ󱸼Ĵ�����ȡRTC��ʼ����־ */
    flag = rtc_read_bkr(0);
    
    /* ��ʼ��RTC */
    if (HAL_RTC_Init(&g_rtc_handle) != HAL_OK)
    {
        return 1;
    }
    
    /* RTC��һ�γ�ʼ�� */
    if ((flag != 0x5050) && (flag != 0x5051))
    {
        /* ����RTCʱ���������Ϣ */
//        rtc_set_time(8, 0, 0, 0);
//        rtc_set_date(26, 3, 2, 1);
        set_rtc_time(26, 3, 2, 8, 0, 0);
    }

    return 0;
}

/**
 * @brief   HAL��RTC��ʼ��MSP����
 * @param   ��
 * @retval  ��
 */
void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc)
{
    RCC_PeriphCLKInitTypeDef rcc_periph_clk_init_struct = {0};
    
    if (hrtc->Instance == RTC)
    {
        /* ʹ��ʱ�� */
        __HAL_RCC_RTCAPB_CLK_ENABLE();
        HAL_PWR_EnableBkUpAccess();
        
        /* ����ʹ��LSE */
        rcc_osc_init_struct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
        rcc_osc_init_struct.LSEState = RCC_LSE_ON;
        rcc_osc_init_struct.PLL1.PLLState = RCC_PLL_NONE;
        rcc_osc_init_struct.PLL2.PLLState = RCC_PLL_NONE;
        rcc_osc_init_struct.PLL3.PLLState = RCC_PLL_NONE;
        if (HAL_RCC_OscConfig(&rcc_osc_init_struct) == HAL_OK)
        {
            rcc_periph_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
            rcc_periph_clk_init_struct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
            HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init_struct);
            
            /* д��RTC��ʼ����־ */
            rtc_write_bkr(0, 0x5050);
            
            __HAL_RCC_RTC_ENABLE();
        }
        else
        {
            /* LSE�����ã���ʹ��LSI */
            rcc_osc_init_struct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
            rcc_osc_init_struct.LSEState = RCC_LSE_OFF;
            rcc_osc_init_struct.PLL1.PLLState = RCC_PLL_NONE;
            rcc_osc_init_struct.PLL2.PLLState = RCC_PLL_NONE;
            rcc_osc_init_struct.PLL3.PLLState = RCC_PLL_NONE;
            HAL_RCC_OscConfig(&rcc_osc_init_struct);
            
            rcc_periph_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
            rcc_periph_clk_init_struct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
            HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init_struct);
            
            /* д��RTC��ʼ����־ */
            rtc_write_bkr(0, 0x5051);
            
            __HAL_RCC_RTC_ENABLE();
        }
    }
}

// 设置RTC日期和时间
void set_rtc_time(uint8_t year, uint8_t month, uint8_t date, uint8_t hour, uint8_t minute, uint8_t second)
{
    rtc_set_date(year, month, date, RTC_WEEKDAY_MONDAY);
    rtc_set_time(hour, minute, second, RTC_HOURFORMAT_24);
}

/**
 * @brief   ����RTCʱ����Ϣ
 * @param   hour: ʱ
 * @param   minute: ��
 * @param   second: ��
 * @param   ampm: ������
 * @arg     0: ����
 * @arg     1: ����
 * @retval  ���ý��
 * @arg     0: ���óɹ�
 * @arg     1: ����ʧ��
 */
uint8_t rtc_set_time(uint8_t hour, uint8_t minute, uint8_t second, uint8_t ampm)
{
    RTC_TimeTypeDef rtc_time_struct = {0};
    
    rtc_time_struct.Hours = hour;
    rtc_time_struct.Minutes = minute;
    rtc_time_struct.Seconds = second;
    rtc_time_struct.TimeFormat = ampm;
    rtc_time_struct.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    rtc_time_struct.StoreOperation = RTC_STOREOPERATION_RESET;
    if (HAL_RTC_SetTime(&g_rtc_handle, &rtc_time_struct, RTC_FORMAT_BIN) != HAL_OK)
    {
        return 1;
    }
    
    return 0;
}

/**
 * @brief   ����RTC������Ϣ
 * @param   year: ��
 * @param   month: ��
 * @param   date: ��
 * @param   week: ����
 * @retval  ���ý��
 * @arg     0: ���óɹ�
 * @arg     1: ����ʧ��
 */
uint8_t rtc_set_date(uint8_t year, uint8_t month, uint8_t date, uint8_t week)
{
    RTC_DateTypeDef rtc_date_struct = {0};
    
    rtc_date_struct.WeekDay = week;
    rtc_date_struct.Month = month;
    rtc_date_struct.Date = date;
    rtc_date_struct.Year = year;
    if (HAL_RTC_SetDate(&g_rtc_handle, &rtc_date_struct, RTC_FORMAT_BIN) != HAL_OK)
    {
        return 1;
    }
    
    return 0;
}

/**
 * @brief   ��ȡRTCʱ����Ϣ
 * @param   hour: ʱ
 * @param   minute: ��
 * @param   second: ��
 * @param   ampm: ������
 * @arg     0: ����
 * @arg     1: ����
 * @retval  ��
 */
void rtc_get_time(uint8_t *hour, uint8_t *minute, uint8_t *second, uint8_t *ampm)
{
    RTC_TimeTypeDef rtc_time_struct = {0};
    
    HAL_RTC_GetTime(&g_rtc_handle, &rtc_time_struct, RTC_FORMAT_BIN);
    
    *hour = rtc_time_struct.Hours;
    *minute = rtc_time_struct.Minutes;
    *second = rtc_time_struct.Seconds;
    *ampm = rtc_time_struct.TimeFormat;
}

/**
 * @brief   ��ȡRTC������Ϣ
 * @param   year: ��
 * @param   month: ��
 * @param   date: ��
 * @param   week: ����
 * @retval  ��
 */
void rtc_get_date(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *week)
{
    RTC_DateTypeDef rtc_date_struct = {0};
    
    HAL_RTC_GetDate(&g_rtc_handle, &rtc_date_struct, RTC_FORMAT_BIN);
    
    *year = rtc_date_struct.Year;
    *month = rtc_date_struct.Month;
    *date = rtc_date_struct.Date;
    *week = rtc_date_struct.WeekDay;
}

/**
 * @brief   ������ת����
 * @param   year: ��
 * @param   month: ��
 * @param   date: ��
 * @retval  ����
 */
uint8_t rtc_get_week(uint16_t year, uint8_t month, uint8_t date)
{
    uint8_t week = 0;
    
    if (month < 3)
    {
        month += 12;
        --year;
    }
    
    week = (date + 1 + 2 * month + 3 * (month + 1) / 5 + year + (year >> 2) - year / 100 + year / 400) % 7;
    
    return week;
}

/**
 * @brief   ����RTC����ʱ����Ϣ
 * @param   week: ����
 * @param   hour: ʱ
 * @param   minute: ��
 * @param   second: ��
 * @retval  ��
 */
void rtc_set_alarm(uint8_t week, uint8_t hour, uint8_t minute, uint8_t second)
{
    RTC_AlarmTypeDef rtc_alarm_struct = {0};
    
    /* ���������ж� */
    rtc_alarm_struct.AlarmTime.Hours = hour;
    rtc_alarm_struct.AlarmTime.Minutes = minute;
    rtc_alarm_struct.AlarmTime.Seconds = second;
    rtc_alarm_struct.AlarmTime.TimeFormat = RTC_HOURFORMAT12_AM;
    rtc_alarm_struct.AlarmTime.SubSeconds = 0;
    rtc_alarm_struct.AlarmMask = RTC_ALARMMASK_NONE;
    rtc_alarm_struct.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_NONE;
    rtc_alarm_struct.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_WEEKDAY;
    rtc_alarm_struct.AlarmDateWeekDay = week;
    rtc_alarm_struct.Alarm = RTC_ALARM_A;
    HAL_RTC_SetAlarm_IT(&g_rtc_handle, &rtc_alarm_struct, RTC_FORMAT_BIN);
    
    HAL_NVIC_SetPriority(RTC_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RTC_IRQn);
}

/**
 * @brief   ����RTC�����Ի����ж�
 * @param   clock: ����ʱ��
 * @param   count: ���Ѽ�����
 * @retval  ��
 */
void rtc_set_wakeup(uint8_t clock, uint8_t count)
{
    HAL_RTCEx_SetWakeUpTimer_IT(&g_rtc_handle, count, clock, 0);
    
    HAL_NVIC_SetPriority(RTC_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(RTC_IRQn);
}

/**
 * @brief   HAL��RTC����A�жϻص�����
 * @param   hrtc: RTC���
 * @retval  ��
 */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
    printf("Alarm A!\r\n");
}

/**
 * @brief   HAL��RTC�����жϻص�����
 * @param   hrtc: RTC���
 * @retval  ��
 */
void HAL_RTCEx_WakeUpTimerEventCallback(RTC_HandleTypeDef *hrtc)
{
    ;
}

/**
 * @brief   RTC�жϷ�����
 * @param   ��
 * @retval  ��
 */
void RTC_IRQHandler(void)
{
    HAL_RTC_AlarmIRQHandler(&g_rtc_handle);
    HAL_RTCEx_WakeUpTimerIRQHandler(&g_rtc_handle);
}

// 获取万年历时间并输出对应的64位秒值
uint64_t rtc_get_timestamp_s(void)
{
    uint64_t timestamp = 0;
    uint8_t year, month, date, hour, minute, second;
    
    // 获取当前时间
    rtc_get_time(&hour, &minute, &second, NULL);
    rtc_get_date(&year, &month, &date, NULL);
    
    // 计算时间戳（秒级）
    timestamp = (uint64_t)year * 31536000 + (uint64_t)(month - 1) * 2629800 + (uint64_t)(date - 1) * 86400 +
                (uint64_t)hour * 3600 + (uint64_t)minute * 60 + (uint64_t)second;
    
    return timestamp;
}
// 获取万年历时间并输出对应的64位毫秒值
uint64_t rtc_get_timestamp_ms(void)
{
    uint64_t timestamp = 0;
    uint8_t year, month, date, hour, minute, second;
    uint32_t ms = HAL_GetTick() % 1000;  // 获取毫秒部分
    
    // 获取当前时间
    rtc_get_time(&hour, &minute, &second, NULL);
    rtc_get_date(&year, &month, &date, NULL);
    
    // 计算秒级时间戳
    timestamp = calculate_timestamp(year, month, date, hour, minute, second);
    
    // 转换为毫秒级
    timestamp = timestamp * 1000 + ms;
    
    return timestamp;
}
// 获取万年历时间并输出对应的64位微秒值
uint64_t rtc_get_timestamp_us(void)
{
    uint64_t timestamp = 0;
    uint8_t year, month, date, hour, minute, second;
    uint32_t us = dwt_get_us() % 1000000;  // 获取微秒部分
    
    // 获取当前时间
    rtc_get_time(&hour, &minute, &second, NULL);
    rtc_get_date(&year, &month, &date, NULL);
    
    // 计算秒级时间戳
    timestamp = calculate_timestamp(year, month, date, hour, minute, second);
    
    // 转换为微秒级
    timestamp = timestamp * 1000000 + us;
    
    return timestamp;
}

static uint64_t calculate_timestamp(uint8_t year, uint8_t month, uint8_t date, 
                             uint8_t hour, uint8_t minute, uint8_t second)
{
    uint64_t timestamp = 0;
    uint16_t full_year = 2000 + year;  // RTC年份从2000开始
    
    // 计算年的秒数（考虑闰年）
    for (uint16_t y = 2000; y < full_year; y++) {
        if ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0)) {
            timestamp += 31622400;  // 闰年秒数
        } else {
            timestamp += 31536000;  // 平年秒数
        }
    }
    
    // 计算月的秒数
    uint8_t days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if ((full_year % 4 == 0 && full_year % 100 != 0) || (full_year % 400 == 0)) {
        days_in_month[2] = 29;  // 闰年2月
    }
    for (uint8_t m = 1; m < month; m++) {
        timestamp += days_in_month[m] * 86400;
    }
    
    // 计算日、时、分、秒的秒数
    timestamp += (date - 1) * 86400 + hour * 3600 + minute * 60 + second;
    
    return timestamp;
}