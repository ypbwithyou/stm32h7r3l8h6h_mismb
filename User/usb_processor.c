#include "app_common.h"
#include "usbd_cdc_if.h"

#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"

#include "./LIBS/lib_usb_protocol/slidingWindowReceiver_c.h"
#include "./LIBS/lib_usb_protocol/usb_protocol.h"
#include "./LIBS/lib_circular_buffer/CircularBuffer.h"
#include "./LIBS/lib_dwt/lib_dwt_timestamp.h"
#include "./LIBS/lib_base_calculation/calculate_check.h"
#include "./LIBS/lib_file_utils/file_utils.h"
#include "./LIBS/lib_ini_fatfs/ini_fatfs.h"

#include "usb_processor.h"
#include "collector_processor.h"
#include "./FATFS/exfuns/fattester.h"

/*********************************************************************************/
// typedef struct
//{
//     float       ch_cfg_value;
//     uint32_t    ch_set_index;
// } Dev_ch_cfg_index;
const Dev_ch_cfg_index g_HighPassFreq[] =
    {
        {0.05, HIGH_PASS_FREQ_005hz},
        {0.5, HIGH_PASS_FREQ_05hz},
        {1, HIGH_PASS_FREQ_1hz},
        {10, HIGH_PASS_FREQ_10hz},
};
const Dev_ch_cfg_index g_LowPassFreq[] =
    {
        {0.0, LOW_PASS_FREQ_110kHz},
        {20, LOW_PASS_FREQ_20Hz},
        {1000, LOW_PASS_FREQ_1kHz},
        {10000, LOW_PASS_FREQ_10kHz},
        {20000, LOW_PASS_FREQ_20kHz},
}; // 浣庨€氭护娉�
const Dev_ch_cfg_index g_ida_ch_rate[] =
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
}; // 鏈増鏈敮鎸佺殑閲囨牱鐜�

/*********************************************************************************/
FRESULT clear_file_content(const char *path);
static uint32_t USB_DetectPeriod(void);

static uint32_t USB_Connect_Reply(uint8_t *data_in, uint32_t data_len);
static uint32_t USB_Disconnect_Reply(uint8_t *data_in, uint32_t data_len);
static uint32_t USB_DevConfig_Done(uint8_t *data_in, uint32_t data_len);
static uint32_t USB_CollectChCfg_Reply(uint8_t *data_in, uint32_t data_len);
static uint32_t USB_Run_Reply(uint8_t *data_in, uint32_t data_len);
static uint32_t USB_Stop_Reply(uint8_t *data_in, uint32_t data_len);
static uint32_t USB_Display_Reply(uint8_t *data_in, uint32_t data_len);
static uint32_t USB_Upgrad_Reply(uint8_t *data_in, uint32_t data_len);
static uint32_t USB_OfflineCfg_Reply(uint8_t *data_in, uint32_t data_len);

static uint32_t USB_GetFilelist(uint8_t *data_in, uint32_t data_len);
static uint32_t USB_DeleteFile(uint8_t *data_in, uint32_t data_len);
static uint32_t USB_DownloadFileStart(uint8_t *data_in, uint32_t data_len);
static uint32_t USB_DownloadFileStop(uint8_t *data_in, uint32_t data_len);
static uint32_t USB_DownloadFileDataAck(uint8_t *data_in, uint32_t data_len); 

static uint32_t PackReplyWithoutDatas(uint32_t event_id);

typedef uint32_t (*usb_protocol_handler)(uint8_t *data_in, uint32_t data_len);

typedef struct
{
    uint32_t usb_frame_tag;                  // frame id
    usb_protocol_handler usb_process_handle; // protocol handler
} USB_ProtocoItems;

/*接收数据解包*/
static const USB_ProtocoItems protocol_getdata[] =
    {
        {DVS_INIT_CONNECT, USB_Connect_Reply},
        {DVS_INIT_DISCONNECT, USB_Disconnect_Reply},
        {DVS_INIT_DEVCONFIG_UPDATE_Done, USB_DevConfig_Done},
        {DVSARM_CSP_START, USB_CollectChCfg_Reply},
        {DVSARM_RUN, USB_Run_Reply},
        {DVSARM_STOP, USB_Stop_Reply},
        {DVSARM_DISPNEXT, USB_Display_Reply},
        {DVS_INIT_ARM_UPGRADE_REQ, USB_Upgrad_Reply},
        {DVS_CSP_OFFLINE_SETCONFIG, USB_OfflineCfg_Reply},

        {DVS_FILE_GET_FILELIST, USB_GetFilelist},
        {DVS_FILE_DELETE, USB_DeleteFile},
        {DVS_FILE_DOWNLOAD_START, USB_DownloadFileStart},
        {DVS_FILE_DOWNLOAD_STOP, USB_DownloadFileStop},
        {DVS_FILE_DOWNLOAD_DATA_ACK, USB_DownloadFileDataAck},
};

/*********************************************************************************/

USBD_HandleTypeDef g_usbd_handle = {0};

// SystemStatus g_IdaSystemStatus;
extern CircularBuffer *g_cb_adc;

// 主卡设备信息
DeviceInfo g_dev_info;
// 子卡设备信息
SubDevicelnfo g_SubDevicelnfo[SUBDEV_NUM_MAX];

uint64_t g_reset_time = 0;

/*********************************************************************************/

