/**
 ****************************************************************************************************
 * @file        adc.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       ADC驱动代码
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

#include "./BSP/ADC/adc.h"

/* ADC句柄 */
ADC_HandleTypeDef g_adc_handle = {0};

/**
 * @brief   初始化ADC
 * @param   无
 * @retval  无
 */
void adc_init(void)
{
    /* 初始化ADC */
    g_adc_handle.Instance = ADC_ADCX;
    g_adc_handle.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV8;                /* 时钟源和预分频系数 */
    g_adc_handle.Init.Resolution = ADC_RESOLUTION_12B;                      /* 分辨率 */
    g_adc_handle.Init.DataAlign = ADC_DATAALIGN_RIGHT;                      /* 数据对齐 */
    g_adc_handle.Init.ScanConvMode = ADC_SCAN_DISABLE;                      /* 扫描转换模式 */
    g_adc_handle.Init.EOCSelection = ADC_EOC_SINGLE_CONV;                   /* EOC */
    g_adc_handle.Init.LowPowerAutoWait = DISABLE;                           /* 低功耗自动延迟 */
    g_adc_handle.Init.ContinuousConvMode = DISABLE;                         /* 连续转换模式 */
    g_adc_handle.Init.NbrOfConversion = 1;                                  /* 连续转换模式下的转换数量 */
    g_adc_handle.Init.DiscontinuousConvMode = DISABLE;                      /* 非连续转换模式 */
    g_adc_handle.Init.NbrOfDiscConversion = 1;                              /* 非连续转换模式下的转换数量 */
    g_adc_handle.Init.ExternalTrigConv = ADC_SOFTWARE_START;                /* 规则组转换外部触发源 */
    g_adc_handle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE; /* 规则组转换外部触发源的有效沿 */
    g_adc_handle.Init.SamplingMode = ADC_SAMPLING_MODE_NORMAL;              /* 采样模式 */
    g_adc_handle.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DR;     /* 管理转换数据 */
    g_adc_handle.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;                   /* 溢出行为 */
    g_adc_handle.Init.OversamplingMode = DISABLE;                           /* 过采样模式 */
    HAL_ADC_Init(&g_adc_handle);
    
    /* ADC自动自校准 */
    HAL_ADCEx_Calibration_Start(&g_adc_handle, ADC_SINGLE_ENDED);
}

/**
 * @brief   获取ADC结果
 * @param   channel: ADC通道
 * @retval  ADC结果
 */
uint32_t adc_get_result(uint32_t channel)
{
    ADC_ChannelConfTypeDef adc_channel_conf_struct = {0};
    uint32_t result;
    
    /* 配置ADC通道 */
    adc_channel_conf_struct.Channel = channel;                          /* 通道 */
    adc_channel_conf_struct.Rank = ADC_REGULAR_RANK_1;                  /* Rank编号 */
    adc_channel_conf_struct.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;  /* 采样时间 */
    adc_channel_conf_struct.SingleDiff = ADC_SINGLE_ENDED;              /* 采样输入模式 */
    adc_channel_conf_struct.OffsetNumber = ADC_OFFSET_NONE;             /* 偏移号 */
    adc_channel_conf_struct.Offset = 0;                                 /* 偏移量 */
    adc_channel_conf_struct.OffsetSign = ADC_OFFSET_SIGN_NEGATIVE;      /* 偏移应用 */
    HAL_ADC_ConfigChannel(&g_adc_handle, &adc_channel_conf_struct);
    
    /* 开启ADC转换 */
    HAL_ADC_Start(&g_adc_handle);
    
    /* 等待ADC转换结束 */
    HAL_ADC_PollForConversion(&g_adc_handle, HAL_MAX_DELAY);
    
    /* 获取ADC转换结果 */
    result = HAL_ADC_GetValue(&g_adc_handle);
    
    /* 停止ADC转换 */
    HAL_ADC_Stop(&g_adc_handle);
    
    return result;
}

/**
 * @brief   均值滤波获取ADC结果
 * @param   channel: ADC通道
 * @param   times: 均值滤波的原始数据个数
 * @retval  ADC结果
 */
uint32_t adc_get_result_average(uint32_t channel, uint8_t times)
{
    uint32_t sum_result = 0;
    uint8_t index;
    uint32_t result;
    
    for (index=0; index<times; index++)
    {
        sum_result += adc_get_result(channel);
    }
    
    result = sum_result / times;
    
    return result;
}

/**
 * @brief   HAL库ADC初始化MSP回调函数
 * @param   hadc: ADC句柄指针
 * @retval  无
 */
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
    RCC_PeriphCLKInitTypeDef rcc_periph_clk_init_struct = {0};
    GPIO_InitTypeDef gpio_init_struct = {0};
    
    if (hadc->Instance == ADC_ADCX)
    {
        /* 配置时钟源 */
        rcc_periph_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_ADC;
        rcc_periph_clk_init_struct.AdcClockSelection = RCC_ADCCLKSOURCE_PLL2P;
        HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init_struct);
        
        /* 使能时钟 */
        ADC_ADCX_CLK_ENABLE();
        ADC_ADCX_CHY_GPIO_CLK_ENABLE();
        
        /* 配置ADC采样引脚 */
        gpio_init_struct.Pin = ADC_ADCX_CHY_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_ANALOG;
        gpio_init_struct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(ADC_ADCX_CHY_GPIO_PORT, &gpio_init_struct);
    }
}
