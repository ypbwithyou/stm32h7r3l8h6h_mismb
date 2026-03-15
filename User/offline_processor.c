#include "offline_processor.h"
#include "ida_config.h"
#include "stdio.h"
#include "./FATFS/exfuns/fattester.h"
#include "./LIBS/lib_usb_protocol/usb_protocol.h"
#include "./LIBS/lib_dwt/lib_dwt_timestamp.h"
#include "usb_processor.h"
#include "collector_processor.h"
#include "./BSP/PCA9554A/pca9554a.h"
#include "./MALLOC/malloc.h"
#include "usb_processor.h"
#include "usbd_cdc_if.h"
#include "./LIBS/lib_file_utils/file_utils.h"
#include "./BSP/DMA_LIST/dma_list.h"  // 用于 dma_set_spi_xfer_size

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
SignalDataSource *g_offline_signal_source;                         // 离线信号数据来源配置缓存（最多24组）
TriggerParamHeaderDSP g_offline_TriggerParamHeader;                // 离线触发参数表头配置缓存
ScheduleParams g_offline_ScheduleParam[OFFLINE_SCHEDULE_ITEM_MAX]; // 离线计划表配置缓存
RECORD_FILE_HEADER g_recorde_file_head;

uint8_t g_schedule_run_status[OFFLINE_SCHEDULE_ITEM_MAX]; // 离线计划表项目执行情况:
                                                          // 0--未执行，1--执行中，2--执行完成

#define NANOSECONDS_PER_SECOND 1e9

static uint32_t record_frame_num = 0;

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
    g_offline_signal_source = (SignalDataSource *)mymalloc(SRAMEX, sizeof(SignalDataSource) * 24);
    if (!g_offline_signal_source)
    {
        usb_printf("[Record] Failed to allocate memory for signal source\r\n");
        return;
    }

    g_IdaSystemStatus.st_dev_offline.offline_mode = 0; // 默认离线模式为待机

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

    record_frame_num = 0;

    g_recorde_file_head.nVersion = RECORD_FILE_VERSION;
    g_recorde_file_head.nCreateTime = dwt_get_ns() / NANOSECONDS_PER_SECOND;
    g_recorde_file_head.nDeviceChNum = DEFAULT_CHANNEL_COUNT;
    g_recorde_file_head.nRecordNum = DEFAULT_CHANNEL_COUNT;
    g_recorde_file_head.nFrameNum = 0;
    g_recorde_file_head.nRecordMask = 0;
    g_recorde_file_head.nHeaderLength = sizeof(RECORD_FILE_HEADER) +
                                        sizeof(ChannelTableHeader) +
                                        sizeof(ChannelTableElem) * g_recorde_file_head.nDeviceChNum +
                                        sizeof(DeviceDetailInfo) +
                                        sizeof(DSAGlobalParams) +
                                        sizeof(SignalDataSource) * g_offline_GlobalParam.nSignalCount +
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

    g_schedule_run_status[idx] = STATUS_END;
}

