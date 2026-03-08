#include "app_common.h"
#include "app_processor.h"
#include "app_daemon.h"
#include "./BSP/TIMER/gtim.h"
#include "./BSP/DMA_LIST/dma_list.h"  // 添加DMA配置头文件
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "./LIBS/lib_circular_buffer/CircularBuffer.h"

/*
 * DMA SPI动态配置使用示例：
 *
 * 1. 初始化阶段（在采集开始前）：
 *    dma_config_spi_channels(8, 8, 8);  // 每路SPI 8个ADC，总共24通道
 *
 * 2. 运行时动态切换：
 *    gtim_timx_stop();                   // 停止采集
 *    dma_config_spi_channels(4, 4, 4);  // 切换到12通道模式
 *    gtim_timx_start();                  // 重新开始采集
 *
 * 3. 支持的配置范围：
 *    - 每路SPI：1-8个ADC
 *    - 总通道数：1-24
 *    - 自动调整DMA传输大小和数据解析
 */

/* 配置参数 */
#define ADC_SAMPLE_RATE     10000     // 10KSPS采样率
#define ADC_BUFFER_SIZE     1024      // ADC数据缓冲区大小
#define ADC_DATA_QUEUE_SIZE 256       // 数据队列大小
#define SPI_NUM             3         // SPI总线数量
#define SPI_CH_ADC_MAX      8         // 单SPI总线上最大ADC个数（支持动态配置1-8）

/* ADC数据结构 - 动态通道数量 */
typedef struct {
    uint16_t data[24];     // 最大24个通道数据（3个SPI × 8个ADC）
    uint32_t timestamp;    // 时间戳
    uint8_t total_channels; // 实际通道数量
} ADC_Data_t;

/* 全局变量 */
static QueueHandle_t adc_data_queue = NULL;
static SemaphoreHandle_t adc_semaphore = NULL;
static TaskHandle_t processor_task_handle = NULL;
static volatile uint32_t adc_sample_count = 0;

CircularBuffer* g_cb_adc;

static void process_adc_data(ADC_Data_t *data);

/**
 * @brief  配置ADC采集参数
 * @param  spi1_channels: SPI1的ADC数量 (1-8)
 * @param  spi2_channels: SPI2的ADC数量 (1-8)  
 * @param  spi3_channels: SPI4的ADC数量 (1-8)
 * @retval 配置结果
 */
int8_t adc_config_channels(uint8_t spi1_channels, uint8_t spi2_channels, uint8_t spi3_channels)
{
    uint32_t total_channels = spi1_channels + spi2_channels + spi3_channels;
    
    if(total_channels > 24 || total_channels == 0) {
        usb_printf("Invalid channel configuration: total channels must be between 1-24\r\n");
        return -1;
    }
    
    // 停止当前采集
    gtim_timx_stop();
    
    // 重新配置DMA
    dma_config_spi_channels(spi1_channels, spi2_channels, spi3_channels);
    
    // 销毁旧缓冲区
    if(g_cb_adc) {
        cb_destroy(g_cb_adc);
    }
    
    // 创建新缓冲区（根据实际通道数调整大小）
    uint32_t buffer_size = total_channels * 2 * 512;  // 通道数 × 2字节 × 512采样点
    g_cb_adc = cb_init(buffer_size);
    if (!g_cb_adc) {
        usb_printf("Failed to create circular buffer for %lu channels\r\n", total_channels);
        return -1;
    }
    
    usb_printf("ADC configuration updated: SPI1=%d, SPI2=%d, SPI4=%d, total channels=%lu\r\n", 
           spi1_channels, spi2_channels, spi3_channels, total_channels);
    
    // 重新启动采集
    gtim_timx_start();
    
    return 0;
}

/**
 * @brief  ProcessorTask任务函数
 * @param  pvParameters: 任务参数
 */
