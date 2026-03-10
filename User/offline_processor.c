#include "offline_processor.h"
#include "ida_config.h"
#include "stdio.h"
#include "./FATFS/exfuns/fattester.h"
#include "./LIBS/lib_usb_protocol/usb_protocol.h"
#include "./LIBS/lib_dwt/lib_dwt_timestamp.h"
#include "usb_processor.h"
#include "collector_processor.h"
#include "./BSP/PCA9554A/pca9554a.h"

#include "usb_processor.h"
#include "usbd_cdc_if.h"

extern CircularBuffer *g_cb_adc;

extern void AdcCollectorContrl(uint8_t run_status);

const Dev_ch_cfg_index g_off_HighPassFreq[] =
    {
        {0.05, HIGH_PASS_FREQ_005hz},
        {0.5, HIGH_PASS_FREQ_05hz},
        {1, HIGH_PASS_FREQ_1hz},
        {10, HIGH_PASS_FREQ_10hz},
};

const Dev_ch_cfg_index g_off_LowPassFreq[] =
    {
        {0.0, LOW_PASS_FREQ_110kHz},
        {20, LOW_PASS_FREQ_20Hz},
        {1000, LOW_PASS_FREQ_1kHz},
        {10000, LOW_PASS_FREQ_10kHz},
        {20000, LOW_PASS_FREQ_20kHz},
};

const Dev_ch_cfg_index g_off_ida_ch_rate[] =
    {
        {512.0, SAMPLE_RATE_INDEX_512HZ},
        {1024.0, SAMPLE_RATE_INDEX_1024HZ},
        {2048.0, SAMPLE_RATE_INDEX_2048HZ},
        {4096.0, SAMPLE_RATE_INDEX_4096HZ},
        {8192.0, SAMPLE_RATE_INDEX_8192HZ},
        {16384.0, SAMPLE_RATE_INDEX_16384HZ},
        {32768.0, SAMPLE_RATE_INDEX_32768HZ},
        {65536.0, SAMPLE_RATE_INDEX_65536HZ},
        {131072.0, SAMPLE_RATE_INDEX_131072HZ},
        {50.0, SAMPLE_RATE_INDEX_50HZ},
        {100.0, SAMPLE_RATE_INDEX_100H},
        {200.0, SAMPLE_RATE_INDEX_200H},
        {400.0, SAMPLE_RATE_INDEX_400H},
        {800.0, SAMPLE_RATE_INDEX_800HZ},
        {1600.0, SAMPLE_RATE_INDEX_1600HZ},
        {3200.0, SAMPLE_RATE_INDEX_3200HZ},
        {6400.0, SAMPLE_RATE_INDEX_6400HZ},
        {12800.0, SAMPLE_RATE_INDEX_12800HZ},
        {25600.0, SAMPLE_RATE_INDEX_25600HZ},
        {51200.0, SAMPLE_RATE_INDEX_51200HZ},
        {102400.0, SAMPLE_RATE_INDEX_102400HZ},
        {204800.0, SAMPLE_RATE_INDEX_204800HZ},
        {256000.0, SAMPLE_RATE_INDEX_256000HZ},
};

#define OFFLINE_SCHEDULE_ITEM_MAX 32
typedef enum ScheduleItemRunStatus
{
    STATUS_NO = 0,
    STATUS_ING,
    STATUS_END,
} ScheduleStatus;

ChannelTableHeader g_offline_chCfgHeader;
ChannelTableElem g_offline_chCfgParam[24]; // 离线通道配置缓存（最多24通道）
DSAGlobalParams g_offline_GlobalParam;
SignalDataSource g_offline_signal_source[24];                      // 离线信号数据来源配置缓存（最多24组）
TriggerParamHeaderDSP g_offline_TriggerParamHeader;                // 离线触发参数表头配置缓存
ScheduleParams g_offline_ScheduleParam[OFFLINE_SCHEDULE_ITEM_MAX]; // 离线计划表配置缓存（最多16组）
RECORD_FILE_HEADER g_recorde_file_head;

uint8_t g_schedule_run_status[OFFLINE_SCHEDULE_ITEM_MAX]; // 离线计划表项目执行情况:
                                                          // 0--未执行，1--执行中，2--执行完成

void SysRunStatusInit(void);
static int8_t GetOfflineCfgParam(const char *f_name);
static int8_t CheckOfflineCfgParam(void);
static FRESULT CreatOfflineRecordFile(uint32_t file_num);
static void OfflineDatasRecord(void);
static int delete_files_by_extension(const char *dir_path,
                                     const char *keep_exts[],
                                     uint32_t timeout_ms);

FIL g_offline_record_fil; // 离线记录文件指针

// 如文件中尚未定义，可添加以下宏
#define RECORD_FILE_VERSION 0x12345678U
#define DEFAULT_CHANNEL_COUNT 3U

static uint32_t file_num = 0;

