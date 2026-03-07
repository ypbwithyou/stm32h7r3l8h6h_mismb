#include "collector_processor.h"
#include "./BSP/TIMER/gtim.h"

CircularBuffer* g_cb_adc;

CircularBuffer* collect_cb_init(uint32_t cb_len)
{
    return cb_init(cb_len);
}

/**
 * @brief  ADC采集数据处理函数
 * @param  data: ADC采集数据+时间戳
 */
void process_adc_data(ADC_Data_t *data)
{
    // 添加采集数据到采集数据缓存buffer
    int ret = 0;
    ret = cb_write(g_cb_adc, (const char*)data, sizeof(data->data));
    if (ret != sizeof(data->data)){
        return;
    }
}

///**
// * @brief  发送ISR获取的ADC采集数据到队列
// * @param  spi_num: SPI号
// * @param  adc_num: ADC通道号
// * @param  data: ADC采集数据
// * @retval 队列追加结果
// */
//BaseType_t adc_send_data_from_isr(uint8_t spi_num, uint8_t adc_num, uint16_t data)
//{
//    static ADC_Data_t current_sample = {0};
//    static uint8_t data_ready = 0;
//    static uint32_t sample_timestamp = 0;
//    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
//    BaseType_t result = pdFALSE;
//    
//    // 存储当前采样点的数据
//    if(spi_num < SPI_NUM && adc_num < SPI_CH_ADC_MAX) {
//        current_sample.data[spi_num][adc_num] = data;
//    }
//    
//    // 如果是最后一个ADC的最后一个数据，完成一次采样
//    if(spi_num == (SPI_NUM-1) && adc_num == (SPI_CH_ADC_MAX-1)) {
//        current_sample.timestamp = sample_timestamp++;
//        data_ready = 1;
//    }
//    
//    // 数据准备好时发送到队列
//    if(data_ready && adc_data_queue != NULL) {
//        // 尝试发送到队列
//        result = xQueueSendToBackFromISR(adc_data_queue, &current_sample, &xHigherPriorityTaskWoken);
//        
//        if(result == pdTRUE) {
//            // 发送成功，通知任务处理
//            xSemaphoreGiveFromISR(adc_semaphore, &xHigherPriorityTaskWoken);
//        } else {
//            // 队列已满，丢弃数据（可以增加错误计数）
//        }
//        
//        data_ready = 0;
//    }
//    
//    // 如果需要上下文切换
//    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
//    
//    return result;
//}

/**
 * @brief  采样率配置
 * @param  采样率
 * @retval 无
 */
void CfgAdcSampleRate(uint32_t sample_rate)
{
    uint32_t tim_freq, arr, psc;

    tim_freq = 300000000;       // 定时器时钟
    arr = 100 - 1;              // 
    psc = (tim_freq / (sample_rate * (arr + 1))) - 1;
    
    gtim_timx_cfg(arr, psc);
}

/**
 * @brief  ADC采集控制函数
 * @param  采集运行状态
 * @retval 无
 */
void AdcCollectorContrl(uint8_t run_status)
{
    if (run_status) {
        gtim_timx_start();
    } else {
        gtim_timx_stop();
    }
}

/**
 * @brief  采集数据缓存清除
 * @param  无
 * @retval 无
 */
void AdcCbClear(void)
{
    cb_clear(g_cb_adc);
}