void usb_init(void)
{

#if UPGRADE_ON
    // ────────────────────────────────────────────────
    // STM32H7 推荐方式：使用 HAL 彻底去初始化并复位 USB
    // ────────────────────────────────────────────────

    __disable_irq(); // 先关闭中断，避免干扰

    // 1. 如果已存在 USB 句柄，先调用 DeInit（清理 HAL 内部状态）
    if (g_usbd_handle.pClassData != NULL || g_usbd_handle.dev_state != USBD_STATE_DEFAULT)
    {
        USBD_DeInit(&g_usbd_handle);
    }

// 2. 关闭 USB 时钟（根据您的配置选择 HS 或 FS）
#if defined(USE_USB_HS_IN_FS) || defined(DEVICE_HS)
    __HAL_RCC_USB_OTG_HS_CLK_DISABLE();
    // __HAL_RCC_USB_OTG_HS_ULPI_CLK_DISABLE(); // 如果用了 ULPI PHY
#else
    __HAL_RCC_USB_OTG_FS_CLK_DISABLE();
#endif

// 3. 强制复位 USB 外设（H7 使用 RCC_AHBxRSTR）
#if defined(USE_USB_HS_IN_FS) || defined(DEVICE_HS)
    __HAL_RCC_USB_OTG_HS_FORCE_RESET();
    HAL_Delay(20);
    __HAL_RCC_USB_OTG_HS_RELEASE_RESET();
#else
    __HAL_RCC_USB_OTG_FS_FORCE_RESET();
    HAL_Delay(20);
    __HAL_RCC_USB_OTG_FS_RELEASE_RESET();
#endif

// 4. 清除 USB 中断挂起（防止残留中断触发）
#if defined(USE_USB_HS_IN_FS) || defined(DEVICE_HS)
    NVIC_DisableIRQ(OTG_HS_IRQn);
    NVIC_ClearPendingIRQ(OTG_HS_IRQn);
#else
    NVIC_DisableIRQ(OTG_FS_IRQn);
    NVIC_ClearPendingIRQ(OTG_FS_IRQn);
#endif

    HAL_Delay(50); // 等待硬件稳定

    __enable_irq();

// ────────────────────────────────────────────────
#endif

    /* 鍒濆鍖朥SB */
    USBD_Init(&g_usbd_handle, &CDC_Desc, DEVICE_HS);
    USBD_RegisterClass(&g_usbd_handle, USBD_CDC_CLASS);
    USBD_CDC_RegisterInterface(&g_usbd_handle, &USBD_CDC_fops);
    USBD_Start(&g_usbd_handle);
}

void SystemStatusInit(void)
{
    memset((void *)&g_IdaSystemStatus, 0, sizeof(g_IdaSystemStatus));
}

static uint32_t PackReplyWithoutDatas(uint32_t event_id)
{
    FrameHeadInfo frame_head = create_default_frame_head(0);
    UserDataHeadInfo user_head = create_user_data_head(event_id,
                                                       SOURCE_TYPE_NO_DATA,
                                                       DESTINATION_ARM_TO_PC,
                                                       0);

    uint32_t packet_len = 0;
    pack_data(NULL, 0, &user_head, &frame_head, &packet_len);
    return packet_len;
}

static uint32_t USB_DetectPeriod(void)
{
    uint8_t serial_num = 0;
    uint8_t send_frame[sizeof(DeviceInfo) + sizeof(SubDevicelnfo) * SUBDEV_NUM_MAX];
    uint32_t send_len = 0;

    // 获取子卡设备信息
    uint8_t subden_num = 0; // 子卡设备数量
    // for(uint8_t i=0; i<SUBDEV_NUM_MAX; i++) {
    //     if (g_SubDevicelnfo[i].SlotId != 0) {
    //         subden_num++;
    //         memcpy(&send_frame[sizeof(DeviceInfo)+sizeof(SubDevicelnfo)*(subden_num-1)], &g_SubDevicelnfo[i], sizeof(SubDevicelnfo));
    //     }
    // }
    SubDevicelnfo subden_info[SUBDEN_NUM];
    subden_num = 1;
    strcpy((char *)&subden_info[0].SerialNumber[0], "MIRA3102501001");
    strcpy((char *)&subden_info[0].DeviceName[0], Name_Mini_SliceAccel);
    strcpy((char *)&subden_info[0].Version[0], "1.0.0.0_20260114");
    subden_info[0].DeviceType = Mini_SliceAccel;
    subden_info[0].SlotId = 1;
    subden_info[0].Sensitivity = 3; // 浼犳劅鍣ㄧ伒鏁忓害绱㈠紩
    memcpy(send_frame + sizeof(DeviceInfo), subden_info[0].SerialNumber, sizeof(SubDevicelnfo));

    strcpy((char *)&g_dev_info.Version[0], "1.0.0.0_20260114");
    strcpy((char *)&g_dev_info.DeviceName[0], Name_Mini_SliceMicro);
    strcpy((char *)&g_dev_info.AccessCode[0], "NTS2026");
    strcpy((char *)&g_dev_info.SerialNumber[0], "MISMB102501001");
    g_dev_info.DeviceType = Mini_SliceMicro;
    // 获取base板设备信息
    if (g_IdaSystemStatus.st_dev_link.link_status == USB_CONNECTED)
    {
        g_dev_info.IsConnected = 1;
    }
    else
    {
        g_dev_info.IsConnected = 0;
    }
    g_dev_info.SubDeviceNum = subden_num;
    memcpy(send_frame, &g_dev_info, sizeof(DeviceInfo));

    // 计算数据长度
    send_len = sizeof(DeviceInfo) + sizeof(SubDevicelnfo) * subden_num;

    // 发送数据
    FrameHeadInfo frame_head = create_default_frame_head(serial_num);
    UserDataHeadInfo user_head = create_user_data_head(DVS_INIT_DETECT,
                                                       SOURCE_TYPE_WITH_DATAS,
                                                       DESTINATION_ARM_TO_PC,
                                                       send_len);
    uint32_t packet_len = 0;
    pack_data(send_frame, send_len, &user_head, &frame_head, &packet_len);
    return packet_len;
}
// 处理PC->ARM的DVS_INIT_CONNECT事件
static uint32_t USB_Connect_Reply(uint8_t *data_in, uint32_t data_len)
{

    g_IdaSystemStatus.st_dev_link.link_status = USB_CONNECTED;

    return PackReplyWithoutDatas(DVS_INIT_CONNECT_OK);
}

// 处理PC->ARM的DVS_INIT_DISCONNECT事件
static uint32_t USB_Disconnect_Reply(uint8_t *data_in, uint32_t data_len)
{
    g_IdaSystemStatus.st_dev_link.link_status = USB_DISCONNECTED;

    return PackReplyWithoutDatas(DVS_INIT_DISCONNECT_OK);
}

// 处理PC->ARM的DVS_INIT_DEVCONFIG_UPDATE_Done事件
static uint32_t USB_DevConfig_Done(uint8_t *data_in, uint32_t data_len)
{
    if (data_len != sizeof(DeviceDetailInfo))
    {
        return PackReplyWithoutDatas(DVS_INIT_DEVCONFIG_UPDATE_Done_OK);
    }
    DeviceDetailInfo *dev_detail = (DeviceDetailInfo *)data_in;
    DeviceInfo dev_info;
    // 拷贝对应字段
    strcpy((char *)g_dev_info.Version, (const char *)dev_detail->SoftwareVersion);
    strcpy((char *)g_dev_info.DeviceName, (const char *)dev_detail->DeviceName);
    strcpy((char *)g_dev_info.AccessCode, (const char *)dev_detail->AccessCode);
    strcpy((char *)g_dev_info.SerialNumber, (const char *)dev_detail->SerialNumber);
    g_dev_info.DeviceType = dev_detail->DeviceType;

    // 更新设备信息文件
    write_device_info_file(dev_info);

    return PackReplyWithoutDatas(DVS_INIT_DEVCONFIG_UPDATE_Done_OK);
}