// 扫描目录获取当前最大文件编号
static uint32_t ScanMaxFileNum(const char *dir_path)
{
    DIR dir;
    FILINFO finfo;
    uint32_t max_num = 0;

    FRESULT res = f_opendir(&dir, dir_path);
    if (res != FR_OK)
    {
        usb_printf("[Record] opendir failed: %s res=%d\r\n", dir_path, res);
        return 0;
    }

    while (1)
    {
        res = f_readdir(&dir, &finfo);
        if (res != FR_OK || finfo.fname[0] == 0)
            break; // 读完或出错

        // 跳过目录
        if (finfo.fattrib & AM_DIR)
            continue;

        // 检查扩展名是否为 .rec
        char *dot = strrchr(finfo.fname, '.');
        if (dot == NULL || strcasecmp(dot, ".rec") != 0)
            continue;

        // 解析文件名中的数字，例如 "00000001.rec" → 1
        uint32_t num = (uint32_t)strtoul(finfo.fname, NULL, 10);
        if (num > max_num)
            max_num = num;
    }

    f_closedir(&dir);

    usb_printf("[Record] ScanMaxFileNum: max=%lu\r\n", max_num);
    return max_num;
}

// 初始化时调用一次
void OfflineRecordInit(void)
{
    file_num = ScanMaxFileNum("0:/RecordDataFiles");
    usb_printf("[Record] file_num init to %lu, next will be %lu\r\n",
               file_num, file_num + 1);
}

// 安全的毫秒差计算（防止 uint32_t 溢出）
static uint32_t SafeElapsedMs(uint32_t old_tick, uint32_t new_tick)
{
    if (new_tick >= old_tick)
    {
        return new_tick - old_tick;
    }
    return (UINT32_MAX - old_tick) + new_tick + 1U;
}

// 获取离线采样率
static uint32_t GetOfflineSampleRate(void)
{
    uint32_t rate = 0;
    size_t count = sizeof(g_off_ida_ch_rate) / sizeof(g_off_ida_ch_rate[0]);

    for (size_t i = 0; i < count; i++)
    {
        if (g_offline_chCfgHeader.fHardwareSampleRate == g_off_ida_ch_rate[i].ch_cfg_value)
        {
            rate = g_off_ida_ch_rate[i].ch_cfg_value;
            break;
        }
    }
    return rate;
}

// 记录开始处理
static void HandleRecordStart(uint8_t idx)
{
    if (g_offline_record_fil.flag)
        return;

    usb_printf("Record_Start\n");

    memset(&g_recorde_file_head, 0, sizeof(g_recorde_file_head));

    g_recorde_file_head.nVersion = RECORD_FILE_VERSION;
    g_recorde_file_head.nCreateTime = dwt_get_ns();
    g_recorde_file_head.nDeviceChNum = DEFAULT_CHANNEL_COUNT;
    g_recorde_file_head.nRecordNum = DEFAULT_CHANNEL_COUNT;
    g_recorde_file_head.nFrameNum = 0;
    g_recorde_file_head.nRecordMask = 0;
    g_recorde_file_head.nHeaderLength = sizeof(RECORD_FILE_HEADER) +
                                        sizeof(ChannelTableHeader) +
                                        sizeof(ChannelTableElem) * g_recorde_file_head.nDeviceChNum +
                                        sizeof(DeviceDetailInfo) +
                                        sizeof(DSAGlobalParams) +
                                        sizeof(SignalDataSource) * g_recorde_file_head.nDeviceChNum +
                                        sizeof(TriggerParamHeaderDSP);
    g_recorde_file_head.nIndexPos = 0;
    g_recorde_file_head.nFrameDataSize = g_offline_GlobalParam.nBlockSize;
    g_recorde_file_head.nIsCalculate = 1;

    file_num++;

    if (CreatOfflineRecordFile(file_num) != FR_OK)
    {
        usb_printf("Create file fail\n");
        return;
    }

    g_IdaSystemStatus.st_dev_record.record_status = RECORD_RUN;

    g_schedule_run_status[idx] = STATUS_ING;
}

// 记录结束处理
static void HandleRecordEnd(uint8_t idx)
{
    if (g_IdaSystemStatus.st_dev_record.record_status != RECORD_RUN)
        return;

    usb_printf("Record_End\n");

    g_IdaSystemStatus.st_dev_record.record_status = RECORD_STOP;

    OfflineDatasRecord();

    f_close(&g_offline_record_fil);

    memset(&g_offline_record_fil, 0, sizeof(g_offline_record_fil));

    g_schedule_run_status[idx] = STATUS_END;
}

// 采集开始处理
static void HandleAcqStart(uint8_t idx, uint32_t elapsed_seconds)
{
    if (g_schedule_run_status[idx] != STATUS_NO)
    {
        return;
    }

    usb_printf("ACQ_Start: starting acquisition\n");

    g_schedule_run_status[idx] = STATUS_ING;
    g_IdaSystemStatus.st_dev_run.run_flag = 1;
    AdcCollectorContrl(1); // 强烈建议打开采集控制

    // 有持续时间限制且已超时，则立即结束
    if (g_offline_ScheduleParam[idx].param1 > 0)
    {
        uint32_t duration_end = (uint32_t)g_offline_ScheduleParam[idx].param0 +
                                (uint32_t)g_offline_ScheduleParam[idx].param1;

        if (elapsed_seconds >= duration_end)
        {
            g_IdaSystemStatus.st_dev_run.run_flag = 0;
            AdcCollectorContrl(0);
            g_schedule_run_status[idx] = STATUS_END;
        }
    }
}

