#include "app_common.h"
#include "app_daemon.h"
#include "app_usbhs.h"
#include "usbd_cdc_if.h"

#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"

#include "LIBS/lib_usb_protocol/slidingWindowReceiver_c.h"
#include "LIBS/lib_usb_protocol/usb_protocol.h"

#include "usb_processor.h"

/* USBD句柄 */
USBD_HandleTypeDef g_usbd_handle = {0};

/**
 * @brief       USB任务 - 纯事件驱动版本
 */
void USBHsTask(void *pvParameters)
{
    SlidingWindowReceiver_C receiver;
    uint8_t usb_rx_buf[USBD_CDC_RX_BUF_SIZE];
    uint16_t data_len = 0;

    SWR_Init(&receiver, on_frame, NULL);
    
    srand(xTaskGetTickCount());
    
    /* 初始化USB */
    USBD_Init(&g_usbd_handle, &CDC_Desc, DEVICE_HS);
    USBD_RegisterClass(&g_usbd_handle, USBD_CDC_CLASS);
    USBD_CDC_RegisterInterface(&g_usbd_handle, &USBD_CDC_fops);
    USBD_Start(&g_usbd_handle);
    
    usb_printf("USB CDC 初始化完成\r\n");
    
    /* 等待USB枚举完成 */
    uint32_t timeout = xTaskGetTickCount();
    while (g_usbd_handle.dev_state != USBD_STATE_CONFIGURED) {
        vTaskDelay(pdMS_TO_TICKS(100));
        if ((xTaskGetTickCount() - timeout) > pdMS_TO_TICKS(5000)) {
            usb_printf("USB枚举超时\r\n");
            break;
        }
    }
    
    /* 枚举成功后初始化RTOS通信 */
    if (USB_CDC_Init_RTOS() == 0) {
        usb_printf("USB事件驱动模式已启用\r\n");
    }
    
    while(1) {
        /* 等待数据信号量 */
        if (xSemaphoreTake(xUsbRxSemaphore, 10) == pdTRUE) {
//        if (xSemaphoreTake(xUsbRxSemaphore, portMAX_DELAY) == pdTRUE) {
            /* 处理所有待处理数据包 */
            while (USB_CDC_Receive_From_Queue(usb_rx_buf, &data_len) == 0) {
                if (data_len > 0) {
                    /* 数据回显（可根据需要修改为业务逻辑） */
                    SWR_ProcessBytes(&receiver, usb_rx_buf, data_len);
                }
            }
        }
        
        USB_Display_All(g_IdaSystemStatus.st_dev_run.run_flag);
        IdaProcessor();
        /* 小延时避免任务过于频繁切换 */
//        vTaskDelay(pdMS_TO_TICKS(1));
        vTaskDelay(1);
    }
}