// 处理PC->ARM的DVSARM_CSP_START事件
static uint32_t USB_CollectChCfg_Reply(uint8_t *data_in, uint32_t data_len)
{
    ChannelTableHeader *channel_header;
    DSAGlobalParams *global_params;
    channel_header = (ChannelTableHeader *)data_in;
    global_params = (DSAGlobalParams *)(data_in + channel_header->nTotalChannelNum * sizeof(DSAGlobalParams));
    uint32_t sample_rate = 0;
    for (uint8_t i = 0; i < (int)(sizeof(g_ida_ch_rate) / sizeof(g_ida_ch_rate[0])); i++)
    {
        if (channel_header->fHardwareSampleRate == g_ida_ch_rate[i].ch_cfg_value)
        {
            sample_rate = g_ida_ch_rate[i].ch_cfg_value;
            break;
        }
    }
    //    sample_rate = 1000;
    usb_printf("sample_rate:%d", sample_rate);

    // 动态配置通道数（假设均匀分配到3路SPI）
    uint8_t total_channels = channel_header->nTotalChannelNum;
    uint8_t ch_per_spi = total_channels / SPI_NUM;
    
    usb_printf("Total channels: %d, channels per SPI: %d\r\n", total_channels, ch_per_spi);
    
    if (ch_per_spi > 0 && ch_per_spi <= SPI_CH_ADC_MAX_HW && (total_channels % SPI_NUM) == 0)
    {
        // 停止当前采集
        AdcCollectorContrl(0);
        
        // 配置通道和采样率
        AdcChanCfgSet(ch_per_spi, sample_rate);
        
        // 启动采集
        g_IdaSystemStatus.st_dev_run.run_flag = 1;
        AdcCollectorContrl(g_IdaSystemStatus.st_dev_run.run_flag);
    }
    else
    {
        usb_printf("Invalid channel configuration: total channels must be multiple of %d and per SPI <= %d\r\n", SPI_NUM, SPI_CH_ADC_MAX_HW);
        // 不启动采集，返回OK但状态为未运行
        g_IdaSystemStatus.st_dev_run.run_flag = 0;
    }

    return PackReplyWithoutDatas(DVSARM_CSP_START_OK);
}
// 处理PC->ARM的DVSARM_RUN事件
static uint32_t USB_Run_Reply(uint8_t *data_in, uint32_t data_len)
{
    return PackReplyWithoutDatas(DVSARM_RUN_OK);
}
// 处理PC->ARM的DVSARM_STOP事件
static uint32_t USB_Stop_Reply(uint8_t *data_in, uint32_t data_len)
{
    uint32_t packet_len = 0;

    g_IdaSystemStatus.st_dev_run.run_flag = 0;
    AdcCollectorContrl(g_IdaSystemStatus.st_dev_run.run_flag);
    cb_clear(g_cb_adc);

    return PackReplyWithoutDatas(DVSARM_STOP_OK);
}
// 处理PC->ARM的DVS_CSP_OFFLINE_SETCONFIG_OK事件
static uint32_t USB_OfflineCfg_Reply(uint8_t *data_in, uint32_t data_len)
{
    int32_t ret = RET_OK;
    if (data_len == 0)
    {
        ret = clear_file_content(OFFLINE_SCHEDULE_FILE);
    }
    else
    {
        ChannelTableHeader *ChCfg_header = (ChannelTableHeader *)data_in;
        ChannelTableElem *ChCfg[SPI_NUM];
        DSAGlobalParams *Global_Param = (DSAGlobalParams *)(data_in + sizeof(ChannelTableHeader) + ChCfg_header->nTotalChannelNum * sizeof(ChannelTableElem));

        if (ChCfg_header->nTotalChannelNum == SPI_NUM)
        {
            for (uint8_t i = 0; i < ChCfg_header->nTotalChannelNum; i++)
            {
                ChCfg[i] = (ChannelTableElem *)(data_in + sizeof(ChannelTableHeader) + i * sizeof(ChannelTableElem));
            }
            if ((ChCfg[0]->nChannelID == 0) && (ChCfg[1]->nChannelID == 1) && (ChCfg[2]->nChannelID == 2))
            {
                DSAGlobalParams *Global_Param = (DSAGlobalParams *)(data_in + sizeof(ChannelTableHeader) + ChCfg_header->nTotalChannelNum * sizeof(ChannelTableElem));
                if (Global_Param->nScheduleCount <= 0)
                {
                    ret = -1;
                }
            }
            else
            {
                ret = -1;
            }
        }
        else
        {
            ret = -1;
        }

        if (ret == RET_OK)
        {

            FIL fil;
            UINT bw, br;
            ret = f_open(&fil, OFFLINE_SCHEDULE_FILE, FA_CREATE_ALWAYS | FA_WRITE);
            if (ret == FR_OK)
            {
                ret = f_write(&fil, data_in, data_len, &bw);

                usb_printf("0:/OfflineCfgSchedule.txt f_write :%d", ret);

                f_close(&fil);
            }
            else
            {
                usb_printf("0:/OfflineCfgSchedule.txt f_open error :%d", ret);
            }
        }
    }

    ret = (ret != RET_OK) ? -1 : RET_OK;
    // 返回应答数据
    FrameHeadInfo frame_head = create_default_frame_head(0);
    UserDataHeadInfo user_head = {0};
    user_head.nIsValidFlag = 0x12345678;
    user_head.nEventID = DVS_CSP_OFFLINE_SETCONFIG_OK;
    user_head.nSourceType = SOURCE_TYPE_NO_DATA;
    user_head.nDestinationID = DESTINATION_ARM_TO_PC;
    user_head.nDataLength = 0;
    user_head.nNanoSecond = dwt_get_ns();
    user_head.nParameters0 = ret;

    uint32_t packet_len = 0;
    pack_data(NULL, 0, &user_head, &frame_head, &packet_len);
    return packet_len;
}
// 处理PC->ARM的DVS_INIT_ARM_UPGRADE_REQ_OK事件
static uint32_t USB_Upgrad_Reply(uint8_t *data_in, uint32_t data_len)
{
    FrameHeadInfo frame_head = create_default_frame_head(0);
    UserDataHeadInfo user_head = {0};
    user_head.nIsValidFlag = 0x12345678;
    user_head.nEventID = DVS_INIT_ARM_UPGRADE_REQ_OK;
    user_head.nSourceType = SOURCE_TYPE_NO_DATA;
    user_head.nDestinationID = DESTINATION_ARM_TO_PC;
    user_head.nDataLength = 0;
    user_head.nNanoSecond = dwt_get_ns();
    user_head.nParameters0 = 1;

    uint32_t packet_len = 0;
    pack_data(NULL, 0, &user_head, &frame_head, &packet_len);
    g_IdaSystemStatus.st_dev_mode.reset_all_flag = 1; // ready to restart
    g_reset_time = dwt_get_ns();
    return packet_len;
}