static void ExecuteScheduleAction(uint8_t idx, uint32_t elapsed_seconds)
{
    switch (g_offline_ScheduleParam[idx].nType)
    {

    case Record_Start:
        HandleRecordStart(idx);
        break;

    case Record_End:
        HandleRecordEnd(idx);
        break;

    case ACQ_Start:
        HandleAcqStart(idx, elapsed_seconds);
        break;

    case ACQ_Stop:

        if (g_IdaSystemStatus.st_dev_run.run_flag)
        {
            g_IdaSystemStatus.st_dev_run.run_flag = 0;
            AdcCollectorContrl(0);
        }

        g_schedule_run_status[idx] = STATUS_END;
        break;

    default:
        break;
    }
}

/* 配置加载失败类型 */
typedef enum
{
    CFG_ERR_NONE = 0,
    CFG_ERR_NO_FILE = 1, /* 文件不存在 */
    CFG_ERR_CORRUPT = 2, /* 文件内容非法 */
    CFG_ERR_FS = 3,      /* 文件系统错误 */
} OfflineCfgErrType;

#define CFG_RETRY_INTERVAL_MS (5000U) /* 两次重试间最小间隔 */
#define CFG_RETRY_MAX (3U)            /* 最多重试次数，超出后进入永久等待 */

void offline_processor(uint8_t mode)
{
    static uint8_t prev_mode = WORKMODE_ONLINE;
    static uint32_t base_tick_ms = 0;
    static uint8_t config_done = 0;

    /* 配置重试状态（模式切换时清零） */
    static OfflineCfgErrType s_cfg_err_type = CFG_ERR_NONE;
    static uint32_t s_cfg_retry_tick = 0;
    static uint8_t s_cfg_retry_cnt = 0;

    // ---------------------------
    // 1 模式切换处理
    // ---------------------------

    if (mode != prev_mode)
    {
        usb_printf("Workmode change\n");

        if (g_IdaSystemStatus.st_dev_run.run_flag)
        {
            g_IdaSystemStatus.st_dev_run.run_flag = 0;
            AdcCollectorContrl(0);
        }

        AdcCbClear();

        SysRunStatusInit();

        prev_mode = mode;
        config_done = 0;
        s_cfg_err_type = CFG_ERR_NONE;
        s_cfg_retry_tick = 0;
        s_cfg_retry_cnt = 0;
        base_tick_ms = HAL_GetTick();
    }

    if (mode != WORKMODE_OFFLINE)
        return;

    // ---------------------------
    // 2 离线配置（含缺失/损坏处理）
    // ---------------------------

    if (!config_done)
    {
        uint32_t now_ms = HAL_GetTick();

        /* 上次已失败，判断是否需要等待或永久挂起 */
        if (s_cfg_err_type != CFG_ERR_NONE)
        {
            if (s_cfg_retry_cnt >= CFG_RETRY_MAX)
            {
                /* 超过最大重试次数：每 30s 打印一次提示，停止 IO */
                static uint32_t last_warn_ms = 0;
                if ((now_ms - last_warn_ms) >= 30000U)
                {
                    last_warn_ms = now_ms;
                    if (s_cfg_err_type == CFG_ERR_NO_FILE)
                        usb_printf("[Offline] Waiting for config '%s'."
                                   " Please upload it via USB.\n",
                                   OFFLINE_SCHEDULE_FILE);
                    else
                        usb_printf("[Offline] Config invalid (err=%d)."
                                   " Please re-upload a valid config file.\n",
                                   (int)s_cfg_err_type);
                }
                return;
            }

            /* 未超最大次数：等待重试间隔 */
            if ((now_ms - s_cfg_retry_tick) < CFG_RETRY_INTERVAL_MS)
                return;

            usb_printf("[Offline] Retrying config load (attempt %u/%u)...\n",
                       s_cfg_retry_cnt + 1, CFG_RETRY_MAX);
        }

        s_cfg_retry_tick = HAL_GetTick();

        /* 先用 f_stat 判断文件是否存在，给出更准确的提示 */
        FILINFO fno;
        FRESULT fres = f_stat(OFFLINE_SCHEDULE_FILE, &fno);
        if (fres == FR_NO_FILE || fres == FR_NO_PATH)
        {
            s_cfg_err_type = CFG_ERR_NO_FILE;
            s_cfg_retry_cnt++;
            g_IdaSystemStatus.st_dev_run.collect_cfg_flag = COLLECT_CFG_ERR;
            usb_printf("[Offline] Config file not found: '%s'"
                       " (retry %u/%u, next in %us)\n",
                       OFFLINE_SCHEDULE_FILE,
                       s_cfg_retry_cnt, CFG_RETRY_MAX,
                       CFG_RETRY_INTERVAL_MS / 1000U);
            return;
        }
        else if (fres != FR_OK)
        {
            s_cfg_err_type = CFG_ERR_FS;
            s_cfg_retry_cnt++;
            g_IdaSystemStatus.st_dev_run.collect_cfg_flag = COLLECT_CFG_ERR;
            usb_printf("[Offline] FS error when checking config: res=%d"
                       " (retry %u/%u)\n",
                       fres, s_cfg_retry_cnt, CFG_RETRY_MAX);
            return;
        }

        /* 文件存在，尝试读取并校验 */
        if (GetOfflineCfgParam(OFFLINE_SCHEDULE_FILE) < 0)
        {
            s_cfg_err_type = CFG_ERR_CORRUPT;
            s_cfg_retry_cnt++;
            g_IdaSystemStatus.st_dev_run.collect_cfg_flag = COLLECT_CFG_ERR;
            usb_printf("[Offline] Failed to parse config (retry %u/%u)\n",
                       s_cfg_retry_cnt, CFG_RETRY_MAX);
            return;
        }

        if (CheckOfflineCfgParam() < 0)
        {
            s_cfg_err_type = CFG_ERR_CORRUPT;
            s_cfg_retry_cnt++;
            g_IdaSystemStatus.st_dev_run.collect_cfg_flag = COLLECT_CFG_ERR;
            usb_printf("[Offline] Config validation failed (retry %u/%u)\n",
                       s_cfg_retry_cnt, CFG_RETRY_MAX);
            return;
        }

        /* 配置加载成功 */
        s_cfg_err_type = CFG_ERR_NONE;
        s_cfg_retry_cnt = 0;
        g_IdaSystemStatus.st_dev_run.collect_cfg_flag = COLLECT_CFG_OK;

        /* 从配置中查表确定采样率，找不到则保底 25600 Hz */
        uint32_t sample_rate = 0;
        for (uint8_t i = 0;
             i < (uint8_t)(sizeof(g_off_ida_ch_rate) / sizeof(g_off_ida_ch_rate[0]));
             i++)
        {
            if (g_offline_chCfgHeader.fHardwareSampleRate == g_off_ida_ch_rate[i].ch_cfg_value)
            {
                sample_rate = (uint32_t)g_off_ida_ch_rate[i].ch_cfg_value;
                break;
            }
        }
        if (sample_rate == 0)
        {
            usb_printf("[Offline] Sample rate %.1f Hz not in table,"
                       " using default 25600 Hz\n",
                       g_offline_chCfgHeader.fHardwareSampleRate);
            sample_rate = 25600;
        }

        CfgAdcSampleRate(sample_rate);

        /* 在配置成功时才重置时间基准，避免调度时序偏移 */
        base_tick_ms = HAL_GetTick();
        config_done = 1;

        usb_printf("[Offline] Config OK: %u ch, %u schedules, %lu Hz\n",
                   g_offline_chCfgHeader.nTotalChannelNum,
                   g_offline_GlobalParam.nScheduleCount,
                   sample_rate);

        // 离线计划启动时
        g_IdaSystemStatus.st_dev_offline.start_flag = 1;
    }

    // ---------------------------
    // 3 计算运行时间
    // ---------------------------

    uint32_t now_ms = HAL_GetTick();
    uint32_t elapsed_ms = SafeElapsedMs(base_tick_ms, now_ms);
    uint32_t elapsed_seconds = elapsed_ms / 1000;

    // ---------------------------
    // 4 执行调度
    // ---------------------------

    for (uint8_t i = 0; i < g_offline_GlobalParam.nScheduleCount; i++)
    {
        ScheduleStatus *status = &g_schedule_run_status[i];

        if (*status == STATUS_END)
            continue;

        uint32_t start_time = (uint32_t)g_offline_ScheduleParam[i].param0;

        if (elapsed_seconds < start_time)
            continue;

        // ---------- 未执行 ----------
        if (*status == STATUS_NO)
        {
            ExecuteScheduleAction(i, elapsed_seconds);

            if (*status == STATUS_NO)
                *status = STATUS_ING;

            continue;
        }

        // ---------- 运行中 ----------
        if (*status == STATUS_ING)
        {
            uint32_t duration = (uint32_t)g_offline_ScheduleParam[i].param1;

            if (duration == 0)
                continue;

            if (elapsed_seconds >= start_time + duration)
            {
                *status = STATUS_END;

                if (g_offline_ScheduleParam[i].nType == Record_Start)
                {
                    HandleRecordEnd(i);
                }

                if (g_offline_ScheduleParam[i].nType == ACQ_Start)
                {
                    // 离线计划结束时
                    g_IdaSystemStatus.st_dev_offline.start_flag = 0;
                    g_IdaSystemStatus.st_dev_run.run_flag = 0;
                    AdcCollectorContrl(0);
                }
            }
        }
    }

    // ---------------------------
    // 5 数据记录
    // ---------------------------

    if (g_IdaSystemStatus.st_dev_run.run_flag &&
        g_IdaSystemStatus.st_dev_record.record_status == RECORD_RUN)
    {
        if (g_offline_record_fil.flag)
        {
            OfflineDatasRecord();
        }
    }
}

