#include "app_common.h"
#include "app_processor.h"
#include "app_daemon.h"
#include "./BSP/TIMER/gtim.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "./LIBS/lib_circular_buffer/CircularBuffer.h"

/* 配置参数 */
#define ADC_SAMPLE_RATE     10000     // 50KSPS
#define ADC_BUFFER_SIZE     1024      // ADC数据缓冲区大小
#define ADC_DATA_QUEUE_SIZE 256       // 数据队列大小
#define SPI_NUM             3         // SPI总线数量
#define SPI_CH_ADC_MAX      1         // 单SPI总线上ADC个数

/* ADC数据结构 */
typedef struct {
    uint16_t data[SPI_NUM][SPI_CH_ADC_MAX];  // 3个SPI，每个1个ADC数据
    uint32_t timestamp;   // 时间戳
} ADC_Data_t;

/* 全局变量 */
static QueueHandle_t adc_data_queue = NULL;
static SemaphoreHandle_t adc_semaphore = NULL;
static TaskHandle_t processor_task_handle = NULL;
static volatile uint32_t adc_sample_count = 0;

CircularBuffer* g_cb_adc;

static void process_adc_data(ADC_Data_t *data);

/**
 * @brief  ProcessorTask任务函数
 * @param  pvParameters: 任务参数
 */
void ProcessorTask(void *pvParameters)
{
    ADC_Data_t adc_data;
    uint32_t tim_freq, arr, psc;
    
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
    
    // 创建容量为1024的缓冲区
    g_cb_adc = cb_init(3*2*512);
    if (!g_cb_adc) {
        printf("Failed to create circular buffer\n");
        return;
    }
    
    // 计算定时器参数（50KSPS）
    tim_freq = 300000000;  // TIM时钟频率
    arr = 100 - 1;        // 自动重装载值
    psc = (tim_freq / (ADC_SAMPLE_RATE * (arr + 1))) - 1;

    // 启动ADC采集定时器
    gtim_timx_start(arr, psc);
    
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
//                    printf("已处理采样点数: %lu\n", adc_sample_count);
//                }
            }
        }
        
        // 检查栈空间
        UBaseType_t stack_remaining = uxTaskGetStackHighWaterMark(NULL);
        if(stack_remaining < 50) {
            printf("警告: ProcessorTask栈空间不足! 剩余: %lu\n", stack_remaining);
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
    // 这里可以添加数据处理逻辑
    int ret = 0;
    ret = cb_write(g_cb_adc, (const char*)data, sizeof(data->data));
    if (ret != sizeof(data->data)){
        printf("Failed to write adc datas to circular buffer.");
        return;
    }
    // 可以在这里添加更多处理逻辑
}

/**
 * @brief  从ISR发送ADC数据到队列
 * @param  spi_num: SPI编号
 * @param  adc_num: ADC编号
 * @param  data: ADC数据
 * @retval 发送结果
 */
BaseType_t adc_send_data_from_isr(uint8_t spi_num, uint8_t adc_num, uint16_t data)
{
    static ADC_Data_t current_sample = {0};
    static uint8_t data_ready = 0;
    static uint32_t sample_timestamp = 0;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t result = pdFALSE;
    
    // 存储当前采样点的数据
    if(spi_num < SPI_NUM && adc_num < SPI_CH_ADC_MAX) {
        current_sample.data[spi_num][adc_num] = data;
    }
    
    // 如果是最后一个ADC的最后一个数据，完成一次采样
    if(spi_num == (SPI_NUM-1) && adc_num == (SPI_CH_ADC_MAX-1)) {
        current_sample.timestamp = sample_timestamp++;
        data_ready = 1;
    }
    
    // 数据准备好时发送到队列
    if(data_ready && adc_data_queue != NULL) {
        // 尝试发送到队列
        result = xQueueSendToBackFromISR(adc_data_queue, &current_sample, &xHigherPriorityTaskWoken);
        
        if(result == pdTRUE) {
            // 发送成功，通知任务处理
            xSemaphoreGiveFromISR(adc_semaphore, &xHigherPriorityTaskWoken);
        } else {
            // 队列已满，丢弃数据（可以增加错误计数）
        }
        
        data_ready = 0;
    }
    
    // 如果需要上下文切换
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    
    return result;
}