void ProcessorTask(void *pvParameters)
{
    ADC_Data_t adc_data;
    uint32_t tim_freq, arr, psc;
    
    usb_printf("ProcessorTask started...\r\n");
    
    // 创建数据队列
    adc_data_queue = xQueueCreate(ADC_DATA_QUEUE_SIZE, sizeof(ADC_Data_t));
    if(adc_data_queue == NULL) {
        usb_printf("ADC data queue creation failed!\r\n");
        vTaskDelete(NULL);
        return;
    }
    
    // 创建二进制信号量
    adc_semaphore = xSemaphoreCreateBinary();
    if(adc_semaphore == NULL) {
        usb_printf("ADC semaphore creation failed!\r\n");
        vQueueDelete(adc_data_queue);
        vTaskDelete(NULL);
        return;
    }
    
    // 保存任务句柄
    processor_task_handle = xTaskGetCurrentTaskHandle();
    
    // 初始化DMA数据缓冲区
    dma_list_data_init();
    
    // 配置SPI通道数量 - 示例：每路SPI 8个ADC，总共24通道
    // 可以根据需要动态调整：dma_config_spi_channels(4, 4, 4); // 12通道
    // 或 dma_config_spi_channels(1, 1, 1); // 3通道
    dma_config_spi_channels(8, 8, 8);
    usb_printf("SPI DMA configuration completed: 24-channel mode\r\n");
    
    // 创建容量为1024的缓冲区（根据实际通道数调整）
    g_cb_adc = cb_init(24 * 2 * 512);  // 24通道 × 2字节 × 512采样点
    if (!g_cb_adc) {
        usb_printf("Failed to create circular buffer\n");
        return;
    }
    
    // 计算定时器参数（10KSPS）
    tim_freq = 300000000;  // TIM时钟频率
    arr = 100 - 1;        // 自动重装载值
    psc = (tim_freq / (ADC_SAMPLE_RATE * (arr + 1))) - 1;

    // 启动ADC采集定时器
    gtim_timx_start(arr, psc);
    
    usb_printf("ADC acquisition started: 10KSPS, 24 channels\r\n");
    
    while(1) {
        // 等待ADC数据
        if(xSemaphoreTake(adc_semaphore, portMAX_DELAY) == pdTRUE) {
            // 处理队列中的所有ADC数据
            while(xQueueReceive(adc_data_queue, &adc_data, 0) == pdTRUE) {
                // 处理ADC数据（例如：存储、发送等）
                process_adc_data(&adc_data);
                adc_sample_count++;
                
                // 每1000个采样点打印一次
//                if(adc_sample_count % 1000 == 0) {
//                    usb_printf("Processed sample count: %lu\n", adc_sample_count);
//                }
            }
        }
        
        // 检查栈空间
        UBaseType_t stack_remaining = uxTaskGetStackHighWaterMark(NULL);
        if(stack_remaining < 50) {
            usb_printf("Warning: ProcessorTask stack space insufficient! Remaining: %lu\n", stack_remaining);
        }
        
        // 让出CPU给其他任务
        taskYIELD();
    }
}

/**
 * @brief  处理ADC数据（示例函数）
 * @param  data: ADC数据指针
 */
static void process_adc_data(ADC_Data_t *data)
{
    // 将数据写入循环缓冲区
    int ret = cb_write(g_cb_adc, (const char*)data->data, data->total_channels * sizeof(uint16_t));
    if (ret != (int)(data->total_channels * sizeof(uint16_t))){
        usb_printf("Failed to write ADC data to circular buffer.\r\n");
        return;
    }

    // 可以在这里添加更多处理逻辑，比如：
    // - 数据校验
    // - 滤波处理
    // - 存储到文件
    // - 发送到USB等
}

/**
 * @brief  从ISR发送ADC数据到队列
 * @param  data: ADC数据指针
 * @param  total_channels: 总通道数
 * @retval 发送结果
 */
BaseType_t adc_send_data_from_isr(uint16_t *data, uint8_t total_channels)
{
    static ADC_Data_t current_sample = {0};
    static uint32_t sample_timestamp = 0;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t result = pdFALSE;

    // 复制数据
    memcpy(current_sample.data, data, total_channels * sizeof(uint16_t));
    current_sample.timestamp = sample_timestamp++;
    current_sample.total_channels = total_channels;

    // 发送到队列
    if(adc_data_queue != NULL) {
        result = xQueueSendToBackFromISR(adc_data_queue, &current_sample, &xHigherPriorityTaskWoken);

        if(result == pdTRUE) {
            // 发送成功，通知任务处理
            xSemaphoreGiveFromISR(adc_semaphore, &xHigherPriorityTaskWoken);
        } else {
            // 队列已满，丢弃数据（可以增加错误计数）
            usb_printf("ADC queue full, data dropped\r\n");
        }
    }

    // 如果需要上下文切换
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

    return result;
}