void transpose(short src[BLOCK_LEN][SPI_NUM], short dst[SPI_NUM][BLOCK_LEN])
{
    for (int i = 0; i < BLOCK_LEN; i++)
    {
        dst[0][i] = src[i][0];
        dst[1][i] = src[i][1];
        dst[2][i] = src[i][2];
    }
}
// 处理PC->ARM的DVSARM_DISPNEXT_OK事件
static uint32_t USB_Display_Reply(uint8_t *data_in, uint32_t data_len)
{
    static uint32_t frame_num = 0;

    /* determine current channel configuration */
    uint8_t ch_per_spi = g_adc_chan_cfg.ch_per_spi;
    uint8_t total_ch   = g_adc_chan_cfg.total_ch;           /* =SPI_NUM*ch_per_spi */
    uint32_t cb_len    = (uint32_t)total_ch * ADC_DATA_LEN * BLOCK_LEN;

    /* wait until a full block of samples is available */
    if (cb_size(g_cb_adc) < cb_len)
    {
        return 0; /* nothing to send */
    }

    frame_num++;

    struct UserData user_data;
    user_data.data_head.nVersion       = 0x12345678;
    user_data.data_head.nDataSource    = 0;
    user_data.data_head.nFrameChCount  = total_ch;          /* dynamic channel count */
    user_data.data_head.nFrameLen      = BLOCK_LEN;
    user_data.data_head.nTotalFrameNum = frame_num;
    user_data.data_head.nCurNs         = dwt_get_ns();

    /* read raw samples out of circular buffer; layout is
       [ch0, ch1, ..., chN-1] per time step, repeated BLOCK_LEN times */
    /* we can read directly into the send_frame array since its first
       dimension is large enough for max channels */
    cb_read(g_cb_adc, (char *)user_data.send_frame, cb_len);

    /* note: internal representation is already channel-major (ch,index)
       so no need to transpose; just ensure we don't send unused rows */

    uint32_t send_len = sizeof(ArmBackFrameHeader)
                        + total_ch * BLOCK_LEN * sizeof(short);

    FrameHeadInfo frame_head = create_default_frame_head(frame_num);
    UserDataHeadInfo user_head = create_user_data_head(DVSARM_DISPNEXT_OK,
                                                       SOURCE_TYPE_WITH_DATAS,
                                                       DESTINATION_ARM_TO_PC,
                                                       send_len);
    uint32_t packet_len = 0;
    pack_data((uint8_t *)&user_data, send_len, &user_head, &frame_head, &packet_len);

    return packet_len;
}
// 处理PC->ARM的DVSARM_DISPNEXT_OK事件，循环发送数据
void USB_Display_All(uint32_t run_flag)
{
    if (run_flag)
    {
        USB_Display_Reply(NULL, 0);
    }
}
// 处理PC->ARM的DVS_FILE_GET_FILELIST_OK事件
static uint32_t USB_GetFilelist(uint8_t *data_in, uint32_t data_len)
{
    uint8_t user_data[2048];
    uint32_t send_len = 0;

    /* todo : get_file_list_by_extension*/
    FileList_t file_list;
    int8_t ret;
    char date_time_buf[20];

    // 获取0:/data目录下的.txt文件列表，最多10个
    ret = get_file_list_by_extension("0:/data", ".txt", &file_list, 10);

    if (ret == 0)
    {
        printf("Found %d files:\n", file_list.count);

        for (uint32_t i = 0; i < file_list.count; i++)
        {
            // 格式化日期时间
            format_date_time(file_list.files[i].create_date,
                             file_list.files[i].create_time,
                             date_time_buf, sizeof(date_time_buf));

            // 打印文件信息
            printf("File %d:\n", i + 1);
            printf("  Path: %s\n", file_list.files[i].path);
            printf("  Size: %u bytes\n", file_list.files[i].size);
            printf("  Created: %s\n", date_time_buf);
            printf("\n");

            strcat((char *)user_data, file_list.files[i].path);
            strcat((char *)user_data, ",");
            char str[8];
            sprintf(str, "%u", file_list.files[i].size);
            strcat((char *)user_data, str);
            strcat((char *)user_data, ",");
            strcat((char *)user_data, date_time_buf);
            strcat((char *)user_data, "|");
        }

        // 释放文件列表
        free_file_list(&file_list);
    }
    else
    {
        printf("Failed to get file list: %d\n", ret);
    }

    send_len = strlen((const char *)user_data);

    // 鍒涘缓涓€涓抚澶?
    FrameHeadInfo frame_head = create_default_frame_head(0);
    // 鍒涘缓涓€涓敤鎴锋暟鎹ご
    UserDataHeadInfo user_head = create_user_data_head(DVS_FILE_GET_FILELIST_OK,
                                                       SOURCE_TYPE_WITH_DATAS,
                                                       DESTINATION_ARM_TO_PC,
                                                       send_len);

    uint32_t packet_len = 0;
    pack_data((uint8_t *)&user_data, send_len, &user_head, &frame_head, &packet_len);

    return packet_len;
}
// 处理PC->ARM的DVS_FILE_DELETE_OK事件
static uint32_t USB_DeleteFile(uint8_t *data_in, uint32_t data_len)
{
    /*todo : 删除文件*/

    return PackReplyWithoutDatas(DVS_FILE_DELETE_OK);
}

// 处理PC->ARM的DVS_FILE_DOWNLOAD_STOP_OK事件
static uint32_t USB_DownloadFileStop(uint8_t *data_in, uint32_t data_len);

