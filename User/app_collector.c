#include "app_common.h"
#include "app_collector.h"
#include "app_daemon.h"
#include "./BSP/TIMER/gtim.h"
#include "collector_processor.h"


/* 全局变量 */
QueueHandle_t adc_data_queue = NULL;
SemaphoreHandle_t adc_semaphore = NULL;
TaskHandle_t processor_task_handle = NULL;
static volatile uint32_t adc_sample_count = 0;

CircularBuffer* g_cb_adc;


/**
 * @brief  ProcessorTask任务函数
 * @param  pvParameters: 任务参数
 */
void CollectorTask(void *pvParameters)
{
    int8_t ret = RET_OK;
    ADC_Data_t adc_data;
    
    printf("ProcessorTask启动...\r\n");
    
    // 创建数据队列
    adc_data_queue = xQueueCreate(ADC_DATA_QUEUE_SIZE, sizeof(ADC_Data_t));
    if(adc_data_queue == NULL) {
        printf("ADC数据队列创建失败!\r\n");
        vTaskDelete(NULL);
        return;
    }
    
    // 创建二进制信号量
    adc_semaphore = xSemaphoreCreateBinary();
    if(adc_semaphore == NULL) {
        printf("ADC信号量创建失败!\r\n");
        vQueueDelete(adc_data_queue);
        vTaskDelete(NULL);
        return;
    }
    
    // 保存任务句柄
    processor_task_handle = xTaskGetCurrentTaskHandle();

    // 初始化采集数据缓存Buffer
    g_cb_adc = collect_cb_init(SPI_NUM*ADC_DATA_LEN*SPI_CH_ADC_MAX*BLOCK_LEN);
    if (!g_cb_adc) {
        printf("collect_cb_init err.\n");
        return;
    }

    while(1) {
        // 等待ADC数据
//        if(xSemaphoreTake(adc_semaphore, portMAX_DELAY) == pdTRUE) {
        if(xSemaphoreTake(adc_semaphore, 10) == pdTRUE) {
            // 处理队列中的所有ADC数据
            while(xQueueReceive(adc_data_queue, &adc_data, 0) == pdTRUE) {
                // 处理ADC数据（例如：存储、发送等）
                process_adc_data(&adc_data);
                adc_sample_count++;
            }
        }
        
        // 检查栈空间
        UBaseType_t stack_remaining = uxTaskGetStackHighWaterMark(NULL);
        if(stack_remaining < 50) {
            printf("警告: ProcessorTask栈空间不足! 剩余: %lu\n", stack_remaining);
        }
        
        /* 小延时避免任务过于频繁切换 */
        vTaskDelay(1);
    }
}
