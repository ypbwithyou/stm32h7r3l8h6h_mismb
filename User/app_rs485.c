#include "app_common.h"
#include "app_rs485.h"
#include "rs485_processor.h"
#include "app_daemon.h"
#include "./BSP/RS485/rs485.h"
#include "./LIBS/lib_link_list/link_list.h"
#include <string.h>

///* 全局链表 */
//LinkList* Rs485ReceiveDataList = NULL;
//LinkList* Rs485SendDataList = NULL;

/**
 * @brief       Rs485Task - 事件驱动版本
 * @param       pvParameters : 传入参数
 * @retval      无
 */
void Rs485Task(void *pvParameters)
{
    UBaseType_t uxHighWaterMark;
    uint8_t rs485buf[RS485_RX_BUF_LEN];
    uint8_t len = 0;
    uint8_t pending_frames = 0;
    
    uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);

//    /* 软件初始化 */
//    if (Rs485ReceiveDataList == NULL) {
//        Rs485ReceiveDataList = LinkListCreate();
//    }
//    if (Rs485SendDataList == NULL) {
//        Rs485SendDataList = LinkListCreate();
//    }
    
    /* 等待RS485硬件初始化完成 */
    vTaskDelay(pdMS_TO_TICKS(100));
    
    while(1) {
        /* 等待信号量，有数据到达时被唤醒 */
//        if (xSemaphoreTake(xRs485RxSemaphore, portMAX_DELAY) == pdTRUE) {
        if (xSemaphoreTake(xRs485RxSemaphore, 10) == pdTRUE) {
            /* 处理所有待处理的帧 */
            pending_frames = rs485_get_pending_frames();
            
            for (uint8_t i = 0; i < pending_frames; i++) {
                /* 从队列中获取数据帧 */
                if (rs485_recv_data(rs485buf, &len) == 0 && len > 0) {
                    /* 解析数据帧 */
//                    rs485_parse_frame(rs485buf, len);
                    
                    /* 可选：将数据存入链表 */
                    // taskENTER_CRITICAL();
                    // LinkListPushBack(Rs485ReceiveDataList, rs485buf, len);
                    // taskEXIT_CRITICAL();
                    
                    /* 可选：数据回环测试（自动添加帧头帧尾） */
                    rs485_send_data(rs485buf, len);
                }
            }
        }
        
        /* 监控栈使用情况 */
        uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
        
        /* 可选：定期发送心跳或其他维护任务 */
        // static uint32_t last_heartbeat = 0;
        // if ((xTaskGetTickCount() - last_heartbeat) > pdMS_TO_TICKS(1000)) {
        //     uint8_t heartbeat[] = {0x01, 0x02, 0x03};
        //     rs485_send_data(heartbeat, sizeof(heartbeat));
        //     last_heartbeat = xTaskGetTickCount();
        // }
        
        vTaskDelay(1);
    }
}