void SysRunStatusInit(void)
{
    g_IdaSystemStatus.st_dev_run.collect_cfg_flag = COLLECT_CFG_INIT;
    g_IdaSystemStatus.st_dev_run.run_flag = 0;
    g_IdaSystemStatus.st_dev_run.record_flag = 0;
    g_IdaSystemStatus.st_dev_run.err_flag = 0;
    g_IdaSystemStatus.st_dev_record.record_status = RECORD_IDLE;
    g_IdaSystemStatus.st_dev_offline.start_flag = 0;
    g_IdaSystemStatus.st_dev_offline.event_flag = 0;
    memset(&g_offline_chCfgHeader, 0, sizeof(g_offline_chCfgHeader));
    memset(g_offline_chCfgParam, 0, sizeof(g_offline_chCfgParam));
    memset(&g_offline_GlobalParam, 0, sizeof(g_offline_GlobalParam));
    memset(g_offline_ScheduleParam, 0, sizeof(g_offline_ScheduleParam));
    memset(&g_recorde_file_head, 0, sizeof(g_recorde_file_head));
    memset(g_schedule_run_status, 0, sizeof(g_schedule_run_status));
}
/**
 * @brief 带超时的文件读取函数
 * @param fp 文件指针
 * @param buff 缓冲区
 * @param btr 要读取的字节数
 * @param br 实际读取的字节数
 * @param timeout_ms 超时时间（毫秒）
 * @return FRESULT 结果
 */
