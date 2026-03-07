#include "app_common.h"
#include "app_offline.h"
#include "offline_processor.h"

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#include "./BSP/PCA9554A/pca9554a.h"

/* 全局变量 */

uint8_t GetDeviceWorkMode(void);
uint8_t GetDeviceEventStatus(void);

/**
 * @brief       OfflineTask - 事件驱动版本
 * @param       pvParameters : 传入参数
 * @retval      无
 */
void OfflineTask(void *pvParameters)
{
    UBaseType_t uxHighWaterMark;
    static uint8_t work_mode = WORKMODE_ONLINE;
    
    uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    
    /* 等待RS485硬件初始化完成 */
    vTaskDelay(pdMS_TO_TICKS(100));
    
    while(1) {
        work_mode = GetDeviceWorkMode();
        offline_processor(work_mode);
        
        /* 监控栈使用情况 */
        uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
        
        vTaskDelay(1);
    }
}

uint8_t GetDeviceWorkMode(void)
{
    return (START_STATUS == 0)?WORKMODE_OFFLINE:WORKMODE_ONLINE;
}

uint8_t GetDeviceEventStatus(void)
{
    return (EVENT_STATUS == 0)?1:0;
}