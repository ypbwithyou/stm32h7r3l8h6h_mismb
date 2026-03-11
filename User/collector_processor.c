#include "collector_processor.h"
#include "./BSP/TIMER/gtim.h"

#include "usbd_cdc_if.h"

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
// void CfgAdcSampleRate(uint32_t sample_rate)
// {
//     uint32_t tim_freq, arr, psc;

//     tim_freq = 300000000;       // 定时器时钟
//     arr = 100 - 1;              // 
//     psc = (tim_freq / (sample_rate * (arr + 1))) - 1;
    
//     gtim_timx_cfg(arr, psc);
// }

void CfgAdcSampleRate(uint32_t sample_rate)
{
    if (sample_rate == 0 || sample_rate > 500000)
    {
        usb_printf("CfgAdcSampleRate: invalid rate %lu (1~500000Hz)", sample_rate);
        return;
    }

    const uint32_t tim_freq = 300000000;

    uint32_t best_arr = 0, best_psc = 0;
    uint32_t best_error = UINT32_MAX;

    /* 
     * 总分频 N = tim_freq / sample_rate
     * 枚举 arr，psc = N/(arr+1) - 1
     * arr 范围：高频时小arr，低频时大arr
     */
    uint32_t N = tim_freq / sample_rate;  // 理想总计数

    /* arr 从小到大枚举，范围 9~65535 */
    for (uint32_t arr = 9; arr <= 65535; arr++)
    {
        uint32_t divisor = arr + 1;

        /* 跳过 psc 为 0 或超出 16bit 的组合 */
        if (N < divisor) break;             // psc 会小于 0

        uint32_t psc = (N / divisor);
        if (psc == 0) continue;
        if (psc > 65536) continue;

        /* 检查 psc 和 psc-1 两个候选（补偿整除误差）*/
        for (uint32_t p = (psc > 1 ? psc - 1 : 1); p <= psc + 1; p++)
        {
            if (p == 0 || p > 65536) continue;

            uint32_t actual_rate = tim_freq / (p * divisor);
            uint32_t error = (actual_rate > sample_rate) ?
                             (actual_rate - sample_rate) :
                             (sample_rate - actual_rate);

            if (error < best_error)
            {
                best_error = error;
                best_arr   = arr;
                best_psc   = p - 1;  // HAL 传入的是 psc-1

                if (error == 0) goto done;  // 完美匹配直接退出
            }
        }
    }

done:
    {
        uint32_t actual = tim_freq / ((best_psc + 1) * (best_arr + 1));
        float error_ppm = (float)best_error / sample_rate * 1e6f;

        usb_printf("Target:%6luHz | Actual:%6luHz | Err:%lu(%.1fppm) | arr:%5lu psc:%5lu",
                   sample_rate, actual, best_error, error_ppm, best_arr, best_psc);

        gtim_timx_cfg((uint16_t)best_arr, (uint16_t)best_psc);
    }
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