FRESULT f_read_timeout(FIL *fp, void *buff, UINT btr, UINT *br, uint32_t timeout_ms)
{
    uint32_t start_tick = HAL_GetTick();
    FRESULT res;

    // 创建一个临时的文件操作函数，带超时检测
    do
    {
        res = f_read(fp, buff, btr, br);
        if (res == FR_OK)
        {
            return res;
        }

        // 检查是否超时
        if ((HAL_GetTick() - start_tick) > timeout_ms)
        {
            return FR_TIMEOUT; // 超时错误
        }

        // 短暂延时后重试
        HAL_Delay(1);
    } while (res == FR_DISK_ERR || res == FR_NOT_READY);

    return res;
}

/**
 * @brief  Retrieve offline plan configuration from offline config file
 * @param  f_name   File name of the configuration file
 * @return int8_t   Result code (0 = success, negative = error)
 */
static int8_t GetOfflineCfgParam(const char *f_name)
{
    FIL fil;     // File object
    UINT bw, br; // Bytes written/read
    FRESULT res;
    uint32_t offset = 0;                   // File pointer offset
    const uint32_t FILE_OP_TIMEOUT = 5000; // File operation timeout (ms)

    // Open the configuration file in read mode
    res = f_open(&fil, f_name, FA_READ);
    if (res != FR_OK)
    {
        usb_printf("Failed to open configuration file '%s': %d\n", f_name, res);
        return -13;
    }

    // Read ChannelTableHeader
    res = f_read_timeout(&fil, &g_offline_chCfgHeader, sizeof(g_offline_chCfgHeader), &br, FILE_OP_TIMEOUT);
    if ((res != FR_OK) || (br <= 0) || (g_offline_chCfgHeader.nTotalChannelNum > 24))
    {
        usb_printf("Failed to read channel table header: %d (read %u bytes)\n", res, br);
        f_close(&fil);
        return -1;
    }

    // Read ChannelTableElem
    offset += sizeof(g_offline_chCfgHeader);
    res = f_lseek(&fil, offset);
    if (res != FR_OK)
    {
        usb_printf("Failed to seek to channel table elements offset: %d\n", res);
        f_close(&fil);
        return -2;
    }

    res = f_read_timeout(&fil, &g_offline_chCfgParam[0],
                         g_offline_chCfgHeader.nTotalChannelNum * sizeof(ChannelTableElem),
                         &br, FILE_OP_TIMEOUT);
    if (res != FR_OK)
    {
        usb_printf("Failed to read channel configuration parameters: %d\n", res);
        f_close(&fil);
        return -3;
    }

    // Read DSAGlobalParams
    offset += g_offline_chCfgHeader.nTotalChannelNum * sizeof(ChannelTableElem);
    res = f_lseek(&fil, offset);
    if (res != FR_OK)
    {
        usb_printf("Failed to seek to global parameters offset: %d\n", res);
        f_close(&fil);
        return -4;
    }

    res = f_read_timeout(&fil, &g_offline_GlobalParam, sizeof(g_offline_GlobalParam), &br, FILE_OP_TIMEOUT);
    if ((res != FR_OK) || (g_offline_GlobalParam.nScheduleCount > 16))
    {
        usb_printf("Failed to read global parameters: %d (schedule count = %u)\n",
                   res, g_offline_GlobalParam.nScheduleCount);
        f_close(&fil);
        return -5;
    }

    usb_printf("nBlockSize: %d nScheduleCount: %d nSignalCount: %d\n", g_offline_GlobalParam.nBlockSize, g_offline_GlobalParam.nScheduleCount, g_offline_GlobalParam.nSignalCount);

    // // Read SignalDataSource
    // offset += sizeof(DSAGlobalParams);
    // res = f_lseek(&fil, offset);
    // if (res != FR_OK)
    // {
    //     usb_printf("Failed to seek to signal data source offset: %d\n", res);
    //     f_close(&fil);
    //     return -6;
    // }

    // res = f_read_timeout(&fil, &g_offline_signal_source[0],
    //                      g_offline_GlobalParam.nSignalCount * sizeof(SignalDataSource),
    //                      &br, FILE_OP_TIMEOUT);
    // if (res != FR_OK)
    // {
    //     usb_printf("Failed to read signal data source parameters: %d\n", res);
    //     f_close(&fil);
    //     return -7;
    // }

    // // Read TriggerParamHeaderDSP
    // offset += g_offline_GlobalParam.nSignalCount * sizeof(SignalDataSource);
    // res = f_lseek(&fil, offset);
    // if (res != FR_OK)
    // {
    //     usb_printf("Failed to seek to TriggerParamHeaderDSP offset: %d\n", res);
    //     f_close(&fil);
    //     return -8;
    // }

    // res = f_read_timeout(&fil, &g_offline_TriggerParamHeader,
    //                      sizeof(TriggerParamHeaderDSP),
    //                      &br, FILE_OP_TIMEOUT);
    // if (res != FR_OK)
    // {
    //     usb_printf("Failed to read TriggerParamHeaderDSP parameters: %d\n", res);
    //     f_close(&fil);
    //     return -9;
    // }

    // Read ScheduleParams
    // offset +=  sizeof(TriggerParamHeaderDSP);
    offset += sizeof(DSAGlobalParams);
    res = f_lseek(&fil, offset);
    if (res != FR_OK)
    {
        usb_printf("Failed to seek to schedule parameters offset: %d\n", res);
        f_close(&fil);
        return -10;
    }

    res = f_read_timeout(&fil, &g_offline_ScheduleParam[0],
                         g_offline_GlobalParam.nScheduleCount * sizeof(ScheduleParams),
                         &br, FILE_OP_TIMEOUT);
    if (res != FR_OK)
    {
        usb_printf("Failed to read schedule parameters: %d\n", res);
        f_close(&fil);
        return -11;
    }

    // Close the file
    res = f_close(&fil);
    if (res != FR_OK)
    {
        usb_printf("Failed to close configuration file: %d\n", res);
        return -12;
    }

    usb_printf("Successfully loaded offline configuration from file '%s'\n", f_name);
    return 0; // Success
}