/* ==========================================================================
 * 文件下载（eMMC -> MCU -> PC）实现
 *
 * 协议流程（ACK握手模式，保证可靠传输）：
 *   PC  --[DVS_FILE_DOWNLOAD_START    + FileDownloadStartReq  ]--> MCU
 *   MCU --[DVS_FILE_DOWNLOAD_START_OK + FileDownloadStartReply]--> PC
 *   MCU --[DVS_FILE_DOWNLOAD_DATA     + FileDownloadDataPack  ]--> PC  (第0包)
 *   PC  --[DVS_FILE_DOWNLOAD_DATA_ACK + FileDownloadDataAck   ]--> MCU (确认第0包)
 *   MCU --[DVS_FILE_DOWNLOAD_DATA     + FileDownloadDataPack  ]--> PC  (第1包)
 *   ...
 *   PC  --[DVS_FILE_DOWNLOAD_STOP     + FileDownloadStopReq   ]--> MCU (全部收完后)
 *   MCU --[DVS_FILE_DOWNLOAD_STOP_OK  + FileDownloadStopReply ]--> PC
 * ========================================================================== */

#define DOWNLOAD_PACK_SIZE_DEFAULT  (4096U)   /* 默认每包字节数 */
#define DOWNLOAD_PACK_SIZE_MAX      (8192U)   /* 最大每包字节数 */

/* 下载会话状态 */
typedef struct
{
    uint8_t   active;               /* 0=空闲, 1=下载中, 2=等待ACK */
    uint32_t  session_id;
    uint32_t  total_size;
    uint32_t  total_packs;
    uint32_t  pack_size;            /* 实际每包大小 */
    uint32_t  next_pack_index;      /* 下一个要发送的包序号 */
    uint32_t  bytes_sent;           /* 已发送字节数 */
    FIL       fil;
    char      file_path[256];
} FileDownloadSession;

static FileDownloadSession g_download_session = {0};

/* 读文件缓冲（静态，4字节对齐，避免 IDMA 问题） */
static uint8_t g_download_buf[DOWNLOAD_PACK_SIZE_MAX] __attribute__((aligned(4)));

/* CRC32（标准多项式 0xEDB88320） */
static uint32_t download_crc32(const uint8_t *buf, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    for (uint32_t i = 0; i < len; i++)
    {
        crc ^= buf[i];
        for (int j = 0; j < 8; j++)
            crc = (crc >> 1) ^ (0xEDB88320U & (-(int32_t)(crc & 1)));
    }
    return ~crc;
}

/* 发送带数据的帧 */
static uint32_t download_send_with_data(uint32_t event_id,
                                        const void *payload,
                                        uint32_t payload_len)
{
    FrameHeadInfo frame_head = create_default_frame_head(0);
    UserDataHeadInfo user_head = create_user_data_head(
        event_id,
        SOURCE_TYPE_WITH_DATAS,
        DESTINATION_ARM_TO_PC,
        payload_len);
    uint32_t packet_len = 0;
    pack_data((const uint8_t *)payload, payload_len,
              &user_head, &frame_head, &packet_len);
    return packet_len;
}

/* 发送下一个数据包（内部调用） */
static void download_send_next_pack(void)
{
    if (!g_download_session.active)
        return;

    UINT br = 0;
    FRESULT fres = f_read(&g_download_session.fil,
                          g_download_buf,
                          g_download_session.pack_size,
                          &br);
    if (fres != FR_OK || br == 0)
    {
        usb_printf("[Download] f_read failed: res=%d br=%u\r\n", fres, br);
        /* 读取出错，发送 STOP */
        FileDownloadStopReply stop_reply = {0};
        stop_reply.session_id  = g_download_session.session_id;
        stop_reply.result      = -1;
        stop_reply.bytes_sent  = g_download_session.bytes_sent;
        download_send_with_data(DVS_FILE_DOWNLOAD_STOP_OK,
                                &stop_reply, sizeof(stop_reply));
        f_close(&g_download_session.fil);
        memset(&g_download_session, 0, sizeof(g_download_session));
        return;
    }

    /* 构造数据包头部（不含柔性数组） */
    FileDownloadDataPack pack_head;
    pack_head.session_id    = g_download_session.session_id;
    pack_head.pack_index    = g_download_session.next_pack_index;
    pack_head.total_packs   = g_download_session.total_packs;
    pack_head.pack_data_len = br;
    pack_head.crc32         = download_crc32(g_download_buf, br);

    /* 组合 pack_head + data 发送（两段发送：先头部再数据）
     * 由于柔性数组不方便整体拷贝，这里把头和数据合并到一个临时缓冲发送 */
    static uint8_t tx_buf[sizeof(FileDownloadDataPack) + DOWNLOAD_PACK_SIZE_MAX]
        __attribute__((aligned(4)));
    memcpy(tx_buf, &pack_head, sizeof(FileDownloadDataPack) - 1); /* -1 去掉data[1] */
    memcpy(tx_buf + sizeof(FileDownloadDataPack) - 1, g_download_buf, br);
    uint32_t tx_len = (sizeof(FileDownloadDataPack) - 1) + br;

    download_send_with_data(DVS_FILE_DOWNLOAD_DATA, tx_buf, tx_len);

    g_download_session.bytes_sent      += br;
    g_download_session.active           = 2; /* 等待 ACK */

    usb_printf("[Download] Pack %lu/%lu sent, bytes=%u, total_sent=%lu\r\n",
               g_download_session.next_pack_index + 1,
               g_download_session.total_packs,
               br,
               g_download_session.bytes_sent);
}

/* --------------------------------------------------------------------------
 * DVS_FILE_DOWNLOAD_START 处理：PC 发起下载请求
 * -------------------------------------------------------------------------- */
