#include "app_common.h"
#include "usbd_cdc_if.h"

#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"

#include "./MALLOC/malloc.h"

#include "./LIBS/lib_usb_protocol/slidingWindowReceiver_c.h"
#include "./LIBS/lib_usb_protocol/usb_protocol.h"
#include "./LIBS/lib_circular_buffer/CircularBuffer.h"
#include "./LIBS/lib_dwt/lib_dwt_timestamp.h"
#include "./LIBS/lib_base_calculation/calculate_check.h"
#include "./LIBS/lib_file_utils/file_utils.h"
#include "./LIBS/lib_ini_fatfs/ini_fatfs.h"
#include "./MALLOC/malloc.h"
#include "usb_processor.h"
#include "collector_processor.h"
#include "./FATFS/exfuns/fattester.h"
#include "offline_processor.h"

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

static uint32_t USB_Connect_Reply(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);
static uint32_t USB_Disconnect_Reply(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);
static uint32_t USB_DevConfig_Done(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);
static uint32_t USB_CollectChCfg_Reply(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);
static uint32_t USB_Run_Reply(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);
static uint32_t USB_Stop_Reply(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);
static uint32_t USB_Display_Reply(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);
static uint32_t USB_Upgrad_Reply(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);
static uint32_t USB_OfflineCfg_Reply(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);

static uint32_t USB_GetFilelist(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);
static uint32_t USB_DeleteFile(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);
static uint32_t USB_DownloadFileStart(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);
static uint32_t USB_DownloadFileStop(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);
static uint32_t USB_DownloadFileDataAck(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);

static uint32_t PackReplyWithoutDatas(uint32_t event_id);

/* 协议处理函数指针类型：
 * @param data_in: 解包后的用户数据
 * @param data_len: 用户数据长度
 * @param frame_head: 帧头信息指针
 * @param user_head: 用户数据头指针
 * @return: 回复包长度
 */
typedef uint32_t (*usb_protocol_handler)(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);

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
    strcpy((char *)&subden_info[0].SerialNumber[0], "MIRA3102501002");
    strcpy((char *)&subden_info[0].DeviceName[0], Name_Mini_SliceAccel);
    strcpy((char *)&subden_info[0].Version[0], "1.0.0.0_20260114");
    subden_info[0].DeviceType = Mini_SliceAccel;
    subden_info[0].SlotId = 1;
    subden_info[0].Sensitivity = 3; // 浼犳劅鍣ㄧ伒鏁忓害绱㈠紩
    memcpy(send_frame + sizeof(DeviceInfo), subden_info[0].SerialNumber, sizeof(SubDevicelnfo));

    strcpy((char *)&g_dev_info.Version[0], "1.0.0.0_20260114");
    strcpy((char *)&g_dev_info.DeviceName[0], Name_Mini_SliceMicro);
    strcpy((char *)&g_dev_info.AccessCode[0], "NTS2026");
    strcpy((char *)&g_dev_info.SerialNumber[0], "MISMB102501002");
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
static uint32_t USB_Connect_Reply(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *fHead, UserDataHeadInfo *userHead)
{
    (void)data_in;    // 未使用用户数据
    (void)data_len;   // 未使用用户数据长度
    (void)fHead; // 未使用帧头
    (void)userHead;  // 未使用用户头

    g_IdaSystemStatus.st_dev_link.link_status = USB_CONNECTED;

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
    strcpy((char *)&subden_info[0].SerialNumber[0], "MIRA3102501002");
    strcpy((char *)&subden_info[0].DeviceName[0], Name_Mini_SliceAccel);
    strcpy((char *)&subden_info[0].Version[0], "1.0.0.0_20260114");
    subden_info[0].DeviceType = Mini_SliceAccel;
    subden_info[0].SlotId = 1;
    subden_info[0].Sensitivity = 3;  
    memcpy(send_frame + sizeof(DeviceInfo), subden_info[0].SerialNumber, sizeof(SubDevicelnfo));

    strcpy((char *)&g_dev_info.Version[0], "1.0.0.0_20260114");
    strcpy((char *)&g_dev_info.DeviceName[0], Name_Mini_SliceMicro);
    strcpy((char *)&g_dev_info.AccessCode[0], "NTS2026");
    strcpy((char *)&g_dev_info.SerialNumber[0], "MISMB102501002");
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
    UserDataHeadInfo user_head = create_user_data_head(DVS_INIT_CONNECT_OK,
                                                       SOURCE_TYPE_WITH_DATAS,
                                                       DESTINATION_ARM_TO_PC,
                                                       send_len);
    uint32_t packet_len = 0;
    pack_data(send_frame, send_len, &user_head, &frame_head, &packet_len);
    return packet_len;
}