/**
 * @brief 校验离线配置参数的有效性
 * @return int8_t  0 表示校验通过
 *                负值表示校验失败，具体含义见下表：
 *                  -1  : 总通道数超出允许范围 (0 < nTotalChannelNum <= MAX_OFFLINE_CHANNELS)
 *                -2～-25 : 通道ID不连续或与索引不匹配 (返回 - (实际索引+2))
 *                  -26 : 计划表数量超出允许范围 (0 < nScheduleCount <= MAX_OFFLINE_SCHEDULES)
 *                -27 : 其他未预期错误（预留）
 */
static int8_t CheckOfflineCfgParam(void)
{
// 定义最大限制（建议移到头文件宏定义，便于统一维护）
#define MAX_OFFLINE_CHANNELS 24U
#define MAX_OFFLINE_SCHEDULES 16U

    // 1. 校验总通道数
    if (g_offline_chCfgHeader.nTotalChannelNum == 0 ||
        g_offline_chCfgHeader.nTotalChannelNum > MAX_OFFLINE_CHANNELS)
    {
        usb_printf("Offline config check failed: invalid total channel count (%u)\n",
                   g_offline_chCfgHeader.nTotalChannelNum);
        return -1;
    }

    // 2. 逐个校验通道ID是否连续且等于索引
    for (uint8_t i = 0; i < g_offline_chCfgHeader.nTotalChannelNum; i++)
    {
        if (g_offline_chCfgParam[i].nChannelID != i)
        {
            usb_printf("Offline config check failed: channel index %u has mismatched ID (%u != %u)\n",
                       i, g_offline_chCfgParam[i].nChannelID, i);
            return -(int8_t)(i + 2);
        }
    }

    // 3. 校验计划表数量
    if (g_offline_GlobalParam.nScheduleCount == 0 ||
        g_offline_GlobalParam.nScheduleCount > MAX_OFFLINE_SCHEDULES)
    {
        usb_printf("Offline config check failed: invalid schedule count (%u)\n",
                   g_offline_GlobalParam.nScheduleCount);
        return -26;
    }

    for (int i = 0; i < g_offline_GlobalParam.nScheduleCount; i++)
    {
        usb_printf("g_offline_ScheduleParam[%d]: %d - (%f, %d)\n", i, g_offline_ScheduleParam[i].nType, g_offline_ScheduleParam[i].param0, g_offline_ScheduleParam[i].param1);
    }

    // 可选：增加其他关键字段校验（视实际需求）
    // if (g_offline_chCfgHeader.fHardwareSampleRate == 0 ||
    //     g_offline_chCfgHeader.fHardwareSampleRate > SOME_MAX_RATE)
    // {
    //     usb_printf("Invalid sample rate in offline config\n");
    //     return -27;
    // }

    usb_printf("Offline configuration parameters validation passed\n");
    return 0;
}

/**
 * @brief 创建离线记录文件并写入所有头部信息
 * @param file_num 文件序号（用于生成文件名，如 00000001.rec）
 * @return FRESULT 操作结果，FR_OK 表示成功，其它值表示失败
 */