static uint32_t USB_DownloadFileStart(uint8_t *data_in, uint32_t data_len)
{
    FileDownloadStartReply reply = {0};

    if (g_download_session.active)
    {
        usb_printf("[Download] Session already active\r\n");
        reply.result = -1;
        snprintf(reply.message, sizeof(reply.message), "Session busy");
        return download_send_with_data(DVS_FILE_DOWNLOAD_START_OK,
                                       &reply, sizeof(reply));
    }

    if (data_len < sizeof(FileDownloadStartReq))
    {
        usb_printf("[Download] START payload too short\r\n");
        reply.result = -2;
        snprintf(reply.message, sizeof(reply.message), "Bad request");
        return download_send_with_data(DVS_FILE_DOWNLOAD_START_OK,
                                       &reply, sizeof(reply));
    }

    FileDownloadStartReq *req = (FileDownloadStartReq *)data_in;
    req->file_path[sizeof(req->file_path) - 1] = '\0';

    /* 打开文件 */
    FRESULT fres = f_open(&g_download_session.fil,
                          req->file_path,
                          FA_OPEN_EXISTING | FA_READ);
    if (fres != FR_OK)
    {
        usb_printf("[Download] f_open failed: path=%s res=%d\r\n",
                   req->file_path, fres);
        reply.result = -3;
        snprintf(reply.message, sizeof(reply.message), "Open failed(%d)", fres);
        return download_send_with_data(DVS_FILE_DOWNLOAD_START_OK,
                                       &reply, sizeof(reply));
    }

    uint32_t file_size  = f_size(&g_download_session.fil);
    uint32_t pack_size  = (req->pack_size > 0 && req->pack_size <= DOWNLOAD_PACK_SIZE_MAX)
                          ? req->pack_size : DOWNLOAD_PACK_SIZE_DEFAULT;
    uint32_t total_packs = (file_size + pack_size - 1) / pack_size;
    if (total_packs == 0) total_packs = 1;

    /* 初始化会话 */
    g_download_session.active          = 1;
    g_download_session.session_id      = HAL_GetTick();
    g_download_session.total_size      = file_size;
    g_download_session.total_packs     = total_packs;
    g_download_session.pack_size       = pack_size;
    g_download_session.next_pack_index = 0;
    g_download_session.bytes_sent      = 0;
    strncpy(g_download_session.file_path, req->file_path,
            sizeof(g_download_session.file_path) - 1);

    usb_printf("[Download] Session %lu started: path=%s size=%lu packs=%lu\r\n",
               g_download_session.session_id,
               g_download_session.file_path,
               file_size, total_packs);

    /* 回复 START_OK */
    reply.result      = 0;
    reply.session_id  = g_download_session.session_id;
    reply.total_size  = file_size;
    reply.total_packs = total_packs;
    reply.pack_size   = pack_size;
    snprintf(reply.message, sizeof(reply.message), "Ready");
    download_send_with_data(DVS_FILE_DOWNLOAD_START_OK, &reply, sizeof(reply));

    /* 立即发送第一包 */
    download_send_next_pack();

    return 0;
}

/* --------------------------------------------------------------------------
 * DVS_FILE_DOWNLOAD_DATA_ACK 处理：PC 确认收到一包，MCU 发下一包
 * -------------------------------------------------------------------------- */
static uint32_t USB_DownloadFileDataAck(uint8_t *data_in, uint32_t data_len)
{
    if (!g_download_session.active)
    {
        usb_printf("[Download] ACK received but no active session\r\n");
        return 0;
    }

    if (data_len < sizeof(FileDownloadDataAck))
    {
        usb_printf("[Download] ACK payload too short\r\n");
        return 0;
    }

    FileDownloadDataAck *ack = (FileDownloadDataAck *)data_in;

    if (ack->session_id != g_download_session.session_id)
    {
        usb_printf("[Download] ACK session_id mismatch\r\n");
        return 0;
    }

    if (ack->result != 0)
    {
        /* PC 要求重传：回退文件指针 */
        usb_printf("[Download] PC requested retransmit pack %lu\r\n", ack->pack_index);
        uint32_t seek_offset = ack->pack_index * g_download_session.pack_size;
        f_lseek(&g_download_session.fil, seek_offset);
        g_download_session.bytes_sent      = seek_offset;
        g_download_session.next_pack_index = ack->pack_index;
        g_download_session.active          = 1;
        download_send_next_pack();
        return 0;
    }

    /* 确认成功，推进包序号 */
    g_download_session.next_pack_index = ack->pack_index + 1;
    g_download_session.active          = 1;

    if (g_download_session.next_pack_index >= g_download_session.total_packs)
    {
        /* 所有包已确认，等待 PC 发 STOP */
        usb_printf("[Download] All packs ACKed, waiting for STOP\r\n");
        return 0;
    }

    /* 发送下一包 */
    download_send_next_pack();
    return 0;
}

/* --------------------------------------------------------------------------
 * DVS_FILE_DOWNLOAD_STOP 处理：PC 通知结束（正常完成或取消）
 * -------------------------------------------------------------------------- */
static uint32_t USB_DownloadFileStop(uint8_t *data_in, uint32_t data_len)
{
    FileDownloadStopReply reply = {0};

    if (!g_download_session.active)
    {
        reply.result = 0;
        return download_send_with_data(DVS_FILE_DOWNLOAD_STOP_OK,
                                       &reply, sizeof(reply));
    }

    reply.session_id  = g_download_session.session_id;
    reply.bytes_sent  = g_download_session.bytes_sent;
    reply.result      = 0;

    uint8_t do_abort = 0;
    if (data_len >= sizeof(FileDownloadStopReq))
    {
        FileDownloadStopReq *req = (FileDownloadStopReq *)data_in;
        do_abort = req->abort;
    }

    f_close(&g_download_session.fil);

    usb_printf("[Download] Session %lu %s: sent=%lu bytes\r\n",
               g_download_session.session_id,
               do_abort ? "aborted" : "done",
               g_download_session.bytes_sent);

    memset(&g_download_session, 0, sizeof(g_download_session));

    return download_send_with_data(DVS_FILE_DOWNLOAD_STOP_OK,
                                   &reply, sizeof(reply));
}