// 处理PC->ARM的DVS_INIT_DISCONNECT事件
static uint32_t USB_Disconnect_Reply(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head)
{
    (void)data_in;
    (void)data_len;
    (void)frame_head;
    (void)user_head;

    g_IdaSystemStatus.st_dev_link.link_status = USB_DISCONNECTED;

    return PackReplyWithoutDatas(DVS_INIT_DISCONNECT_OK);
}

// 处理PC->ARM的DVS_INIT_DEVCONFIG_UPDATE_Done事件
static uint32_t USB_DevConfig_Done(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head)
{
    (void)frame_head;
    (void)user_head;

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
static uint32_t USB_CollectChCfg_Reply(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head)
{
    (void)frame_head;
    (void)user_head;

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

    if (sample_rate > 0)
    {
        CfgAdcSampleRate(sample_rate);

        g_IdaSystemStatus.st_dev_run.run_flag = 1;
        AdcCollectorContrl(g_IdaSystemStatus.st_dev_run.run_flag);
    }

    return PackReplyWithoutDatas(DVSARM_CSP_START_OK);
}
// 处理PC->ARM的DVSARM_RUN事件
static uint32_t USB_Run_Reply(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head)
{
    (void)data_in;
    (void)data_len;
    (void)frame_head;
    (void)user_head;

    return PackReplyWithoutDatas(DVSARM_RUN_OK);
}
// 处理PC->ARM的DVSARM_STOP事件
static uint32_t USB_Stop_Reply(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head)
{
    (void)data_in;
    (void)data_len;
    (void)frame_head;
    (void)user_head;

    uint32_t packet_len = 0;

    g_IdaSystemStatus.st_dev_run.run_flag = 0;
    AdcCollectorContrl(g_IdaSystemStatus.st_dev_run.run_flag);
    AdcCbClear();

    return PackReplyWithoutDatas(DVSARM_STOP_OK);
}

static int32_t f_write_dma_safe(FIL *fil, const uint8_t *src, uint32_t len, UINT *bw_total)
{
#define WRITE_CHUNK_SIZE 512

    int32_t ret = FR_OK;
    uint32_t offset = 0;
    UINT bw = 0;
    *bw_total = 0;

    uint8_t dma_buf[WRITE_CHUNK_SIZE] __attribute__((aligned(4))); // 32字节对齐，确保DMA访问安全

    while (offset < len)
    {
        uint32_t chunk = ((len - offset) > WRITE_CHUNK_SIZE) ? WRITE_CHUNK_SIZE : (len - offset);

        /* 从 HyperRAM 拷贝到 AHB-SRAM */
        memcpy(dma_buf, src + offset, chunk);

        /* 刷新 D-Cache，确保 DMA 读到最新数据 */
        SCB_CleanDCache_by_Addr((uint32_t *)dma_buf, WRITE_CHUNK_SIZE);

        ret = f_write(fil, dma_buf, chunk, &bw);
        if (ret != FR_OK)
        {
            usb_printf("f_write error at offset %lu: %d", offset, ret);
            break;
        }

        *bw_total += bw;
        offset += chunk;

        if (bw < chunk)
        {
            ret = FR_DENIED;
            break;
        }
    }

    return ret;
}

// 处理PC->ARM的DVS_CSP_OFFLINE_SETCONFIG_OK事件
static uint32_t USB_OfflineCfg_Reply(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head)
{
    (void)frame_head;
    (void)user_head;

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
                ret = f_write_dma_safe(&fil, data_in, data_len, &bw);

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
    FrameHeadInfo reply_frame_head = create_default_frame_head(0);
    UserDataHeadInfo reply_user_head = {0};
    reply_user_head.nIsValidFlag = 0x12345678;
    reply_user_head.nEventID = DVS_CSP_OFFLINE_SETCONFIG_OK;
    reply_user_head.nSourceType = SOURCE_TYPE_NO_DATA;
    reply_user_head.nDestinationID = DESTINATION_ARM_TO_PC;
    reply_user_head.nDataLength = 0;
    reply_user_head.nNanoSecond = dwt_get_ns();
    reply_user_head.nParameters0 = ret;

    uint32_t packet_len = 0;
    pack_data(NULL, 0, &reply_user_head, &reply_frame_head, &packet_len);
    return packet_len;
}
// 处理PC->ARM的DVS_INIT_ARM_UPGRADE_REQ_OK事件
static uint32_t USB_Upgrad_Reply(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head)
{
    (void)data_in;
    (void)data_len;
    (void)frame_head;
    (void)user_head;

    FrameHeadInfo reply_frame_head = create_default_frame_head(0);
    UserDataHeadInfo reply_user_head = {0};
    reply_user_head.nIsValidFlag = 0x12345678;
    reply_user_head.nEventID = DVS_INIT_ARM_UPGRADE_REQ_OK;
    reply_user_head.nSourceType = SOURCE_TYPE_NO_DATA;
    reply_user_head.nDestinationID = DESTINATION_ARM_TO_PC;
    reply_user_head.nDataLength = 0;
    reply_user_head.nNanoSecond = dwt_get_ns();
    reply_user_head.nParameters0 = 1;

    uint32_t packet_len = 0;
    pack_data(NULL, 0, &reply_user_head, &reply_frame_head, &packet_len);
    g_IdaSystemStatus.st_dev_mode.reset_all_flag = 1; // ready to restart
    g_reset_time = dwt_get_ns();

    NVIC_SystemReset();

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
// static uint32_t USB_Display_Reply(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head)
// {
//     (void)data_in;
//     (void)data_len;
//     (void)frame_head;
//     (void)user_head;

//     static uint32_t frame_num = 0;
//     uint32_t cb_len = SPI_NUM * ADC_DATA_LEN * SPI_CH_ADC_MAX * BLOCK_LEN;
//     short send_data[BLOCK_LEN][SPI_NUM];
//     struct UserData user_data;

//     if (cb_size(g_cb_adc) < cb_len)
//     {
//         return NULL;
//     }

//     frame_num++;

//     //    ArmBackFrameHeader data_head;
//     user_data.data_head.nVersion = 0x12345678;
//     user_data.data_head.nDataSource = 0;
//     user_data.data_head.nFrameChCount = SPI_NUM;
//     user_data.data_head.nFrameLen = BLOCK_LEN;
//     user_data.data_head.nTotalFrameNum = frame_num;
//     user_data.data_head.nCurNs = dwt_get_ns();

//     //    memset(send_data, 0, sizeof(send_data));
//     //    memcpy(send_data, &data_head, sizeof(data_head));
//     cb_read(g_cb_adc, (char *)send_data, cb_len);
//     transpose(send_data, user_data.send_frame);

//     uint32_t send_len = sizeof(user_data);

//     FrameHeadInfo reply_frame_head = create_default_frame_head(frame_num);

//     UserDataHeadInfo reply_user_head = create_user_data_head(DVSARM_DISPNEXT_OK,
//                                                              SOURCE_TYPE_WITH_DATAS,
//                                                              DESTINATION_ARM_TO_PC,
//                                                              send_len);
//     uint32_t packet_len = 0;
//     pack_data((uint8_t *)&user_data, send_len, &reply_user_head, &reply_frame_head, &packet_len);

//     return packet_len;
// }

static uint32_t USB_Display_Reply(uint8_t *data_in, uint32_t data_len,
                                  FrameHeadInfo *frame_head,
                                  UserDataHeadInfo *user_head)
{
    (void)data_in;
    (void)data_len;
    (void)frame_head;
    (void)user_head;

    static uint32_t frame_num = 0;
    const uint32_t cb_needed = BLOCK_LEN * ADC_DATA_LEN; // 单通道需要字节数

    /* 统计使能且数据就绪的通道 */
    uint8_t ready_chs[ADC_CH_TOTAL];
    uint8_t ready_cnt = 0;
    for (uint8_t i = 0; i < ADC_CH_TOTAL; i++)
    {
        if ((g_ch_enable_mask & (1u << i)) &&
            g_cb_ch[i] &&
            cb_size(g_cb_ch[i]) >= (int)cb_needed)
        {
            ready_chs[ready_cnt++] = i;
        }
    }
    if (ready_cnt == 0)
        return 0;

    /* 填充帧头 */
    struct UserData user_data;
    memset(&user_data, 0, sizeof(user_data));
    frame_num++;
    user_data.data_head.nVersion = 0x12345678;
    user_data.data_head.nDataSource = 0;
    user_data.data_head.nFrameChCount = ready_cnt; // 实际发送通道数
    user_data.data_head.nFrameLen = BLOCK_LEN;
    user_data.data_head.nTotalFrameNum = frame_num;
    user_data.data_head.nCurNs = dwt_get_ns();

    /* 按通道顺序读取数据，写入连续发送区 */
    for (uint8_t n = 0; n < ready_cnt; n++)
    {
        uint8_t ch = ready_chs[n];
        cb_read(g_cb_ch[ch], (char *)user_data.send_frame[n], cb_needed);
    }

    /* 实际发送大小：帧头 + ready_cnt×BLOCK_LEN×2字节 */
    uint32_t send_len = sizeof(ArmBackFrameHeader) + (uint32_t)ready_cnt * BLOCK_LEN * sizeof(short);

    FrameHeadInfo reply_frame_head = create_default_frame_head(frame_num);
    UserDataHeadInfo reply_user_head =
        create_user_data_head(DVSARM_DISPNEXT_OK,
                              SOURCE_TYPE_WITH_DATAS,
                              DESTINATION_ARM_TO_PC,
                              send_len);
    uint32_t packet_len = 0;
    pack_data((uint8_t *)&user_data, send_len,
              &reply_user_head, &reply_frame_head, &packet_len);
    return packet_len;
}

// 处理PC->ARM的DVSARM_DISPNEXT_OK事件，循环发送数据
void USB_Display_All(uint32_t run_flag)
{
    // 离线计划运行中，禁止发送
    if (g_IdaSystemStatus.st_dev_offline.start_flag == 1)
        return;

    // 互斥处理：离线记录时不能发送
    if (g_IdaSystemStatus.st_dev_record.record_status == RECORD_RUN)
    {
        // 正在记录，禁止发送
        return;
    }

    if (run_flag)
    {
        FrameHeadInfo reply_frame_head = create_default_frame_head(0);
        UserDataHeadInfo reply_user_head = {0};
        USB_Display_Reply(NULL, 0, &reply_frame_head, &reply_user_head);
    }
}
// 处理PC->ARM的DVS_FILE_GET_FILELIST_OK事件
static uint32_t USB_GetFilelist(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head)
{
    (void)data_in;
    (void)data_len;
    (void)frame_head;
    (void)user_head;

#define FILELIST_BUFFER_SIZE (4096)

    // 使用动态内存分配
    uint8_t *user_data = (uint8_t *)mymalloc(SRAMEX, FILELIST_BUFFER_SIZE);
    if (user_data == NULL)
    {
        usb_printf("[GetFilelist] ERROR: Memory allocation failed\n");
        // 返回错误回复
        FrameHeadInfo reply_frame_head = create_default_frame_head(0);
        UserDataHeadInfo reply_user_head = create_user_data_head(DVS_FILE_GET_FILELIST_OK,
                                                                 SOURCE_TYPE_NO_DATA,
                                                                 DESTINATION_ARM_TO_PC,
                                                                 0);
        uint32_t packet_len = 0;
        pack_data(NULL, 0, &reply_user_head, &reply_frame_head, &packet_len);
        return packet_len;
    }

    uint32_t send_len = 0;

    // 初始化缓冲区
    memset(user_data, 0, FILELIST_BUFFER_SIZE);

    /* todo : get_file_list_by_extension*/
    FileList_t file_list = {0};
    int8_t ret;
    char date_time_buf[20];

    // 获取0:/data目录下的.txt文件列表，最多10个
    ret = get_file_list_by_extension("0:/RecordDataFiles", ".rec", &file_list, 10);

    if (ret == 0 && file_list.count > 0)
    {
        usb_printf("Found %d files:\n", file_list.count);

        for (uint32_t i = 0; i < file_list.count; i++)
        {
            // 检查缓冲区是否足够
            uint32_t used_len = strlen((const char *)user_data);
            uint32_t remaining = FILELIST_BUFFER_SIZE - used_len - 1;

            // 估算需要空间（路径+分隔符+数字+日期+分隔符）
            uint32_t needed = strlen(file_list.files[i].path) + strlen(date_time_buf) + 50;

            if (remaining < needed)
            {
                usb_printf("Buffer exhausted at file %u\n", i);
                break;
            }

            // 格式化日期时间
            format_date_time(file_list.files[i].create_date,
                             file_list.files[i].create_time,
                             date_time_buf, sizeof(date_time_buf));

            // 打印文件信息
            usb_printf("File %d:\n", i + 1);
            usb_printf("  Path: %s\n", file_list.files[i].path);
            usb_printf("  Size: %u bytes\n", file_list.files[i].size);
            usb_printf("  Created: %s\n", date_time_buf);
            usb_printf("\n");

            // 使用安全的字符串操作
            strncat((char *)user_data, file_list.files[i].path, remaining);
            remaining = FILELIST_BUFFER_SIZE - strlen((const char *)user_data) - 1;

            strncat((char *)user_data, ",", remaining);
            remaining = FILELIST_BUFFER_SIZE - strlen((const char *)user_data) - 1;

            char str[16];
            snprintf(str, sizeof(str), "%u", file_list.files[i].size);
            strncat((char *)user_data, str, remaining);
            remaining = FILELIST_BUFFER_SIZE - strlen((const char *)user_data) - 1;

            strncat((char *)user_data, ",", remaining);
            remaining = FILELIST_BUFFER_SIZE - strlen((const char *)user_data) - 1;

            strncat((char *)user_data, date_time_buf, remaining);
            remaining = FILELIST_BUFFER_SIZE - strlen((const char *)user_data) - 1;

            strncat((char *)user_data, "|", remaining);
        }

        // 释放文件列表
        free_file_list(&file_list);
    }
    else
    {
        usb_printf("Failed to get file list: ret=%d, count=%d\n", ret,
                   (ret == 0 ? file_list.count : 0));
        snprintf((char *)user_data, FILELIST_BUFFER_SIZE, "ERROR:GetListFailed");
    }

    send_len = strlen((const char *)user_data);

    FrameHeadInfo reply_frame_head = create_default_frame_head(0);

    UserDataHeadInfo reply_user_head = create_user_data_head(DVS_FILE_GET_FILELIST_OK,
                                                             SOURCE_TYPE_WITH_DATAS,
                                                             DESTINATION_ARM_TO_PC,
                                                             send_len);

    uint32_t packet_len = 0;
    pack_data((uint8_t *)user_data, send_len, &reply_user_head, &reply_frame_head, &packet_len);

    // 释放动态分配的内存
    myfree(SRAMEX, user_data);

#undef FILELIST_BUFFER_SIZE

    return packet_len;
}

// 处理PC->ARM的DVS_FILE_DELETE_OK事件
static uint32_t USB_DeleteFile(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head)
{
    (void)user_head;

    int32_t ret = RET_OK;
    FRESULT fres = FR_OK;
    FILINFO finfo = {0};
    char file_path[256] = {0};
    uint64_t op_time_start = 0;
    FrameHeadInfo reply_frame_head;
    UserDataHeadInfo reply_user_head;
    uint32_t packet_len = 0;

    // 参数验证
    if (data_in == NULL || data_len == 0 || data_len > 255)
    {
        usb_printf("[DeleteFile] ERROR: Invalid input (data_len=%u)\r\n", data_len);
        ret = -1;
        goto send_reply;
    }

    // 文件路径处理（确保以null终止）
    memcpy(file_path, data_in, data_len);
    file_path[data_len] = '\0';

    // 检查文件路径是否合法（防止路径遍历攻击）
    if (strlen(file_path) == 0)
    {
        usb_printf("[DeleteFile] ERROR: Invalid/empty path (path=%s)\r\n", file_path);
        ret = -2;
        goto send_reply;
    }

    // 检查路径格式（至少包含 'X:' 盘符）
    if (strlen(file_path) < 2 || file_path[1] != ':')
    {
        usb_printf("[DeleteFile] ERROR: Invalid path format (path=%s)\r\n", file_path);
        ret = -2;
        goto send_reply;
    }

    // 防止删除受保护的关键文件
    if (strstr(file_path, "DeviceInfo.ini") != NULL ||
        strstr(file_path, "OfflineCfgSchedule.txt") != NULL)
    {
        usb_printf("[DeleteFile] ERROR: Protected file (path=%s)\r\n", file_path);
        ret = -7;
        goto send_reply;
    }

    op_time_start = dwt_get_ns();
    usb_printf("[DeleteFile] START: path=%s\r\n", file_path);

    // 验证文件存在（删除前检查）
    fres = f_stat(file_path, &finfo);
    if (fres != FR_OK)
    {
        if (fres == FR_NO_FILE)
        {
            usb_printf("[DeleteFile] WARNING: File not found (path=%s)\r\n", file_path);
            ret = -3; // 文件不存在
        }
        else if (fres == FR_INVALID_NAME)
        {
            usb_printf("[DeleteFile] ERROR: Invalid path format (path=%s, code=%d)\r\n", file_path, fres);
            ret = -4; // 路径无效
        }
        else
        {
            usb_printf("[DeleteFile] ERROR: Stat failed (path=%s, code=%d)\r\n", file_path, fres);
            ret = -6;
        }
        goto send_reply;
    }

    // 防止删除目录
    if (finfo.fattrib & AM_DIR)
    {
        usb_printf("[DeleteFile] ERROR: Cannot delete directory (path=%s)\r\n", file_path);
        ret = -8;
        goto send_reply;
    }

    uint32_t file_size = finfo.fsize;
    usb_printf("[DeleteFile] File info: size=%u bytes\r\n", file_size);

    // 调用FATFS删除文件
    fres = f_unlink(file_path);

    if (fres == FR_OK)
    {
        // 验证删除结果（二次确认文件已不存在）
        memset(&finfo, 0, sizeof(finfo));
        FRESULT verify_res = f_stat(file_path, &finfo);

        if (verify_res == FR_NO_FILE)
        {
            uint64_t op_time_end = dwt_get_ns();
            usb_printf("[DeleteFile] SUCCESS: File deleted (path=%s, size=%u bytes, time=%llu ns)\r\n",
                       file_path, file_size, op_time_end - op_time_start);
            ret = RET_OK;
        }
        else
        {
            usb_printf("[DeleteFile] ERROR: Delete verification failed (path=%s, verify_code=%d)\r\n",
                       file_path, verify_res);
            ret = -9; // 删除验证失败
        }
    }
    else if (fres == FR_DENIED)
    {
        usb_printf("[DeleteFile] ERROR: Access denied (path=%s, code=%d)\r\n", file_path, fres);
        ret = -5; // 拒绝访问
    }
    else
    {
        usb_printf("[DeleteFile] ERROR: Operation failed (path=%s, code=%d)\r\n", file_path, fres);
        ret = -6; // 其他错误
    }

send_reply:
    // 构建回复包
    reply_frame_head = create_default_frame_head(0);
    memset(&reply_user_head, 0, sizeof(reply_user_head));
    reply_user_head.nIsValidFlag = 0x12345678;
    reply_user_head.nEventID = DVS_FILE_DELETE_OK;
    reply_user_head.nSourceType = SOURCE_TYPE_NO_DATA;
    reply_user_head.nDestinationID = DESTINATION_ARM_TO_PC;
    reply_user_head.nDataLength = 0;
    reply_user_head.nNanoSecond = dwt_get_ns();
    reply_user_head.nParameters0 = ret; // 删除结果：0=成功, <0=错误码

    pack_data(NULL, 0, &reply_user_head, &reply_frame_head, &packet_len);
    return packet_len;
}

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

#define DOWNLOAD_PACK_SIZE_DEFAULT (4096U) /* 默认每包字节数 */
#define DOWNLOAD_PACK_SIZE_MAX (8192U)     /* 最大每包字节数 */

/* 下载会话状态 */
typedef struct
{
    uint8_t active; /* 0=空闲, 1=下载中, 2=等待ACK */
    uint32_t session_id;
    uint32_t total_size;
    uint32_t total_packs;
    uint32_t pack_size;       /* 实际每包大小 */
    uint32_t next_pack_index; /* 下一个要发送的包序号 */
    uint32_t bytes_sent;      /* 已发送字节数 */
    FIL fil;
    char file_path[256];
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
        /* 读取出错，关闭文件 */
        f_close(&g_download_session.fil);
        memset(&g_download_session, 0, sizeof(g_download_session));
        return;
    }

    /* 使用 SendRecordPackHead_t 作为数据包头 */
    SendRecordPackHead pack_head;
    pack_head.totalPackNum = g_download_session.total_packs;
    pack_head.currentPackIdx = g_download_session.next_pack_index + 1; /* 从1开始计数 */
    pack_head.currentPackAddr = g_download_session.bytes_sent;
    pack_head.packSize = br;
    pack_head.crc32 = download_crc32(g_download_buf, br);
    pack_head.reserved = 0;

    /* 组合 pack_head + data 发送 */
    // download_send_next_pack() 中，将栈变量改为静态或使用 g_download_buf 分段发送
    static uint8_t tx_buf[sizeof(SendRecordPackHead) + DOWNLOAD_PACK_SIZE_MAX]
        __attribute__((aligned(4)));

    memcpy(tx_buf, &pack_head, sizeof(SendRecordPackHead));
    memcpy(tx_buf + sizeof(SendRecordPackHead), g_download_buf, br);
    uint32_t tx_len = sizeof(SendRecordPackHead) + br;

    download_send_with_data(DVS_FILE_DOWNLOAD_DATA, tx_buf, tx_len);

    g_download_session.bytes_sent += br;
    g_download_session.active = 2; /* 等待 ACK */

    usb_printf("[Download] Pack %lu/%lu sent, bytes=%u, total_sent=%lu\r\n",
               g_download_session.next_pack_index + 1,
               g_download_session.total_packs,
               br,
               g_download_session.bytes_sent);
}

/* --------------------------------------------------------------------------
 * DVS_FILE_DOWNLOAD_START 处理：PC 发起下载请求（接收文件名字符串）
 * -------------------------------------------------------------------------- */
static uint32_t USB_DownloadFileStart(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head)
{
    (void)frame_head;

    FrameHeadInfo reply_frame_head = create_default_frame_head(0);
    UserDataHeadInfo reply_user_head = {0};

    uint32_t packet_len = 0;

    int32_t ret = 0;
    char file_path[256] = {0};

    if (g_download_session.active)
    {
        usb_printf("[Download] Session already active\r\n");
        ret = -1;
        goto send_start_reply;
    }

    /* 验证输入 */
    if (data_in == NULL || data_len == 0 || data_len > 255)
    {
        usb_printf("[Download] START payload invalid (len=%u)\r\n", data_len);
        ret = -2;
        goto send_start_reply;
    }

    /* 文件名字符串处理 */
    memcpy(file_path, data_in, data_len);
    file_path[data_len] = '\0';

    /* 打开文件 */
    FRESULT fres = f_open(&g_download_session.fil,
                          file_path,
                          FA_OPEN_EXISTING | FA_READ);
    if (fres != FR_OK)
    {
        usb_printf("[Download] f_open failed: path=%s res=%d\r\n",
                   file_path, fres);
        ret = -3;
        goto send_start_reply;
    }

    uint32_t file_size = f_size(&g_download_session.fil);
    uint32_t pack_size = DOWNLOAD_PACK_SIZE_DEFAULT; /* 使用默认包大小 */
    uint32_t total_packs = (file_size + pack_size - 1) / pack_size;
    if (total_packs == 0)
        total_packs = 1;

    /* 初始化会话 */
    g_download_session.active = 1;
    g_download_session.session_id = HAL_GetTick();
    g_download_session.total_size = file_size;
    g_download_session.total_packs = total_packs;
    g_download_session.pack_size = pack_size;
    g_download_session.next_pack_index = 0;
    g_download_session.bytes_sent = 0;
    strncpy(g_download_session.file_path, file_path,
            sizeof(g_download_session.file_path) - 1);

    usb_printf("[Download] Session started: path=%s size=%lu packs=%lu\r\n",
               g_download_session.file_path,
               file_size, total_packs);

    ret = 0; /* 成功 */

    /* 回复 START_OK，通过 nParameters0 报告结果 */
    reply_user_head.nIsValidFlag = 0x12345678;
    reply_user_head.nEventID = DVS_FILE_DOWNLOAD_START_OK;
    reply_user_head.nSourceType = SOURCE_TYPE_NO_DATA;
    reply_user_head.nDestinationID = DESTINATION_ARM_TO_PC;
    reply_user_head.nDataLength = 0;
    reply_user_head.nNanoSecond = dwt_get_ns();
    reply_user_head.nParameters0 = ret; /* 0=成功, 负数=错误码 */

    pack_data(NULL, 0, &reply_user_head, &reply_frame_head, &packet_len);

    /* 立即发送第一包 */
    download_send_next_pack();

    return packet_len;

send_start_reply:;
    /* 回复 START_OK，通过 nParameters0 报告结果 */

    reply_user_head.nIsValidFlag = 0x12345678;
    reply_user_head.nEventID = DVS_FILE_DOWNLOAD_START_OK;
    reply_user_head.nSourceType = SOURCE_TYPE_NO_DATA;
    reply_user_head.nDestinationID = DESTINATION_ARM_TO_PC;
    reply_user_head.nDataLength = 0;
    reply_user_head.nNanoSecond = dwt_get_ns();
    reply_user_head.nParameters0 = ret; /* 0=成功, 负数=错误码 */

    pack_data(NULL, 0, &reply_user_head, &reply_frame_head, &packet_len);

    return packet_len;
}

/* --------------------------------------------------------------------------
 * DVS_FILE_DOWNLOAD_DATA_ACK 处理：PC 确认收到一包，MCU 发下一包
 * 数据格式：pack_index (4字节，从1开始），result (4字节，0=OK)
 * -------------------------------------------------------------------------- */
static uint32_t USB_DownloadFileDataAck(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head)
{
    (void)frame_head;
    (void)user_head;

    if (!g_download_session.active)
    {
        usb_printf("[Download] ACK received but no active session\r\n");
        return 0;
    }

    // /* 解析ACK：pack_index(4字节) + result(4字节) */
    // if (data_len < 8)
    // {
    //     usb_printf("[Download] ACK payload too short\r\n");
    //     return 0;
    // }

    // uint32_t pack_index = *(uint32_t *)data_in;
    // int32_t result = *(int32_t *)(data_in + 4);

    // if (result != 0)
    // {
    //     /* PC 要求重传：回退文件指针 */
    //     usb_printf("[Download] PC requested retransmit pack %lu\r\n", pack_index);
    //     uint32_t seek_offset = (pack_index - 1) * g_download_session.pack_size; /* pack_index从1开始 */
    //     f_lseek(&g_download_session.fil, seek_offset);
    //     g_download_session.bytes_sent = seek_offset;
    //     g_download_session.next_pack_index = pack_index - 1;
    //     g_download_session.active = 1;
    //     download_send_next_pack();
    //     return 0;
    // }

    // /* 确认成功，推进包序号 */
    g_download_session.next_pack_index++;
    g_download_session.active = 1;

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
 * 数据格式：abort_flag (1字节，0=正常完成, 1=中止)
 * -------------------------------------------------------------------------- */
static uint32_t USB_DownloadFileStop(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head)
{
    (void)data_in;
    (void)data_len;
    (void)frame_head;

    int32_t ret = 0;

    if (g_download_session.active)
    {
        f_close(&g_download_session.fil);
        usb_printf("[Download] Session closed: sent=%lu bytes\r\n",
                   g_download_session.bytes_sent);
        memset(&g_download_session, 0, sizeof(g_download_session));
        ret = 0; /* 成功 */
    }
    else
    {
        /* 没有活跃会话，仍返回成功 */
        ret = 0;
    }

    /* 回复 STOP_OK，通过 nParameters0 报告结果 */
    FrameHeadInfo reply_frame_head = create_default_frame_head(0);
    UserDataHeadInfo reply_user_head = {0};
    reply_user_head.nIsValidFlag = 0x12345678;
    reply_user_head.nEventID = DVS_FILE_DOWNLOAD_STOP_OK;
    reply_user_head.nSourceType = SOURCE_TYPE_NO_DATA;
    reply_user_head.nDestinationID = DESTINATION_ARM_TO_PC;
    reply_user_head.nDataLength = 0;
    reply_user_head.nNanoSecond = dwt_get_ns();
    reply_user_head.nParameters0 = ret; /* 0=成功, 负数=错误码 */

    uint32_t packet_len = 0;
    pack_data(NULL, 0, &reply_user_head, &reply_frame_head, &packet_len);
    return packet_len;
}

uint64_t heart_recv_time = 0;
// 处理PC->ARM的事件
void on_frame(uint8_t *data, uint32_t len)
{

    // // 解包
    // uint8_t *unpacked_data = NULL;
    // UserDataHeadInfo unpacked_head;
    // FrameHeadInfo unpacked_frame_head;
    // uint32_t unpacked_data_len = 0;

    // int result = unpack_data(frame, frame_len, &unpacked_data,
    //                          &unpacked_head, &unpacked_frame_head,
    //                          &unpacked_data_len);

    uint8_t *ptr = data;

    // ----------------FrameHeadInfo------------------

    FrameHeadInfo *frameHeadInfo = (FrameHeadInfo *)ptr;

    ptr += sizeof(FrameHeadInfo);

    // ----------------FrameLen------------------

    uint32_t *frameLen = (uint32_t *)ptr;

    ptr += sizeof(uint32_t);

    // ----------------UserDataHeadInfo------------------

    UserDataHeadInfo *userDataHeadInfo = (UserDataHeadInfo *)ptr;

    ptr += sizeof(UserDataHeadInfo);

    // -----------------Data------------------

    uint8_t *userData = ptr;

    // -----------------------------------

    // usb_printf("---------------------on_frame: %d %d \n", len, userDataHeadInfo->nDataLength);

    heart_recv_time = dwt_get_us();
    for (uint8_t i = 0; i < sizeof(protocol_getdata) / sizeof(protocol_getdata[0]); i++)
    {
        if (userDataHeadInfo->nEventID == protocol_getdata[i].usb_frame_tag)
        {
            protocol_getdata[i].usb_process_handle(userData, userDataHeadInfo->nDataLength, frameHeadInfo, userDataHeadInfo);
        }
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
            AdcCbClear();
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
