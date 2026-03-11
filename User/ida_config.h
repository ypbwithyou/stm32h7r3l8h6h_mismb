#ifndef __IDA_CONFIG_H
#define __IDA_CONFIG_H

#include <stdint.h>

// 文件路径定义
#define DEVICE_INFO_FILE        "0:/DeviceInfo.ini"
#define OFFLINE_SCHEDULE_FILE   "0:/OfflineCfgSchedule.txt"
#define RECORD_FILE_PATH        "0:/RecordDataFiles"

// 设备工作模式定义
#define WORKMODE_ONLINE     (0)
#define WORKMODE_OFFLINE    (1)

#define BLOCK_LEN       (256)

/* 超时定义*/ 
enum timeout_ms 
{ 
    TIMEOUT_1000    =   1000,
    TIMEOUT_3000    =   3000,
    TIMEOUT_5000    =   5000,
    TIMEOUT_10000   =   10000,
    TIMEOUT_15000   =   15000,
    TIMEOUT_20000   =   20000,
    TIMEOUT_30000   =   30000,
}; 

typedef struct SystemStatus_t
{ 
    struct dev_link_status
    { 
        int8_t link_status;             // usbhs连接状态 
        int8_t disconnect_flag;         // 需要断开连接的标志位
    }st_dev_link; 
    struct dev_status
    {
        int8_t reset_all_flag;          // 设备整体软重启标志
        int8_t debug_status;            // DEBUG模式设置标志：正常工作模式（WORK）/DEBUG模式（DEBUG），DEBUG模式下不监测心跳
        int8_t work_mode;               // 设备运行模式：在线模式（0）/离线模式（1）
    }st_dev_mode;   
    struct dev_dsp_status   
    {   
        int8_t collect_cfg_flag;        // 采集配置状态，1：配置完成；0：未配置；-1：配置失败
        int8_t run_flag;                // DVS_RUN:1, DVS_STOP:0
        int8_t record_flag;             // 开始记录：1，停止记录：0
        int8_t err_flag;                // 运行过程中是否存在异常，异常：1，未见异常：0
    }st_dev_run;    
    struct dev_record_status    
    {   
        uint8_t     record_status;      // 当前记录状态：0--待记录，1--记录中，2--暂停记录，3--停止记录
    }st_dev_record;
    struct dev_offline_status
    {
        uint8_t     offline_mode;       // 离线模式：0--待机，1--离线运行
        uint8_t     run_status;         // 离线运行状态
        uint8_t     start_flag;         // START运行状态
        uint8_t     event_flag;         // EVENT运行状态
    }st_dev_offline;
}SystemStatus;

extern volatile SystemStatus g_IdaSystemStatus; 

int8_t IdaDeviceInit(void);
int8_t app_processor(void);

#endif /* __IDA_CONFIG_H */