uint64_t heart_recv_time = 0;
// 处理PC->ARM的事件
void on_frame(const uint8_t *frame, uint32_t frame_len)
{

    usb_printf("on_frame: %d\n", frame_len);

    // 解包
    uint8_t *unpacked_data = NULL;
    UserDataHeadInfo unpacked_head;
    FrameHeadInfo unpacked_frame_head;
    uint32_t unpacked_data_len = 0;

    int result = unpack_data(frame, frame_len, &unpacked_data,
                             &unpacked_head, &unpacked_frame_head,
                             &unpacked_data_len);

    if (result == 0)
    {
        heart_recv_time = dwt_get_us();
        for (uint8_t i = 0; i < sizeof(protocol_getdata) / sizeof(protocol_getdata[0]); i++)
        {
            if (unpacked_head.nEventID == protocol_getdata[i].usb_frame_tag)
            {
                uint32_t reply_len = 0;
                reply_len = protocol_getdata[i].usb_process_handle(unpacked_data, unpacked_data_len);
            }
        }

        //        if (unpacked_data) {
        //            printf("User data: %s\n", unpacked_data);
        //            free_unpacked_user_data(unpacked_data);
        //        }
    }
    else
    {
        printf("Unpack failed with error code: %d\n", result);
    }
}
// 处理ARM->PC的事件
void IdaProcessor(void)
{
    static uint64_t time_l;
    static uint64_t heart_time_l;
    uint64_t time_n = 0;
    int32_t time_off = 0;
    uint32_t time_out = 0;

    time_out = (g_IdaSystemStatus.st_dev_link.link_status == USB_CONNECTED) ? TIMEOUT_20000 : TIMEOUT_5000;
    time_n = dwt_get_us();
    time_off = (time_n - time_l) / 1000;
    if (time_off > time_out)
    {
        memcpy(&time_l, &time_n, sizeof(uint64_t));
        uint32_t data_len = 0;
        data_len = USB_DetectPeriod();
        if (data_len == 0)
        {
            // printf("USB_DetectPeriod err.\n");
        }
    }
    if (g_IdaSystemStatus.st_dev_link.link_status == USB_CONNECTED)
    {
        time_off = (time_n - heart_time_l) / 1000;
        if (time_off > TIMEOUT_5000)
        {
            memcpy(&heart_time_l, &time_n, sizeof(uint64_t));
            PackReplyWithoutDatas(DVS_INIT_CONNECT_DETECT_OK);
        }
    }
    if ((g_IdaSystemStatus.st_dev_link.link_status != USB_IDLE) &&
        (g_IdaSystemStatus.st_dev_link.link_status != USB_DISCONNECTED))
    {
        time_off = (time_n - heart_recv_time) / 1000;
        if (time_off >= TIMEOUT_30000)
        {
            // disconnect

            SystemStatusInit();
            AdcCollectorContrl(g_IdaSystemStatus.st_dev_run.run_flag);
            cb_clear(g_cb_adc);
        }
    }
    if (g_IdaSystemStatus.st_dev_mode.reset_all_flag == 1)
    {
        time_off = (time_n - g_reset_time) / 1000;
        if (time_off > TIMEOUT_1000)
        {
            g_reset_time = 0;
            g_IdaSystemStatus.st_dev_mode.reset_all_flag = 0;
            NVIC_SystemReset();
        }
    }
}

FRESULT clear_file_content(const char *path)
{
    FRESULT res;
    res = f_unlink(path);
    res = ((res == FR_NO_FILE) || (res == FR_OK)) ? FR_OK : res;
    return res;
}

SendRecordPack g_SendRecordFilesPackHead;
#define PACK_DATA_LEN (1024)
int8_t SendRecordFiles(const char *file_name)
{
    int8_t ret = RET_OK;
    static FIL *fp = NULL;
    UINT bw, br;
    uint32_t total_size = 0;
    uint32_t offset = 0;

    if ((g_SendRecordFilesPackHead.head.totalPackNum == 0) && (fp == NULL))
    {

        ret = f_open(fp, file_name, FA_OPEN_EXISTING);
        total_size = f_size(fp);
        g_SendRecordFilesPackHead.head.totalPackNum = (total_size + PACK_DATA_LEN - 1) / PACK_DATA_LEN;
    }
    if (g_SendRecordFilesPackHead.head.totalPackNum > g_SendRecordFilesPackHead.head.currentPackIdx)
    {
        // todo: get bytes from file
        offset = g_SendRecordFilesPackHead.head.currentPackAddr;
        ret = f_lseek(fp, offset);
        ret = f_read(fp, g_SendRecordFilesPackHead.data, PACK_DATA_LEN, &br);

        g_SendRecordFilesPackHead.head.currentPackAddr = offset;
        g_SendRecordFilesPackHead.head.currentPackIdx++;
        g_SendRecordFilesPackHead.head.packSize = PACK_DATA_LEN;
        g_SendRecordFilesPackHead.head.crc32 = make_crc32(g_SendRecordFilesPackHead.data, br);

        if (g_SendRecordFilesPackHead.head.totalPackNum == g_SendRecordFilesPackHead.head.currentPackIdx)
        {
            if (fp != NULL)
            {
                f_close(fp);
                fp = NULL;
            }
        }

        FrameHeadInfo frame_head = create_default_frame_head(0);
        UserDataHeadInfo user_head = {0};
        user_head.nIsValidFlag = 0x12345678;
        user_head.nEventID = DVS_INIT_ARM_UPGRADE_REQ_OK;
        user_head.nSourceType = SOURCE_TYPE_WITH_DATAS;
        user_head.nDestinationID = DESTINATION_ARM_TO_PC;
        user_head.nDataLength = sizeof(SendRecordPackHead) + br;
        user_head.nNanoSecond = dwt_get_ns();
        user_head.nParameters0 = 0;

        uint32_t packet_len = 0;
        pack_data((const uint8_t *)&g_SendRecordFilesPackHead.head, sizeof(SendRecordPackHead) + br, &user_head, &frame_head, &packet_len);

        return packet_len;
    }
    else
    {
        return 0;
    }
}