// 采集开始处理
static void HandleAcqStart(uint8_t idx, uint32_t elapsed_seconds)
{
    if (g_schedule_run_status[idx] != STATUS_NO)
    {
        return;
    }

    // -----------------通道配置-----------------
    g_ch_enable_mask = 0;
    g_enabled_ch_cnt = 0;
    memset(g_spi_adc_cnt, 0, sizeof(g_spi_adc_cnt));

    for (size_t i = 0; i < g_offline_chCfgHeader.nTotalChannelNum; i++)
    {
        int32_t ch_id = g_offline_chCfgParam[i].nChannelID;
        if (g_offline_chCfgParam[0].nChannelID == 1)
            ch_id -= 1;

        if (ch_id >= 0 && ch_id < ADC_CH_TOTAL)
        {
            g_ch_enable_mask |= (1u << ch_id);
            g_enabled_chs[g_enabled_ch_cnt++] = (uint8_t)ch_id; // 按上位机顺序记录

            uint8_t spi_idx = ch_id % SPI_NUM;
            uint8_t adc_pos = ch_id / SPI_NUM;
            if ((adc_pos + 1) > g_spi_adc_cnt[spi_idx])
                g_spi_adc_cnt[spi_idx] = adc_pos + 1;
        }
    }

    usb_printf("spi_adc_cnt: [%d, %d, %d]  mask=0x%06lX\n",
               g_spi_adc_cnt[0], g_spi_adc_cnt[1], g_spi_adc_cnt[2],
               g_ch_enable_mask);

    /* 根据新的 g_spi_adc_cnt 设置 DMA 传输大小 */
    for (uint8_t spi_idx = 0; spi_idx < SPI_NUM; spi_idx++)
    {
        /* 每个ADC转换结果为2字节，计算总传输大小 */
        /* 如果g_spi_adc_cnt为0，使用默认值ADS8319_CHAIN_LENGTH */
        uint8_t adc_count = g_spi_adc_cnt[spi_idx];
        if (adc_count == 0)
        {
            adc_count = 8;  // ADS8319_CHAIN_LENGTH 默认值
        }
        uint32_t xfer_size = adc_count * 2;  // 2字节/ADC
        dma_set_spi_xfer_size(spi_idx, xfer_size);
        usb_printf("SPI%d: adc_count=%d, xfer_size=%d bytes\n", spi_idx, adc_count, xfer_size);
    }

    // -----------------------统计使能通道数-----------------------------------

    uint32_t enabled_cnt = 0;
    for (uint8_t i = 0; i < ADC_CH_TOTAL; i++)
        if (g_ch_enable_mask & (1u << i))
            enabled_cnt++;
    g_recorde_file_head.nRecordNum = enabled_cnt;

    usb_printf("g_recorde_file_head.nRecordNum: %d \n", g_recorde_file_head.nRecordNum);

    // ------------------------采样率配置----------------------------------

    uint32_t sample_rate = GetOfflineSampleRate();

    usb_printf("sample_rate:%d", sample_rate);

    // ------------------------启动采集----------------------------------

    if (sample_rate > 0)
    {
        CfgAdcSampleRate(sample_rate);

        g_IdaSystemStatus.st_dev_run.run_flag = 1;
        AdcCollectorContrl(g_IdaSystemStatus.st_dev_run.run_flag);
    }
    // ----------------------------------

    usb_printf("ACQ_Start: starting acquisition\n");

    g_schedule_run_status[idx] = STATUS_ING;

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
        uint32_t sample_rate = GetOfflineSampleRate();

        if (sample_rate == 0)
        {
            usb_printf("[Offline] Sample rate %.1f Hz not in table,"
                       " using default 25600 Hz\n",
                       g_offline_chCfgHeader.fHardwareSampleRate);
            sample_rate = 25600;
        }

        /* 在配置成功时才重置时间基准，避免调度时序偏移 */
        base_tick_ms = HAL_GetTick();
        config_done = 1;

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

            // if (*status == STATUS_NO)
            //     *status = STATUS_ING;

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

                    g_IdaSystemStatus.st_dev_offline.offline_mode = 0;
                }

                if (g_offline_ScheduleParam[i].nType == ACQ_Start)
                {
                    // 离线计划结束时

                    g_IdaSystemStatus.st_dev_offline.start_flag = 0;
                    g_IdaSystemStatus.st_dev_run.run_flag = 0;
                    AdcCollectorContrl(0);

                    g_IdaSystemStatus.st_dev_offline.offline_mode = 0;
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
static int8_t GetOfflineCfgParam(const char *f_name)
{
    FIL fil;
    UINT br;
    FRESULT res;
    const uint32_t FILE_OP_TIMEOUT = 5000;

    res = f_open(&fil, f_name, FA_READ);
    if (res != FR_OK)
    {
        usb_printf("Failed to open config file '%s': %d\n", f_name, res);
        return -13;
    }

// 统一错误处理宏，避免重复的 f_close + return
#define READ_CHECK(buf, size, errcode)                                     \
    do                                                                     \
    {                                                                      \
        UINT _expect = (UINT)(size);                                       \
        res = f_read_timeout(&fil, (buf), _expect, &br, FILE_OP_TIMEOUT);  \
        if (res != FR_OK || br != _expect)                                 \
        {                                                                  \
            usb_printf("Read failed at step %d: res=%d br=%u expect=%u\n", \
                       (errcode), res, br, _expect);                       \
            f_close(&fil);                                                 \
            return (errcode);                                              \
        }                                                                  \
    } while (0)

    // ── ChannelTableHeader ────────────────────────────────────────────────
    READ_CHECK(&g_offline_chCfgHeader, sizeof(g_offline_chCfgHeader), -1);

    if (g_offline_chCfgHeader.nTotalChannelNum > 24)
    {
        usb_printf("Invalid nTotalChannelNum: %u\n", g_offline_chCfgHeader.nTotalChannelNum);
        f_close(&fil);
        return -1;
    }

    // ── ChannelTableElem ──────────────────────────────────────────────────
    READ_CHECK(&g_offline_chCfgParam[0],
               g_offline_chCfgHeader.nTotalChannelNum * sizeof(ChannelTableElem), -3);

    // ── DSAGlobalParams ───────────────────────────────────────────────────
    READ_CHECK(&g_offline_GlobalParam, sizeof(g_offline_GlobalParam), -5);

    if (g_offline_GlobalParam.nScheduleCount > OFFLINE_SCHEDULE_ITEM_MAX)
    {
        usb_printf("Invalid nScheduleCount: %u\n", g_offline_GlobalParam.nScheduleCount);
        f_close(&fil);
        return -5;
    }

    usb_printf("nTotalChannelNum:%u nBlockSize:%u nScheduleCount:%u nSignalCount:%u\n",
               g_offline_chCfgHeader.nTotalChannelNum,
               g_offline_GlobalParam.nBlockSize,
               g_offline_GlobalParam.nScheduleCount,
               g_offline_GlobalParam.nSignalCount);

    // ── SignalDataSource ──────────────────────────────────────────────────
    READ_CHECK(&g_offline_signal_source[0],
               g_offline_GlobalParam.nSignalCount * sizeof(SignalDataSource), -7);

    // ── TriggerParamHeaderDSP ─────────────────────────────────────────────
    READ_CHECK(&g_offline_TriggerParamHeader, sizeof(TriggerParamHeaderDSP), -9);

    // ── ScheduleParams ────────────────────────────────────────────────────
    READ_CHECK(&g_offline_ScheduleParam[0],
               g_offline_GlobalParam.nScheduleCount * sizeof(ScheduleParams), -11);

#undef READ_CHECK

    res = f_close(&fil);
    if (res != FR_OK)
    {
        usb_printf("Failed to close file: %d\n", res);
        return -12;
    }

    usb_printf("Successfully loaded offline config from '%s'\n", f_name);
    return 0;
}

/**
 * @brief 校验离线配置参数的有效性
 * @return int8_t  0 表示校验通过，负值表示失败：
 *                  -1      : 总通道数无效 (须满足 0 < n <= MAX_OFFLINE_CHANNELS)
 *                  -2～-25 : 第 (|ret|-2) 个通道的 ID 与索引不匹配
 *                  -26     : 计划表数量无效 (须满足 0 < n <= MAX_OFFLINE_SCHEDULES)
 */
static int8_t CheckOfflineCfgParam(void)
{

#define MAX_OFFLINE_CHANNELS 24U
#define MAX_OFFLINE_SCHEDULES OFFLINE_SCHEDULE_ITEM_MAX

    // 1. 校验总通道数
    if (g_offline_chCfgHeader.nTotalChannelNum == 0 ||
        g_offline_chCfgHeader.nTotalChannelNum > MAX_OFFLINE_CHANNELS)
    {
        usb_printf("Check failed: invalid nTotalChannelNum (%u), must be 1..%u\n",
                   g_offline_chCfgHeader.nTotalChannelNum, MAX_OFFLINE_CHANNELS);
        return -1;
    }

    // 2. 校验每个通道 ID 是否从 0 开始连续递增
    for (uint8_t i = 0; i < g_offline_chCfgHeader.nTotalChannelNum; i++)
    {
        uint8_t expected = i;
        if (g_offline_chCfgParam[i].nChannelID != expected)
        {
            usb_printf("Check failed: channel[%u].nChannelID = %u, expected %u\n",
                       i, g_offline_chCfgParam[i].nChannelID, expected); // 原来 expected 写成了 i
            return -(int8_t)(i + 2);
        }
    }

    // 3. 校验计划表数量
    if (g_offline_GlobalParam.nScheduleCount == 0 ||
        g_offline_GlobalParam.nScheduleCount > MAX_OFFLINE_SCHEDULES)
    {
        usb_printf("Check failed: invalid nScheduleCount (%u), must be 1..%u\n",
                   g_offline_GlobalParam.nScheduleCount, MAX_OFFLINE_SCHEDULES);
        return -26;
    }

    // 打印计划表内容（调试用，正式版本可用条件编译包裹）
    // #ifdef DEBUG_OFFLINE_CFG
    for (uint8_t i = 0; i < g_offline_GlobalParam.nScheduleCount; i++)
    {
        usb_printf("ScheduleParam[%u]: type=%d param0=%f param1=%d\n",
                   i,
                   g_offline_ScheduleParam[i].nType,
                   (double)g_offline_ScheduleParam[i].param0,
                   g_offline_ScheduleParam[i].param1);
    }
    // #endif

    usb_printf("Offline config validation passed\n");

#undef MAX_OFFLINE_CHANNELS
#undef MAX_OFFLINE_SCHEDULES

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
#define WRITE_STRUCT(ptr, size, desc)                                                       \
    do                                                                                      \
    {                                                                                       \
        res = f_write_dma_safe(&g_offline_record_fil, (const uint8_t *)(ptr), (size), &bw); \
        if (res != FR_OK || bw != (size))                                                   \
        {                                                                                   \
            usb_printf("Failed to write %s to file '%s': res=%d, wrote=%u/%u\n",            \
                       (desc), file_name, res, bw, (UINT)(size));                           \
            f_close(&g_offline_record_fil);                                                 \
            return res ? res : FR_DENIED;                                                   \
        }                                                                                   \
    } while (0)

    // 5. 按顺序写入各个部分
    WRITE_STRUCT(&g_recorde_file_head, sizeof(g_recorde_file_head), "file header");

    WRITE_STRUCT(&g_offline_chCfgHeader, sizeof(g_offline_chCfgHeader), "channel config header");

    WRITE_STRUCT(&g_offline_chCfgParam[0], g_offline_chCfgHeader.nTotalChannelNum * sizeof(g_offline_chCfgParam[0]), "channel config parameters");

    DeviceDetailInfo device_info = {0};
    WRITE_STRUCT(&device_info, sizeof(device_info), "device info");

    WRITE_STRUCT(&g_offline_GlobalParam, sizeof(g_offline_GlobalParam), "global parameters");

    WRITE_STRUCT(g_offline_signal_source, sizeof(SignalDataSource) * g_offline_GlobalParam.nSignalCount, "global parameters");

    WRITE_STRUCT(&g_offline_TriggerParamHeader, sizeof(TriggerParamHeaderDSP), "trigger header");

    // 6. 全部写入成功
    usb_printf("Offline record file created successfully: %s\n", file_name);

#undef WRITE_STRUCT

    return FR_OK;
}

/**
 * @brief 将 ADC 环形缓冲区数据追加写入离线记录文件
 *        数据格式：按使能通道顺序，每通道独立写入：
 *                  [RECORD_FRAMEHEADER] + [nBlockSize * sizeof(short) 字节原始数据]
 */
static void OfflineDatasRecord(void)
{

    FRESULT res;
    UINT bw;

    g_offline_GlobalParam.nBlockSize = BLOCK_LEN;

    const uint32_t block_bytes = (uint32_t)g_offline_GlobalParam.nBlockSize * sizeof(short);

    /* ── 1. 检查是否有足够数据（所有使能通道都需满足 nBlockSize 个采样点）── */
    for (uint8_t i = 0; i < ADC_CH_TOTAL; i++)
    {
        if (!(g_ch_enable_mask & (1u << i)))
            continue;
        if (!g_cb_ch[i] || cb_size(g_cb_ch[i]) < (int)block_bytes)
            return; /* 任一使能通道不足，等待下次 */
    }

    /* ── 2. 按通道逐个写入 [RECORD_FRAMEHEADER + 数据] ── */
    short ch_buf[BLOCK_LEN]; /* 按实际最大 nBlockSize 调整 */

    for (uint8_t ch = 0; ch < ADC_CH_TOTAL; ch++)
    {
        if (!(g_ch_enable_mask & (1u << ch)))
            continue;

        /* 填写通道帧头 */
        RECORD_FRAMEHEADER rec_hdr;
        memset(&rec_hdr, 0, sizeof(rec_hdr));

        /* AoLocalColumn 部分 */
        rec_hdr.RecLocalColumn.nVersion = 0x12345678;
        rec_hdr.RecLocalColumn.nDataSource = 0;
        rec_hdr.RecLocalColumn.nFrameChCount = 1; /* 每个头只描述本通道 */
        rec_hdr.RecLocalColumn.nFrameLen = g_offline_GlobalParam.nBlockSize;
        rec_hdr.RecLocalColumn.nTotalFrameNum = record_frame_num;
        rec_hdr.RecLocalColumn.nCurNs = dwt_get_ns() / NANOSECONDS_PER_SECOND;
        rec_hdr.RecLocalColumn.gp11 = ch; /* 通道ID存入gp11，便于合并时区分 */

        rec_hdr.nValidNum = (unsigned int)g_offline_GlobalParam.nBlockSize;
        rec_hdr.nChID = (int)ch;

        record_frame_num++;

        if (record_frame_num == 1)
        {
            g_recorde_file_head.dRecValidStartTime = dwt_get_ns() / NANOSECONDS_PER_SECOND;
        }

        /* 写帧头 */
        res = f_write(&g_offline_record_fil, &rec_hdr, sizeof(rec_hdr), &bw);
        if (res != FR_OK || bw != sizeof(rec_hdr))
        {
            usb_printf("[Record] ch%u: write header failed, res=%d\n", ch, res);
            g_IdaSystemStatus.st_dev_record.record_status = RECORD_ERROR;
            f_close(&g_offline_record_fil);
            return;
        }

        /* 从环形缓冲区读取数据并写入文件 */
        cb_read(g_cb_ch[ch], (char *)ch_buf, block_bytes);
        res = f_write(&g_offline_record_fil, ch_buf, block_bytes, &bw);
        if (res != FR_OK || bw != block_bytes)
        {
            usb_printf("[Record] ch%u: write data failed, res=%d\n", ch, res);
            g_IdaSystemStatus.st_dev_record.record_status = RECORD_ERROR;
            f_close(&g_offline_record_fil);
            return;
        }
    }

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

    /* ── 3. 记录停止时的最终处理 ── */
    if (g_IdaSystemStatus.st_dev_record.record_status == RECORD_STOP)
    {
        g_recorde_file_head.nFrameNum = record_frame_num;
        g_recorde_file_head.dRecValidEndTime = dwt_get_ns() / NANOSECONDS_PER_SECOND;

        /* 回写文件头部（更新帧数） */
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
            usb_printf("[Record] Failed to update file header, res=%d\n", res);
        }

        f_sync(&g_offline_record_fil);
        f_close(&g_offline_record_fil);

        record_frame_num = 0;
        memset(&g_offline_record_fil, 0, sizeof(g_offline_record_fil));
        g_IdaSystemStatus.st_dev_record.record_status = RECORD_IDLE;

        usb_printf("[Record] Record stopped, total frames=%lu\n",
                   (unsigned long)g_recorde_file_head.nFrameNum);
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
        char full_path[256];

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