FRESULT CreatOfflineRecordFile(uint32_t file_num)
{
    char file_name[64];
    FRESULT res;
    UINT bw;

    // 1. 构建文件名
    snprintf(file_name, sizeof(file_name), "%s/%08u.rec", RECORD_FILE_PATH, file_num);

    // 2. 确保目录存在
    res = f_mkdir(RECORD_FILE_PATH);
    if (res != FR_OK && res != FR_EXIST)
    {
        usb_printf("Failed to create directory '%s': %d\n", RECORD_FILE_PATH, res);
        return res;
    }

    // 3. 打开文件（强制覆盖）
    res = f_open(&g_offline_record_fil, file_name, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK)
    {
        if (res == FR_EXIST) // 理论上 CREATE_ALWAYS 不应出现此情况，但以防万一
        {
            f_unlink(file_name);
            res = f_open(&g_offline_record_fil, file_name, FA_CREATE_ALWAYS | FA_WRITE);
        }
        if (res != FR_OK)
        {
            usb_printf("Failed to open file '%s' for writing: %d\n", file_name, res);
            return res;
        }
    }

// 4. 定义写入段落的统一处理宏（大幅减少重复代码）
#define WRITE_STRUCT(ptr, size, desc)                                            \
    do                                                                           \
    {                                                                            \
        res = f_write(&g_offline_record_fil, (ptr), (size), &bw);                \
        if (res != FR_OK || bw != (size))                                        \
        {                                                                        \
            usb_printf("Failed to write %s to file '%s': res=%d, wrote=%u/%u\n", \
                       (desc), file_name, res, bw, (UINT)(size));                \
            f_close(&g_offline_record_fil);                                      \
            return res ? res : FR_DENIED;                                        \
        }                                                                        \
    } while (0)

    // 5. 按顺序写入各个部分
    WRITE_STRUCT(&g_recorde_file_head, sizeof(g_recorde_file_head), "file header");

    WRITE_STRUCT(&g_offline_chCfgHeader, sizeof(g_offline_chCfgHeader), "channel config header");

    WRITE_STRUCT(&g_offline_chCfgParam[0], g_offline_chCfgHeader.nTotalChannelNum * sizeof(g_offline_chCfgParam[0]), "channel config parameters");

    DeviceDetailInfo device_info = {0};
    WRITE_STRUCT(&device_info, sizeof(device_info), "device info");

    WRITE_STRUCT(&g_offline_GlobalParam, sizeof(g_offline_GlobalParam), "global parameters");

    WRITE_STRUCT(&g_offline_signal_source, sizeof(SignalDataSource) * g_offline_GlobalParam.nSignalCount, "global parameters");

    WRITE_STRUCT(&g_offline_TriggerParamHeader, sizeof(TriggerParamHeaderDSP), "trigger header");

    // 6. 全部写入成功
    usb_printf("Offline record file created successfully: %s\n", file_name);

#undef WRITE_STRUCT

    return FR_OK;
}

/**
 * @brief 将 ADC 环形缓冲区数据追加写入离线记录文件
 *        数据格式：每帧 = [AoLocalColumn头] + [通道0所有采样点] + [通道1所有采样点] + ...
 */
static void OfflineDatasRecord(void)
{
    static uint32_t frame_num = 0;
    FRESULT res;
    UINT bw;

    // 计算一次“完整帧组”所需的字节数（所有通道 × BLOCK_LEN 个采样点）
    const uint32_t bytes_per_frame_group = SPI_NUM * BLOCK_LEN * sizeof(short);

    // 环形缓冲区当前可用字节数
    int avail = cb_size(g_cb_adc);

    while (avail >= bytes_per_frame_group)
    {
        AoLocalColumn data_head = {
            .nVersion = 0x12345678,
            .nDataSource = 0,
            .nFrameChCount = SPI_NUM,
            .nFrameLen = BLOCK_LEN,
            .nTotalFrameNum = ++frame_num, // 先自增再使用
            .nCurNs = dwt_get_ns()};

        // 读取原始数据（通道优先排列）
        short get_data[SPI_NUM][BLOCK_LEN];
        cb_read(g_cb_adc, (char *)get_data, bytes_per_frame_group);

        // 写入文件头（每帧只写一次）

        res = f_write(&g_offline_record_fil, &data_head, sizeof(data_head), &bw);
        // usb_printf("f_write, res=%d\n", res);
        if (res != FR_OK || bw != sizeof(data_head))
        {
            usb_printf("Offline record: failed to write frame header, res=%d\n", res);
            g_IdaSystemStatus.st_dev_record.record_status = RECORD_ERROR; // 建议增加错误状态
            f_close(&g_offline_record_fil);
            return;
        }

        // 连续写入所有通道的数据（按通道顺序）
        for (uint8_t ch = 0; ch < SPI_NUM; ch++)
        {
            res = f_write(&g_offline_record_fil,
                          &get_data[ch][0],
                          BLOCK_LEN * sizeof(short),
                          &bw);

            if (res != FR_OK || bw != BLOCK_LEN * sizeof(short))
            {
                usb_printf("Offline record: failed to write channel %u data, res=%d\n", ch, res);
                g_IdaSystemStatus.st_dev_record.record_status = RECORD_ERROR;
                f_close(&g_offline_record_fil);
                return;
            }
        }

        // 可选：每写入 N 帧同步一次（权衡性能与掉电安全性）
        // static uint32_t sync_counter = 0;
        // if (++sync_counter >= 50) {
        //     f_sync(&g_offline_record_fil);
        //     sync_counter = 0;
        // }

        // 在每个帧组写入后
        // static uint32_t last_sync_pos = 0;
        // uint32_t current_pos = f_tell(&g_offline_record_fil);

        // if (current_pos - last_sync_pos >= 65536U)
        // { // 每 64 KB 同步一次
        //     FRESULT sync_res = f_sync(&g_offline_record_fil);
        //     if (sync_res != FR_OK)
        //     {
        //         usb_printf("f_sync failed during record, res=%d\n", sync_res);
        //     }
        //     last_sync_pos = current_pos;
        // }

        static uint32_t last_tell = 0;
        static uint32_t last_time = 0;
        // 每秒打印一次
        if (HAL_GetTick() - last_time >= 1000)
        {
            uint32_t now_tell = f_tell(&g_offline_record_fil);
            uint32_t bytes_per_sec = now_tell - last_tell;
            usb_printf("Write speed: %lu KB/s\n", bytes_per_sec / 1024U);
            last_tell = now_tell;
            last_time = HAL_GetTick();
        }

        // 更新可用字节数
        avail = cb_size(g_cb_adc);
    }

    // ── 记录停止时的最终处理 ───────────────────────────────────────
    if (g_IdaSystemStatus.st_dev_record.record_status == RECORD_STOP)
    {

        // 更新文件总帧数
        g_recorde_file_head.nFrameNum = frame_num;

        // 回写文件头部
        res = f_lseek(&g_offline_record_fil, 0);
        if (res == FR_OK)
        {
            res = f_write(&g_offline_record_fil,
                          &g_recorde_file_head,
                          sizeof(g_recorde_file_head),
                          &bw);
        }

        if (res != FR_OK || bw != sizeof(g_recorde_file_head))
        {
            usb_printf("Failed to update file header during stop, res=%d\n", res);
        }

        // 确保所有数据落盘
        f_sync(&g_offline_record_fil);

        // 关闭文件
        f_close(&g_offline_record_fil);

        // 重置帧计数，为下一次记录准备
        frame_num = 0;

        // 可选：清空标志
        memset(&g_offline_record_fil, 0, sizeof(g_offline_record_fil));
        g_IdaSystemStatus.st_dev_record.record_status = RECORD_IDLE; // 建议增加空闲状态

        usb_printf("g_IdaSystemStatus.st_dev_record.record_status == RECORD_STOP, res=%d\n", res);
    }
}