/* read device info file */
int read_device_info_file(DeviceInfo *dev_info_data)
{
    int ret = 0;
    // int tmp_value = 0;
    char tmp_str[KEY_VALUE_MAX_LEN] = {0};
    int tmp_int = 0;
    float tmp_f = 0.0;
    ret = LoadConfFile(IDA_DEVICE_INFO_FILE);
    if (ret)
    {
        usb_printf("load config file [%s] fail\n", IDA_DEVICE_INFO_FILE);
        return RET_ERROR;
    }
    else
    {
        /* get device infomation */
        // Version
        if (INI_RET_OK != GetSecStr(INI_SECTION_DEVICE_INFO, INI_KEY_SoftwareVersion, tmp_str))
        {
            usb_printf("[ Error ]get SoftwareVersion [%s] from %s faild.", tmp_str, IDA_DEVICE_INFO_FILE);
            return RET_ERROR;
        }
        strncpy((char *)dev_info_data->Version, tmp_str, KEY_VALUE_MAX_LEN);
        usb_printf("get Version [%s] from config file", dev_info_data->Version);
        // AccessCode
        if (INI_RET_OK != GetSecStr(INI_SECTION_DEVICE_INFO, INI_KEY_AccessCode, tmp_str))
        {
            usb_printf("[ Error ]get AccessCode [%s] from %s faild.", tmp_str, IDA_DEVICE_INFO_FILE);
            return RET_ERROR;
        }
        strncpy((char *)dev_info_data->AccessCode, tmp_str, KEY_VALUE_MAX_LEN);
        usb_printf("get AccessCode [%s] from config file", dev_info_data->AccessCode);
        // ID
        if (INI_RET_OK != GetSecInt(INI_SECTION_DEVICE_INFO, INI_KEY_ID, &tmp_int))
        {
            usb_printf("[ Error ]get ID [%d] from %s faild.", tmp_int, IDA_DEVICE_INFO_FILE);
            return RET_ERROR;
        }
        dev_info_data->ID = tmp_int;
        usb_printf("get ID [%d] from config file", dev_info_data->ID);
        // SerialNumber
        if (INI_RET_OK != GetSecStr(INI_SECTION_DEVICE_INFO, INI_KEY_SerialNumber, tmp_str))
        {
            usb_printf("[ Error ]get SerialNumber [%s] from %s faild.", tmp_str, IDA_DEVICE_INFO_FILE);
            return RET_ERROR;
        }
        strcpy((char *)dev_info_data->SerialNumber, tmp_str);
        usb_printf("get SerialNumber [%s] from config file", dev_info_data->SerialNumber);
        // DeviceName
        if (INI_RET_OK != GetSecStr(INI_SECTION_DEVICE_INFO, INI_KEY_DeviceName, tmp_str))
        {
            usb_printf("[ Error ]get DeviceName [%s] from %s faild.", tmp_str, IDA_DEVICE_INFO_FILE);
            return RET_ERROR;
        }
        strcpy((char *)dev_info_data->DeviceName, tmp_str);
        usb_printf("get DeviceName messages [%s] from config file", dev_info_data->DeviceName);
        // DeviceType
        if (INI_RET_OK != GetSecInt(INI_SECTION_DEVICE_INFO, INI_KEY_DeviceType, &tmp_int))
        {
            usb_printf("[ Error ]get DeviceType [%d] from %s faild.", tmp_int, IDA_DEVICE_INFO_FILE);
            return RET_ERROR;
        }
        dev_info_data->DeviceType = tmp_int;
        usb_printf("get DeviceType [%d] from config file", dev_info_data->DeviceType);
        // LastUpdateDateTime
        if (INI_RET_OK != GetSecStr(INI_SECTION_DEVICE_INFO, INI_KEY_LastUpdateDateTime, tmp_str))
        {
            usb_printf("[ Error ]get LastUpdateDateTime [%s] from %s faild.", tmp_str, IDA_DEVICE_INFO_FILE);
            return RET_ERROR;
        }
        strcpy((char *)dev_info_data->LastUpdateDateTime, tmp_str);
        usb_printf("get LastUpdateDateTime [%s] from config file", dev_info_data->LastUpdateDateTime);
    }
    usb_printf("get device information OK!", NULL);
    return RET_OK;
}
/* write device_info_file */
int write_device_info_file(DeviceInfo dev_data)
{
    int i = 0, ret = 0;
    int tmp_int = 0;
    float tmp_f = 0.0;

    ret = LoadConfFile(IDA_DEVICE_INFO_FILE);
    if (ret)
    {
        usb_printf("load config file [%s] fail when update ad config paramaters", IDA_DEVICE_INFO_FILE);
        return RET_ERROR;
    }
    else
    {
        /* write ida device infomation */
        // Version
        if (INI_RET_OK != SetSecStr(INI_SECTION_DEVICE_INFO, INI_KEY_SoftwareVersion, (char *)dev_data.Version))
        {
            usb_printf("update [ %s ] failed! [Version = %s]", IDA_DEVICE_INFO_FILE, dev_data.Version);
            i |= 0x01;
        }
        // AccessCode
        if (INI_RET_OK != SetSecStr(INI_SECTION_DEVICE_INFO, INI_KEY_AccessCode, (char *)dev_data.AccessCode))
        {
            usb_printf("update [ %s ] failed! [AccessCode = %s]", IDA_DEVICE_INFO_FILE, dev_data.AccessCode);
            i |= 0x01;
        }
        // ID
        tmp_int = dev_data.ID;
        if (INI_RET_OK != SetSecInt(INI_SECTION_DEVICE_INFO, INI_KEY_ID, &tmp_int))
        {
            usb_printf("update [ %s ] failed! [ID = %d]", IDA_DEVICE_INFO_FILE, tmp_int);
            i |= 0x02;
        }
        // SerialNumber
        if (INI_RET_OK != SetSecStr(INI_SECTION_DEVICE_INFO, INI_KEY_SerialNumber, (char *)dev_data.SerialNumber))
        {
            usb_printf("update [ %s ] failed! [SerialNumber = %s]", IDA_DEVICE_INFO_FILE, dev_data.SerialNumber);
            i |= 0x04;
        }
        // DeviceName
        if (INI_RET_OK != SetSecStr(INI_SECTION_DEVICE_INFO, INI_KEY_DeviceName, (char *)dev_data.DeviceName))
        {
            usb_printf("update [ %s ] failed! [DeviceName = %s]", IDA_DEVICE_INFO_FILE, dev_data.DeviceName);
            i |= 0x08;
        }
        // DeviceType
        tmp_int = dev_data.DeviceType;
        if (INI_RET_OK != SetSecInt(INI_SECTION_DEVICE_INFO, INI_KEY_DeviceType, &tmp_int))
        {
            usb_printf("update [ %s ] failed! [DeviceType = %d]", IDA_DEVICE_INFO_FILE, tmp_int);
            i |= 0x10;
        }
        // LastUpdateDateTime
        if (INI_RET_OK != SetSecStr(INI_SECTION_DEVICE_INFO, INI_KEY_LastUpdateDateTime, (char *)dev_data.LastUpdateDateTime))
        {
            usb_printf("update [ %s ] failed! [LastUpdateDateTime = %s]", IDA_DEVICE_INFO_FILE, dev_data.LastUpdateDateTime);
            i |= 0x20000;
        }
        if (0 != i)
        {
            usb_printf("update [ %s ] failed with err:0x%x!", IDA_DEVICE_INFO_FILE, i);
            return RET_ERROR;
        }
        usb_printf("update [ %s ] successfully!", IDA_DEVICE_INFO_FILE);
        return RET_OK;
    }
}

// 获取设备信息
void GetDeviceInfo(DeviceInfo *dev_info_data)
{
    int ret = 0;

    ret = read_device_info_file(dev_info_data);
    if (ret)
    {
        usb_printf("read device info file failed!");
        // 读取设备信息异常时使用默认值
        strcpy((char *)dev_info_data->Version, DEFAULT_VERSION);
        strcpy((char *)dev_info_data->AccessCode, DEFAULT_ACCESSCODE);
        dev_info_data->ID = DEFAULT_ID;
        strcpy((char *)dev_info_data->SerialNumber, DEFAULT_SERIALNUMBER);
        strcpy((char *)dev_info_data->DeviceName, DEFAULT_DEVICENAME);
        dev_info_data->DeviceType = DEFAULT_DEVICETYPE;
        strcpy((char *)dev_info_data->LastUpdateDateTime, DEFAULT_LASTUPDATETIME);
    }
}