/**
 * @brief 删除后缀不匹配的文件（简单高效版）
 * @param dir_path 目录路径，如："0:/" 或 "0:/data"
 * @param keep_exts 要保留的后缀数组，如：{".jpg", ".txt", NULL}
 * @param timeout_ms 超时时间（毫秒）
 * @return 删除的文件数量，负数表示错误
 */
static int delete_files_by_extension(const char *dir_path,
                                     const char *keep_exts[],
                                     uint32_t timeout_ms)
{
    FRESULT res;
    DIR dir;
    FILINFO fno;
    int deleted = 0;
    uint32_t start_tick;

    /* 参数检查 */
    if (dir_path == NULL || keep_exts == NULL)
    {
        return -1;
    }

    start_tick = HAL_GetTick();

    /* 打开目录 */
    res = f_opendir(&dir, dir_path);
    if (res != FR_OK)
    {
        usb_printf("delete_files_by_extension f_opendir : %d \n", res);
        return -2;
    }

    /* 遍历目录 */
    while (1)
    {
        /* 超时检查 */
        //        if (timeout_ms > 0 && (HAL_GetTick() - start_tick) > timeout_ms) {
        //            printf("操作超时\n");
        //            break;
        //        }

        /* 读取目录项 */
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0)
        {
            break; /* 错误或结束 */
        }

        /* 跳过目录和特殊文件 */
        if (fno.fattrib & AM_DIR)
        {
            continue; /* 跳过目录 */
        }

        if (strcmp(fno.fname, ".") == 0 || strcmp(fno.fname, "..") == 0)
        {
            continue; /* 跳过特殊项 */
        }

        /* 检查文件后缀 */
        const char *dot = strrchr(fno.fname, '.');

        /* 情况1: 文件没有后缀 */
        if (dot == NULL)
        {
            /* 如果配置不允许无后缀文件，则删除 */
            int found = 0;
            for (int i = 0; keep_exts[i] != NULL; i++)
            {
                if (strcmp("", keep_exts[i]) == 0)
                {
                    found = 1;
                    break;
                }
            }
            if (!found)
            {
                goto delete_file;
            }
            continue;
        }

        /* 情况2: 检查后缀是否在保留列表中 */
        int should_keep = 0;
        for (int i = 0; keep_exts[i] != NULL; i++)
        {
            /* 不区分大小写比较 */
            if (strcasecmp(dot, keep_exts[i]) == 0)
            {
                should_keep = 1;
                break;
            }
        }

        /* 如果不在保留列表中，删除文件 */
        if (!should_keep)
        {
            char full_path[256];
        delete_file:
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, fno.fname);

            res = f_unlink(full_path);
            if (res == FR_OK)
            {
                deleted++;

                usb_printf("delete_files_by_extension  %s\n", fno.fname);
            }
            else
            {
                usb_printf("delete_files_by_extension  %s (error: %d)\n", fno.fname, res);
            }
        }
    }

    /* 关闭目录 */
    f_closedir(&dir);

    return deleted;
}
