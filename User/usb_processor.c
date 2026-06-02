#include "app_common.h"
#include "usbd_cdc_if.h"

#include "usbd_core.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"

#include "./SYSTEM/delay/delay.h"
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
#include "rs485_processor.h"
#include "./FATFS/exfuns/fattester.h"
#include "offline_processor.h"
#include "./BSP/MMC/mmc_sdcard.h"
#include "./FATFS/source/ff.h"
#include "./BSP/RS485/rs485.h"
#include "dataType.h"
#include "bridge_config.h"

/*********************************************************************************/

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

static uint32_t USB_SubDevConfig_Update(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);

static uint32_t USB_SubDevConfig_Read(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);

static uint32_t USB_SubDevId_Update(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);

static uint32_t USB_GetFilelist(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);
static uint32_t USB_DeleteFile(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);
static uint32_t USB_DownloadFileStart(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);
static uint32_t USB_DownloadFileStop(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);
static uint32_t USB_DownloadFileDataAck(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);
static uint32_t USB_CalibrationRead(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);
static uint32_t USB_CalibrationWrite(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);
static uint32_t USB_Zeroing_Reply(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head);

static uint32_t PackReplyWithoutDatas(uint32_t event_id);
static uint32_t PackReplyWithoutDatasParam(uint32_t event_id, int32_t param0);

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
        {DVS_INIT_SUB_DEVCONFIG_UPDATE_Done, USB_SubDevConfig_Update},
        {DVS_INIT_SUB_DEVCONFIG_READ_Done, USB_SubDevConfig_Read},
        {DVS_INIT_SUB_ID_UPDATE_Done, USB_SubDevId_Update},
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
        {DVS_FILE_CALIBRATION_READ, USB_CalibrationRead},
        {DVS_FILE_CALIBRATION_WRITE, USB_CalibrationWrite},

        {DVS_INIT_ZEROING, USB_Zeroing_Reply},
};

/*********************************************************************************/

USBD_HandleTypeDef g_usbd_handle = {0};

uint64_t g_reset_time = 0;

struct UserData *g_user_data;

/*********************************************************************************/

#define DEVICE_INFO_BIN_PATH "0:/DeviceInfo.bin"
#define DEVICE_INFO_BIN_MAGIC (0x31495644UL) /* "DVI1" */
#define DEVICE_INFO_BIN_VERSION (1U)

typedef struct
{
    uint32_t magic;
    uint16_t version;
    uint16_t reserved;
    uint32_t payload_len;
    uint32_t payload_crc32;
} DeviceInfoBinHeader;

static void copy_text_field(char *dst, uint32_t dst_size, const char *src, uint32_t src_size)
{
    uint32_t i;

    if ((dst == NULL) || (src == NULL) || (dst_size == 0U))
    {
        return;
    }

    for (i = 0U; i < (dst_size - 1U); i++)
    {
        if ((i >= src_size) || (src[i] == '\0'))
        {
            break;
        }
        dst[i] = src[i];
    }
    dst[i] = '\0';
}

static void device_info_set_defaults(void)
{
    memset(&g_dev_info, 0, sizeof(g_dev_info));
    strcpy((char *)g_dev_info.Version, FIRMWARE_VERSION);
    strcpy((char *)g_dev_info.DeviceName, DEFAULT_DEVICENAME);
    strcpy((char *)g_dev_info.AccessCode, DEFAULT_ACCESSCODE);
    strcpy((char *)g_dev_info.SerialNumber, DEFAULT_SERIALNUMBER);
    g_dev_info.DeviceType = DEFAULT_DEVICETYPE;
}

static void device_info_print_detail(const char *tag)
{
    const char *name = (tag == NULL) ? "g_dev_info" : tag;

    usb_printf("[%s] Version      : %s\r\n", name, g_dev_info.Version);
    usb_printf("[%s] DeviceName   : %s\r\n", name, g_dev_info.DeviceName);
    usb_printf("[%s] AccessCode   : %s\r\n", name, g_dev_info.AccessCode);
    usb_printf("[%s] SerialNumber : %s\r\n", name, g_dev_info.SerialNumber);
    usb_printf("[%s] DeviceType   : %ld\r\n", name, (long)g_dev_info.DeviceType);
    usb_printf("[%s] IsConnected  : %ld\r\n", name, (long)g_dev_info.IsConnected);
    usb_printf("[%s] SubDeviceNum : %u\r\n", name, (unsigned int)g_dev_info.SubDeviceNum);
    usb_printf("[%s] TotalDiskKB  : %.1f\r\n", name, g_dev_info.fTotalDiskSapce);
    usb_printf("[%s] FreeDiskKB   : %.1f\r\n", name, g_dev_info.fFreeDiskSpace);
}

static int8_t device_info_save_to_bin(const DeviceInfo *info)
{
    FIL fil;
    FRESULT res;
    UINT bw = 0U;
    DeviceInfoBinHeader hdr;
    uint32_t crc32;

    if (info == NULL)
    {
        return RET_ERROR;
    }

    crc32 = (uint32_t)make_crc32((unsigned char *)info, (unsigned int)sizeof(DeviceInfo));
    hdr.magic = DEVICE_INFO_BIN_MAGIC;
    hdr.version = DEVICE_INFO_BIN_VERSION;
    hdr.reserved = 0U;
    hdr.payload_len = sizeof(DeviceInfo);
    hdr.payload_crc32 = crc32;

    res = f_open(&fil, DEVICE_INFO_BIN_PATH, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK)
    {
        return RET_ERROR;
    }

    res = f_write(&fil, &hdr, sizeof(hdr), &bw);
    if ((res == FR_OK) && (bw != (UINT)sizeof(hdr)))
    {
        res = FR_DISK_ERR;
    }
    if (res == FR_OK)
    {
        res = f_write(&fil, info, sizeof(DeviceInfo), &bw);
    }
    if ((res == FR_OK) && (bw != (UINT)sizeof(DeviceInfo)))
    {
        res = FR_DISK_ERR;
    }
    if (res == FR_OK)
    {
        (void)f_sync(&fil);
    }
    (void)f_close(&fil);
    return (res == FR_OK) ? RET_OK : RET_ERROR;
}

static int8_t device_info_load_from_bin(DeviceInfo *info)
{
    FIL fil;
    FRESULT res;
    UINT br = 0U;
    DeviceInfoBinHeader hdr;
    uint32_t crc32;

    if (info == NULL)
    {
        return RET_ERROR;
    }

    res = f_open(&fil, DEVICE_INFO_BIN_PATH, FA_READ);
    if (res != FR_OK)
    {
        return RET_ERROR;
    }

    if (f_size(&fil) == sizeof(DeviceInfo))
    {
        /* Legacy format: payload only */
        res = f_read(&fil, info, sizeof(DeviceInfo), &br);
        (void)f_close(&fil);
        if ((res != FR_OK) || (br != (UINT)sizeof(DeviceInfo)))
        {
            return RET_ERROR;
        }
    }
    else
    {
        res = f_read(&fil, &hdr, sizeof(hdr), &br);
        if ((res != FR_OK) || (br != (UINT)sizeof(hdr)))
        {
            (void)f_close(&fil);
            return RET_ERROR;
        }
        if ((hdr.magic != DEVICE_INFO_BIN_MAGIC) ||
            (hdr.version != DEVICE_INFO_BIN_VERSION) ||
            (hdr.payload_len != sizeof(DeviceInfo)))
        {
            (void)f_close(&fil);
            return RET_ERROR;
        }

        res = f_read(&fil, info, sizeof(DeviceInfo), &br);
        (void)f_close(&fil);
        if ((res != FR_OK) || (br != (UINT)sizeof(DeviceInfo)))
        {
            return RET_ERROR;
        }

        crc32 = (uint32_t)make_crc32((unsigned char *)info, (unsigned int)sizeof(DeviceInfo));
        if (crc32 != hdr.payload_crc32)
        {
            return RET_ERROR;
        }
    }

    info->Version[STR_32 - 1] = '\0';
    info->DeviceName[STR_32 - 1] = '\0';
    info->AccessCode[STR_32 - 1] = '\0';
    info->SerialNumber[STR_32 - 1] = '\0';
    strcpy((char *)info->Version, FIRMWARE_VERSION);
    return RET_OK;
}

void usb_device_info_reload_from_file(void)
{
    if (device_info_load_from_bin(&g_dev_info) != RET_OK)
    {
        device_info_set_defaults();
        (void)device_info_save_to_bin(&g_dev_info);
        device_info_print_detail("DeviceInfo-Default");
    }
    else
    {
        device_info_print_detail("DeviceInfo-Load");
    }
}

static void device_info_update_from_detail(const DeviceDetailInfo *dev_detail)
{
    if (dev_detail == NULL)
    {
        return;
    }

    copy_text_field((char *)g_dev_info.Version, STR_32, dev_detail->SoftwareVersion, STR_32);
    strcpy((char *)g_dev_info.Version, FIRMWARE_VERSION);
    copy_text_field((char *)g_dev_info.DeviceName, STR_32, dev_detail->DeviceName, STR_32);
    copy_text_field((char *)g_dev_info.AccessCode, STR_32, dev_detail->AccessCode, STR_32);
    copy_text_field((char *)g_dev_info.SerialNumber, STR_32, dev_detail->SerialNumber, STR_32);
    g_dev_info.DeviceType = dev_detail->DeviceType;
}

void usb_init(void)
{
    device_info_set_defaults();

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

    // ----------------------

    g_user_data = (struct UserData *)mymalloc(SRAMEX, sizeof(struct UserData));
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

static uint32_t PackReplyWithoutDatasParam(uint32_t event_id, int32_t param0)
{
    FrameHeadInfo frame_head = create_default_frame_head(0);
    UserDataHeadInfo user_head = create_user_data_head(event_id,
                                                       SOURCE_TYPE_NO_DATA,
                                                       DESTINATION_ARM_TO_PC,
                                                       0);

    uint32_t packet_len = 0;
    user_head.nParameters0 = param0;
    pack_data(NULL, 0, &user_head, &frame_head, &packet_len);
    return packet_len;
}

static void fill_default_subdev_info(SubDevicelnfo *info)
{
    if (info == NULL)
    {
        return;
    }

    memset(info, 0, sizeof(SubDevicelnfo));
    strcpy((char *)&info->SerialNumber[0], "MIRA3102501001");
    strcpy((char *)&info->DeviceName[0], Name_Mini_SliceAccel);
    strcpy((char *)&info->Version[0], "1.0.0.0_20260114");
    info->DeviceType = Mini_SliceAccel;
    info->SlotId = 1;
    info->Sensitivity = 3;
}

static uint8_t build_device_report_payload(uint8_t *send_frame, uint32_t *send_len)
{
    uint8_t i;
    uint8_t subdev_count = 0U;
    uint8_t *subdev_ptr;
    SubDevicelnfo default_subdev;

    if ((send_frame == NULL) || (send_len == NULL))
    {
        return 0U;
    }

    subdev_ptr = send_frame + sizeof(DeviceInfo);
    for (i = 0U; i < SUBDEV_NUM_MAX; i++)
    {
        if (g_subdev_valid[i] != 0U)
        {
            memcpy(subdev_ptr, &g_SubDevicelnfo[i], sizeof(SubDevicelnfo));
            subdev_ptr += sizeof(SubDevicelnfo);
            subdev_count++;
        }
    }

    if (subdev_count == 0U)
    {
        fill_default_subdev_info(&default_subdev);
        memcpy(subdev_ptr, &default_subdev, sizeof(SubDevicelnfo));
        subdev_ptr += sizeof(SubDevicelnfo);
        subdev_count = 1U;
    }

    g_dev_info.IsConnected = (g_IdaSystemStatus.st_dev_link.link_status == USB_CONNECTED) ? 1 : 0;
    g_dev_info.SubDeviceNum = subdev_count;

    memcpy(send_frame, &g_dev_info, sizeof(DeviceInfo));
    *send_len = sizeof(DeviceInfo) + sizeof(SubDevicelnfo) * subdev_count;
    return subdev_count;
}

static uint32_t USB_DetectPeriod(void)
{
    uint8_t serial_num = 0;
    uint8_t send_frame[sizeof(DeviceInfo) + sizeof(SubDevicelnfo) * SUBDEV_NUM_MAX];
    uint32_t send_len = 0;

    // 发送数据
    build_device_report_payload(send_frame, &send_len);
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
    (void)data_in;  // 未使用用户数据
    (void)data_len; // 未使用用户数据长度
    (void)fHead;    // 未使用帧头

    g_IdaSystemStatus.st_dev_link.link_status = USB_CONNECTED;
    if (userHead != NULL)
    {
        if (SoftTimeSyncFromNanoSecond(userHead->nNanoSecond) == RET_OK)
        {
            usb_printf("[Time] Synced from USB_Connect_Reply, ns=%lld\r\n", userHead->nNanoSecond);
        }
    }

    uint8_t serial_num = 0;
    uint8_t send_frame[sizeof(DeviceInfo) + sizeof(SubDevicelnfo) * SUBDEV_NUM_MAX];
    uint32_t send_len = 0;

    // 发送数据
    // IdaGetDiskSpaceKB(&g_dev_info.fTotalDiskSapce, &g_dev_info.fFreeDiskSpace);
    test_filesystem();
    build_device_report_payload(send_frame, &send_len);
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
    // 拷贝对应字段
    device_info_update_from_detail(dev_detail);
    (void)device_info_save_to_bin(&g_dev_info);
    device_info_print_detail("DeviceInfo-Update");

    return PackReplyWithoutDatas(DVS_INIT_DEVCONFIG_UPDATE_Done_OK);
}

static uint32_t USB_SubDevConfig_Update(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head)
{
    (void)frame_head;

    uint8_t addr;
    uint32_t start_tick;
    uint8_t frame[RS485_RX_BUF_LEN];
    uint16_t frame_len;
    int8_t ret;
    int32_t result_code = -1;

    if (data_len != sizeof(SubDevicelnfo))
    {
        usb_printf("USB_SubDevConfig_Update err: data_len=%lu, expected=%lu\n", (unsigned long)data_len, (unsigned long)sizeof(SubDevicelnfo));
        FrameHeadInfo reply_frame_head = create_default_frame_head(0);
        UserDataHeadInfo reply_user_head = create_user_data_head(DVS_INIT_SUB_DEVCONFIG_UPDATE_Done_OK,
                                                                 SOURCE_TYPE_NO_DATA,
                                                                 DESTINATION_ARM_TO_PC,
                                                                 0);
        reply_user_head.nParameters0 = -1;

        uint32_t packet_len = 0;
        pack_data(NULL, 0, &reply_user_head, &reply_frame_head, &packet_len);
        return packet_len;
    }

    addr = (uint8_t)user_head->nParameters0;
    if ((addr < RS485_SLAVE_ADDR_MIN) || (addr > RS485_SLAVE_ADDR_MAX))
    {
        usb_printf("USB_SubDevConfig_Update err: invalid addr=%d\n", addr);
        FrameHeadInfo reply_frame_head = create_default_frame_head(0);
        UserDataHeadInfo reply_user_head = create_user_data_head(DVS_INIT_SUB_DEVCONFIG_UPDATE_Done_OK,
                                                                 SOURCE_TYPE_NO_DATA,
                                                                 DESTINATION_ARM_TO_PC,
                                                                 0);
        reply_user_head.nParameters0 = -1;

        uint32_t packet_len = 0;
        pack_data(NULL, 0, &reply_user_head, &reply_frame_head, &packet_len);
        return packet_len;
    }

    const SubDevicelnfo *sub_info = (const SubDevicelnfo *)data_in;

    usb_printf("USB_SubDevConfig_Update addr:%d \n", addr);
    rs485_subdev_clear_write_ack(addr);
    ret = rs485_send_frame(addr, DEVINFO_WRITE_REQ, (const uint8_t *)sub_info, sizeof(SubDevicelnfo));
    if (ret != 0)
    {
        usb_printf("USB_SubDevConfig_Update err: rs485_send_frame failed, ret=%d\n", ret);
        result_code = -1;
    }
    else
    {
        start_tick = HAL_GetTick();
        while ((HAL_GetTick() - start_tick) < RS485_STARTUP_SCAN_INTERVAL_MS)
        {
            if (rs485_read_raw_frame(frame, sizeof(frame), &frame_len) == 0)
            {
                rs485_parse_frame(frame, frame_len);
            }
            if (rs485_subdev_get_write_ack(addr) != 0U)
            {
                usb_printf("USB_SubDevConfig_Update ok: write ack received, addr=%d\n", addr);
                result_code = 0;
                break;
            }
        }
        if (result_code != 0)
        {
            usb_printf("USB_SubDevConfig_Update err: timeout, no write ack, addr=%d\n", addr);
            result_code = -2;
        }
    }

    FrameHeadInfo reply_frame_head = create_default_frame_head(0);
    UserDataHeadInfo reply_user_head = create_user_data_head(DVS_INIT_SUB_DEVCONFIG_UPDATE_Done_OK,
                                                             SOURCE_TYPE_NO_DATA,
                                                             DESTINATION_ARM_TO_PC,
                                                             0);
    reply_user_head.nParameters0 = result_code;

    uint32_t packet_len = 0;
    pack_data(NULL, 0, &reply_user_head, &reply_frame_head, &packet_len);
    return packet_len;
}

static uint32_t USB_SubDevConfig_Read(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head)
{
    (void)data_in;
    (void)data_len;
    (void)frame_head;

    uint8_t addr;
    uint32_t start_tick;
    uint8_t frame[RS485_RX_BUF_LEN];
    uint16_t frame_len;
    int8_t ret;
    int32_t result_code = -1;
    SubDevicelnfo read_info;

    addr = (uint8_t)user_head->nParameters0;
    if ((addr < RS485_SLAVE_ADDR_MIN) || (addr > RS485_SLAVE_ADDR_MAX))
    {
        usb_printf("USB_SubDevConfig_Read err: invalid addr=%d\n", addr);
        FrameHeadInfo reply_frame_head = create_default_frame_head(0);
        UserDataHeadInfo reply_user_head = create_user_data_head(DVS_INIT_SUB_DEVCONFIG_READ_Done_OK,
                                                                 SOURCE_TYPE_NO_DATA,
                                                                 DESTINATION_ARM_TO_PC,
                                                                 0);
        reply_user_head.nParameters0 = -1;

        uint32_t packet_len = 0;
        pack_data(NULL, 0, &reply_user_head, &reply_frame_head, &packet_len);
        return packet_len;
    }

    usb_printf("USB_SubDevConfig_Read addr:%d \n", addr);
    g_subdev_valid[addr - 1U] = 0U;
    ret = rs485_send_frame(addr, DEVINFO_READ_REQ, NULL, 0);
    if (ret != 0)
    {
        usb_printf("USB_SubDevConfig_Read err: rs485_send_frame failed, ret=%d\n", ret);
        result_code = -1;
    }
    else
    {
        start_tick = HAL_GetTick();
        while ((HAL_GetTick() - start_tick) < RS485_STARTUP_SCAN_INTERVAL_MS)
        {
            if (rs485_read_raw_frame(frame, sizeof(frame), &frame_len) == 0)
            {
                rs485_parse_frame(frame, frame_len);
            }
            if (g_subdev_valid[addr - 1U] != 0U)
            {
                usb_printf("USB_SubDevConfig_Read ok: read ack received, addr=%d\n", addr);
                result_code = 0;
                break;
            }
        }
        if (result_code != 0)
        {
            usb_printf("USB_SubDevConfig_Read err: timeout, no read ack, addr=%d\n", addr);
            result_code = -2;
        }
    }

    if (result_code == 0)
    {
        memcpy(&read_info, &g_SubDevicelnfo[addr - 1U], sizeof(SubDevicelnfo));
        FrameHeadInfo reply_frame_head = create_default_frame_head(0);
        UserDataHeadInfo reply_user_head = create_user_data_head(DVS_INIT_SUB_DEVCONFIG_READ_Done_OK,
                                                                 SOURCE_TYPE_WITH_DATAS,
                                                                 DESTINATION_ARM_TO_PC,
                                                                 sizeof(SubDevicelnfo));
        reply_user_head.nParameters0 = 0;

        uint32_t packet_len = 0;
        pack_data((uint8_t *)&read_info, sizeof(SubDevicelnfo), &reply_user_head, &reply_frame_head, &packet_len);
        return packet_len;
    }
    else
    {
        FrameHeadInfo reply_frame_head = create_default_frame_head(0);
        UserDataHeadInfo reply_user_head = create_user_data_head(DVS_INIT_SUB_DEVCONFIG_READ_Done_OK,
                                                                 SOURCE_TYPE_NO_DATA,
                                                                 DESTINATION_ARM_TO_PC,
                                                                 0);
        reply_user_head.nParameters0 = result_code;

        uint32_t packet_len = 0;
        pack_data(NULL, 0, &reply_user_head, &reply_frame_head, &packet_len);
        return packet_len;
    }
}

static uint32_t USB_SubDevId_Update(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head)
{
    (void)frame_head;
    (void)data_in;
    (void)data_len;

    uint8_t addr;
    uint32_t start_tick;
    uint8_t frame[RS485_RX_BUF_LEN];
    uint16_t frame_len;
    int8_t ret;
    int32_t result_code = -1;
    SubDevicelnfo sub_info;

    addr = (uint8_t)user_head->nParameters0;
    if ((addr < RS485_SLAVE_ADDR_MIN) || (addr > RS485_SLAVE_ADDR_MAX))
    {
        usb_printf("USB_SubDevId_Update err: invalid addr=%d\n", addr);
        FrameHeadInfo reply_frame_head = create_default_frame_head(0);
        UserDataHeadInfo reply_user_head = create_user_data_head(DVS_INIT_SUB_ID_UPDATE_Done_OK,
                                                                 SOURCE_TYPE_NO_DATA,
                                                                 DESTINATION_ARM_TO_PC,
                                                                 0);
        reply_user_head.nParameters0 = -1;

        uint32_t packet_len = 0;
        pack_data(NULL, 0, &reply_user_head, &reply_frame_head, &packet_len);
        return packet_len;
    }

    if (g_subdev_valid[addr - 1U] == 0U)
    {
        usb_printf("USB_SubDevId_Update err: subdev addr=%d not valid\n", addr);
        FrameHeadInfo reply_frame_head = create_default_frame_head(0);
        UserDataHeadInfo reply_user_head = create_user_data_head(DVS_INIT_SUB_ID_UPDATE_Done_OK,
                                                                 SOURCE_TYPE_NO_DATA,
                                                                 DESTINATION_ARM_TO_PC,
                                                                 0);
        reply_user_head.nParameters0 = -1;

        uint32_t packet_len = 0;
        pack_data(NULL, 0, &reply_user_head, &reply_frame_head, &packet_len);
        return packet_len;
    }

    memcpy(&sub_info, &g_SubDevicelnfo[addr - 1U], sizeof(SubDevicelnfo));
    sub_info.SlotId = user_head->nParameters1;

    usb_printf("USB_SubDevId_Update addr:%d, new SlotId:%d\n", addr, sub_info.SlotId);
    rs485_subdev_clear_write_ack(addr);
    ret = rs485_send_frame(addr, DEVINFO_WRITE_REQ, (const uint8_t *)&sub_info, sizeof(SubDevicelnfo));
    if (ret != 0)
    {
        usb_printf("USB_SubDevId_Update err: rs485_send_frame failed, ret=%d\n", ret);
        result_code = -1;
    }
    else
    {
        start_tick = HAL_GetTick();
        while ((HAL_GetTick() - start_tick) < RS485_STARTUP_SCAN_INTERVAL_MS)
        {
            if (rs485_read_raw_frame(frame, sizeof(frame), &frame_len) == 0)
            {
                rs485_parse_frame(frame, frame_len);
            }
            if (rs485_subdev_get_write_ack(addr) != 0U)
            {
                usb_printf("USB_SubDevId_Update ok: write ack received, addr=%d\n", addr);
                result_code = 0;
                break;
            }
        }
        if (result_code != 0)
        {
            usb_printf("USB_SubDevId_Update err: timeout, no write ack, addr=%d\n", addr);
            result_code = -2;
        }
    }

    FrameHeadInfo reply_frame_head = create_default_frame_head(0);
    UserDataHeadInfo reply_user_head = create_user_data_head(DVS_INIT_SUB_ID_UPDATE_Done_OK,
                                                             SOURCE_TYPE_NO_DATA,
                                                             DESTINATION_ARM_TO_PC,
                                                             0);
    reply_user_head.nParameters0 = result_code;

    uint32_t packet_len = 0;
    pack_data(NULL, 0, &reply_user_head, &reply_frame_head, &packet_len);
    return packet_len;
}

// 处理PC->ARM的DVSARM_CSP_START事件
static uint32_t USB_CollectChCfg_Reply(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head)
{
    (void)frame_head;
    (void)user_head;

    // 判断是否是离线模式，如果是离线模式直接返回错误
    if (g_offline_mode == 1)
    {
        usb_printf("[CollectChCfg] Rejected: device is in offline mode\r\n");
        return PackReplyWithoutDatasParam(DVSARM_CSP_START_OK, -1);
    }

    // 如果正在运行，先停止采集
    if (g_IdaSystemStatus.st_dev_run.run_flag == 1)
    {
        g_IdaSystemStatus.st_dev_run.run_flag = 0;
        AdcCollectorContrl(g_IdaSystemStatus.st_dev_run.run_flag);
        AdcCbClear();
        usb_printf("[CollectChCfg] Acquisition stopped for reconfiguration\r\n");
    }

    int userDataLoc = 0;

    // ---------------- 解析ChannelTableHeader ----------------
    if (data_len < userDataLoc + sizeof(ChannelTableHeader))
    {
        usb_printf("parseData_DVS_RUN userData too short for ChannelTableHeader \n");
        return 0;
    }

    const ChannelTableHeader *channelTableHeader = (const ChannelTableHeader *)(data_in + userDataLoc);

    usb_printf("nTotalChannelNum:%d\n", channelTableHeader->nTotalChannelNum);
    usb_printf("fHardwareSampleRate:%d\n", channelTableHeader->fHardwareSampleRate);
    usb_printf("nFlagIntDiff:%d\n", channelTableHeader->nFlagIntDiff);

    userDataLoc += sizeof(ChannelTableHeader);

    // ---------------------通道信息----------------------------
    if (data_len < userDataLoc + (sizeof(ChannelTableElem) * channelTableHeader->nTotalChannelNum))
    {
        usb_printf("parseData_DVS_RUN userData too short for ChannelTableElem \n");
        return 0;
    }

    const ChannelTableElem *channelTableElem = (const ChannelTableElem *)(data_in + userDataLoc);

    for (size_t i = 0; i < channelTableHeader->nTotalChannelNum; i++)
    {
        usb_printf("ChannelTableElem nChannelID:%d\n", channelTableElem[i].nChannelID);

        usb_printf("ChannelTableElem nInputRange:%d\n", channelTableElem[i].nInputRange);

        usb_printf("ChannelTableElem nGroupID:%d\n", channelTableElem[i].nGroupID);
    }

    userDataLoc += (sizeof(ChannelTableElem) * channelTableHeader->nTotalChannelNum);

    g_ch_enable_mask = 0;
    g_enabled_ch_cnt = 0;
    memset(g_spi_adc_cnt, 0, sizeof(g_spi_adc_cnt));

    for (size_t i = 0; i < channelTableHeader->nTotalChannelNum; i++)
    {
        int32_t ch_id = channelTableElem[i].nChannelID;
        if (channelTableElem[0].nChannelID == 1)
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

    // g_spi_adc_cnt[0] = 1;
    // g_spi_adc_cnt[1] = 1;
    // g_spi_adc_cnt[2] = 1;

    usb_printf("spi_adc_cnt: [%d, %d, %d]  mask=0x%06lX\n",
               g_spi_adc_cnt[0], g_spi_adc_cnt[1], g_spi_adc_cnt[2],
               g_ch_enable_mask);

    // // ---------------------设备详细信息----------------------------
    // if (data_len < userDataLoc + sizeof(DeviceDetailInfo))
    // {
    //     usb_printf("parseData_DVS_RUN userData too short for DeviceDetailInfo\n");
    //     return false;
    // }

    // const DeviceDetailInfo *dviceDetailInfo = (DeviceDetailInfo *)(data_in + userDataLoc);

    // usb_printf("dviceDetailInfo.SystemID:%d\n", dviceDetailInfo->SystemID);

    // userDataLoc += sizeof(DeviceDetailInfo);

    // // ---------------------全局信息-----------------------------

    // if (data_len < userDataLoc + sizeof(DSAGlobalParams))
    // {
    //     usb_printf("parseData_DVS_RUN userData too short for DSAGlobalParams\n");
    //     return false;
    // }

    // const DSAGlobalParams *dsaGlobalParams = (DSAGlobalParams *)(data_in + userDataLoc);

    // usb_printf("nBlockSize: %d\n", dsaGlobalParams->nBlockSize);
    // usb_printf("nOverlapRatio: %d\n", dsaGlobalParams->nOverlapRatio);

    // userDataLoc += sizeof(DSAGlobalParams);
    // //------------------------------------------------------

    uint32_t sample_rate = 0;
    for (uint8_t i = 0; i < (uint8_t)g_ida_ch_rate_count; i++)
    {
        if (channelTableHeader->fHardwareSampleRate == g_ida_ch_rate[i].ch_cfg_value)
        {
            sample_rate = g_ida_ch_rate[i].ch_cfg_value;
            break;
        }
    }

    // if (sample_rate > 51200)
    // {
    //     sample_rate = 51200;
    // }

    usb_printf("sample_rate:%d", sample_rate);

    if (sample_rate > 0)
    {
        uint32_t requested_rate = sample_rate;
        AdcCollectMode mode;

        sample_rate = AdcCollectorMatchSampleRate(sample_rate, g_enabled_ch_cnt);
        if (sample_rate == 0U)
        {
            usb_printf("[CollectCfg] rejected: enable_cnt=%u requested=%lu exceeds mode budget\r\n",
                       (unsigned int)g_enabled_ch_cnt,
                       (unsigned long)requested_rate);
            return PackReplyWithoutDatasParam(DVSARM_CSP_START_OK, -1);
        }

        mode = AdcCollectorSelectMode(sample_rate);
        usb_printf("[CollectCfg] enable_cnt=%u mask=0x%06lX spi_adc_cnt=[%u,%u,%u] requested=%lu actual=%lu mode=%s\r\n",
                   (unsigned int)g_enabled_ch_cnt,
                   (unsigned long)g_ch_enable_mask,
                   (unsigned int)g_spi_adc_cnt[0],
                   (unsigned int)g_spi_adc_cnt[1],
                   (unsigned int)g_spi_adc_cnt[2],
                   (unsigned long)requested_rate,
                   (unsigned long)sample_rate,
                   (mode == ADC_COLLECT_MODE_DMA) ? "DMA" : "POLL");
        CfgAdcSampleRate(sample_rate);

        /* ---------- 配置桥路子设备 ---------- */
        {
            int8_t bridge_ret;
            bridge_ret = bridge_config_all_subdevs(channelTableElem, channelTableHeader->nTotalChannelNum, sample_rate);
            if (bridge_ret != 0)
            {
                usb_printf("[CollectCfg] Bridge subdev config failed, ret=%d\r\n", bridge_ret);
                /* 桥路配置失败不阻塞采集启动，仅记录日志 */
            }
            else
            {
                usb_printf("[CollectCfg] Bridge subdev config success\r\n");
            }
        }
        /* ----------------------------------- */

        g_IdaSystemStatus.st_dev_run.run_flag = 1;
        AdcCollectorContrl(g_IdaSystemStatus.st_dev_run.run_flag);
        usb_printf("[CollectCfg] acquisition started\r\n");
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

// 处理PC->ARM的DVS_CSP_OFFLINE_SETCONFIG_OK事件
static uint32_t USB_OfflineCfg_Reply(uint8_t *data_in, uint32_t data_len,
                                     FrameHeadInfo *frame_head, UserDataHeadInfo *user_head)
{
    (void)frame_head;
    (void)user_head;

    int32_t ret = RET_OK;

    if (data_len == 0)
    {
        ret = clear_file_content(OFFLINE_SCHEDULE_FILE);
        if (ret != RET_OK)
            usb_printf("Failed to clear offline config file: %d\n", ret);
    }
    else
    {
        FIL fil;
        UINT bw;
        FRESULT fres;

        fres = f_open(&fil, OFFLINE_SCHEDULE_FILE, FA_CREATE_ALWAYS | FA_WRITE);
        if (fres != FR_OK)
        {
            usb_printf("f_open '%s' failed: %d\n", OFFLINE_SCHEDULE_FILE, fres);
            ret = -1;
        }
        else
        {
            fres = f_write_dma_safe(&fil, data_in, data_len, &bw);
            if (fres != FR_OK || bw != data_len) // 原代码未校验 bw
            {
                usb_printf("f_write failed: res=%d written=%u expected=%u\n",
                           fres, bw, data_len);
                ret = -1;
            }
            else
            {
                usb_printf("f_write '%s' ok, %u bytes written\n",
                           OFFLINE_SCHEDULE_FILE, bw);
            }

            fres = f_close(&fil); // 原代码未检查返回值
            if (fres != FR_OK)
            {
                usb_printf("f_close failed: %d\n", fres);
                ret = -1;
            }
        }
    }

    // 构造应答帧
    FrameHeadInfo reply_frame_head = create_default_frame_head(0);
    UserDataHeadInfo reply_user_head = {0};
    reply_user_head.nIsValidFlag = 0x12345678;
    reply_user_head.nEventID = DVS_CSP_OFFLINE_SETCONFIG_OK;
    reply_user_head.nSourceType = SOURCE_TYPE_NO_DATA;
    reply_user_head.nDestinationID = DESTINATION_ARM_TO_PC;
    reply_user_head.nDataLength = 0;
    reply_user_head.nNanoSecond = SoftTimeGetEpochNanosecond();
    reply_user_head.nParameters0 = (ret != RET_OK) ? -1 : RET_OK;

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
    reply_user_head.nNanoSecond = SoftTimeGetEpochNanosecond();
    reply_user_head.nParameters0 = 1;

    uint32_t packet_len = 0;
    pack_data(NULL, 0, &reply_user_head, &reply_frame_head, &packet_len);
    g_IdaSystemStatus.st_dev_mode.reset_all_flag = 1; // ready to restart
    g_reset_time = SoftTimeGetEpochNanosecond();

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
    const uint32_t cb_needed = BLOCK_LEN * ADC_DATA_LEN;

    // 没有配置通道则直接返回
    if (g_enabled_ch_cnt == 0)
    {
        return 0;
    }

    // 所有配置通道必须全部就绪，否则等下一次
    for (uint8_t i = 0; i < g_enabled_ch_cnt; i++)
    {
        uint8_t ch = g_enabled_chs[i];
        if (!g_cb_ch[ch] || cb_size(g_cb_ch[ch]) < (int)cb_needed)
        {
            return 0; // 有通道未就绪，本次不发
        }
    }

    /* 填充帧头 */
    struct UserData *user_data = g_user_data;

    frame_num++;
    user_data->data_head.nVersion = 0x12345678;
    user_data->data_head.nDataSource = 0;
    user_data->data_head.nFrameChCount = g_enabled_ch_cnt; // 固定值
    user_data->data_head.nFrameLen = BLOCK_LEN;
    user_data->data_head.nTotalFrameNum = frame_num;
    user_data->data_head.nCurNs = SoftTimeGetEpochNanosecond();

    // cb_read 之前加
    // usb_printf("frame=%lu cb0_size=%d overflow=%lu\n",
    //            frame_num,
    //            cb_size(g_cb_ch[g_enabled_chs[0]]),
    //            g_cb_overflow_cnt);

    /* 按配置顺序读取，与上位机通道顺序一致 */
    for (uint8_t n = 0; n < g_enabled_ch_cnt; n++)
    {
        uint8_t ch = g_enabled_chs[n];
        cb_read(g_cb_ch[ch], (char *)user_data->send_frame[n], cb_needed);
    }

    // USB_Display_Reply 里，cb_read 之后加：
    // usb_printf("ch0 pt0=%d pt1=%d pt2=%d pt3=%d pt4=%d\n",
    //            ((int16_t *)user_data.send_frame[0])[0],
    //            ((int16_t *)user_data.send_frame[0])[1],
    //            ((int16_t *)user_data.send_frame[0])[2],
    //            ((int16_t *)user_data.send_frame[0])[3],
    //            ((int16_t *)user_data.send_frame[0])[4]);

    // usb_printf("ch0 pt252=%d pt253=%d pt254=%d pt255=%d\n",
    //            ((int16_t *)user_data.send_frame[0])[252],
    //            ((int16_t *)user_data.send_frame[0])[253],
    //            ((int16_t *)user_data.send_frame[0])[254],
    //            ((int16_t *)user_data.send_frame[0])[255]);

    uint32_t send_len = sizeof(ArmBackFrameHeader) +
                        (uint32_t)g_enabled_ch_cnt * BLOCK_LEN * sizeof(short);

    FrameHeadInfo reply_frame_head = create_default_frame_head(frame_num);
    UserDataHeadInfo reply_user_head = create_user_data_head(
        DVSARM_DISPNEXT_OK, SOURCE_TYPE_WITH_DATAS, DESTINATION_ARM_TO_PC, send_len);

    uint32_t packet_len = 0;
    pack_data((uint8_t *)user_data, send_len,
              &reply_user_head, &reply_frame_head, &packet_len);

    return packet_len;
}

// 处理PC->ARM的DVSARM_DISPNEXT_OK事件，循环发送数据
void USB_Display_All(uint32_t run_flag)
{
    // usb_printf("DisplayAll: run=%lu offline=%d record=%d\n",
    //            run_flag,
    //            g_IdaSystemStatus.st_dev_offline.start_flag,
    //            g_IdaSystemStatus.st_dev_record.record_status);

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

#define FILELIST_BUFFER_SIZE (128 * 1024) // 16KB缓冲区，实际使用中可根据需求调整

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
    ret = get_file_list_by_extension("0:/RecordDataFiles", ".rec", &file_list, 1000);

    if (ret == 0 && file_list.count > 0)
    {
        usb_printf("Found %d files:\n", file_list.count);

        for (uint32_t i = 0; i < file_list.count; i++)
        {
            // 检查缓冲区是否足够
            uint32_t used_len = strlen((const char *)user_data);
            uint32_t remaining = FILELIST_BUFFER_SIZE - used_len - 1;

            // 格式化日期时间
            format_date_time(file_list.files[i].create_date,
                             file_list.files[i].create_time,
                             date_time_buf, sizeof(date_time_buf));

            // 估算需要空间（路径+分隔符+数字+日期+分隔符）
            uint32_t needed = strlen(file_list.files[i].path) + strlen(date_time_buf) + 50;

            if (remaining < needed)
            {
                usb_printf("Buffer exhausted at file %u\n", i);
                break;
            }

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

            strncat((char *)user_data, date_time_buf, remaining);
            remaining = FILELIST_BUFFER_SIZE - strlen((const char *)user_data) - 1;

            strncat((char *)user_data, ",", remaining);
            remaining = FILELIST_BUFFER_SIZE - strlen((const char *)user_data) - 1;

            char str[16];
            snprintf(str, sizeof(str), "%u", file_list.files[i].size);
            strncat((char *)user_data, str, remaining);
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

    op_time_start = SoftTimeGetEpochNanosecond();
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
            uint64_t op_time_end = SoftTimeGetEpochNanosecond();
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
    reply_user_head.nNanoSecond = SoftTimeGetEpochNanosecond();
    reply_user_head.nParameters0 = ret; // 删除结果：0=成功, <0=错误码

    delay_ms(100);

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
    reply_user_head.nNanoSecond = SoftTimeGetEpochNanosecond();
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
    reply_user_head.nNanoSecond = SoftTimeGetEpochNanosecond();
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
    reply_user_head.nNanoSecond = SoftTimeGetEpochNanosecond();
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
    // if ((g_IdaSystemStatus.st_dev_link.link_status != USB_IDLE) &&
    //     (g_IdaSystemStatus.st_dev_link.link_status != USB_DISCONNECTED))
    // {
    //     time_off = (time_n - heart_recv_time) / 1000;
    //     if (time_off >= TIMEOUT_30000)
    //     {
    //         // disconnect

    //         SystemStatusInit();
    //         AdcCollectorContrl(g_IdaSystemStatus.st_dev_run.run_flag);
    //         AdcCbClear();
    //     }
    // }
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

// 处理PC->ARM的DVS_INIT_ZEROING事件（留待后续模块添加）
static uint32_t USB_Zeroing_Reply(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head)
{
    (void)data_in;
    (void)data_len;
    (void)frame_head;

    int32_t result = 0;
    int32_t channel = user_head->nParameters0;

    FrameHeadInfo reply_frame_head = create_default_frame_head(0);
    UserDataHeadInfo reply_user_head = create_user_data_head(DVS_INIT_ZEROING_OK,
                                                             SOURCE_TYPE_NO_DATA,
                                                             DESTINATION_ARM_TO_PC,
                                                             0);
    reply_user_head.nParameters0 = result;
    reply_user_head.nParameters1 = channel;

    uint32_t packet_len = 0;
    pack_data(NULL, 0, &reply_user_head, &reply_frame_head, &packet_len);
    return packet_len;
}

// 处理PC->ARM的DVS_FILE_CALIBRATION_READ事件
static uint32_t USB_CalibrationRead(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head)
{
    (void)data_in;
    (void)data_len;
    (void)frame_head;
    (void)user_head;

    FIL fil;
    FRESULT fres;
    uint8_t *calib_data = NULL;
    uint32_t file_size = 0;
    UINT br;
    int32_t ret = RET_OK;

    usb_printf("[CalibrationRead] Reading calibration file: %s\r\n", CALIBRATION_FILE_PATH);

    // 打开校准文件
    fres = f_open(&fil, CALIBRATION_FILE_PATH, FA_READ);
    if (fres != FR_OK)
    {
        if (fres == FR_NO_FILE)
        {
            usb_printf("[CalibrationRead] Calibration file not found\r\n");
        }
        else
        {
            usb_printf("[CalibrationRead] f_open failed: %d\r\n", fres);
        }
        ret = -1;
        goto send_reply;
    }

    // 获取文件大小
    file_size = f_size(&fil);
    if (file_size == 0)
    {
        usb_printf("[CalibrationRead] Calibration file is empty\r\n");
        f_close(&fil);
        ret = -2;
        goto send_reply;
    }

    // 限制最大文件大小（防止内存溢出）
    if (file_size > 64 * 1024) // 最大64KB
    {
        usb_printf("[CalibrationRead] File too large: %u bytes\r\n", file_size);
        f_close(&fil);
        ret = -3;
        goto send_reply;
    }

    // 分配内存
    calib_data = (uint8_t *)mymalloc(SRAMEX, file_size);
    if (calib_data == NULL)
    {
        usb_printf("[CalibrationRead] Memory allocation failed\r\n");
        f_close(&fil);
        ret = -4;
        goto send_reply;
    }

    // 读取文件内容
    fres = f_read(&fil, calib_data, file_size, &br);
    f_close(&fil);

    if (fres != FR_OK || br != file_size)
    {
        usb_printf("[CalibrationRead] f_read failed: res=%d br=%u expected=%u\r\n", fres, br, file_size);
        myfree(SRAMEX, calib_data);
        ret = -5;
        goto send_reply;
    }

    usb_printf("[CalibrationRead] Success: read %u bytes\r\n", file_size);

    // 发送数据
    {
        FrameHeadInfo reply_frame_head = create_default_frame_head(0);
        UserDataHeadInfo reply_user_head = create_user_data_head(
            DVS_FILE_CALIBRATION_READ_OK,
            SOURCE_TYPE_WITH_DATAS,
            DESTINATION_ARM_TO_PC,
            file_size);
        reply_user_head.nParameters0 = RET_OK;

        uint32_t packet_len = 0;
        pack_data(calib_data, file_size, &reply_user_head, &reply_frame_head, &packet_len);

        myfree(SRAMEX, calib_data);
        return packet_len;
    }

send_reply:
    // 发送错误回复
    {
        FrameHeadInfo reply_frame_head = create_default_frame_head(0);
        UserDataHeadInfo reply_user_head = {0};
        reply_user_head.nIsValidFlag = 0x12345678;
        reply_user_head.nEventID = DVS_FILE_CALIBRATION_READ_OK;
        reply_user_head.nSourceType = SOURCE_TYPE_NO_DATA;
        reply_user_head.nDestinationID = DESTINATION_ARM_TO_PC;
        reply_user_head.nDataLength = 0;
        reply_user_head.nNanoSecond = SoftTimeGetEpochNanosecond();
        reply_user_head.nParameters0 = ret;

        uint32_t packet_len = 0;
        pack_data(NULL, 0, &reply_user_head, &reply_frame_head, &packet_len);
        return packet_len;
    }
}

// 处理PC->ARM的DVS_FILE_CALIBRATION_WRITE事件
static uint32_t USB_CalibrationWrite(uint8_t *data_in, uint32_t data_len, FrameHeadInfo *frame_head, UserDataHeadInfo *user_head)
{
    (void)frame_head;
    (void)user_head;

    int32_t ret = RET_OK;
    FRESULT fres;
    FIL fil;
    UINT bw;

    usb_printf("[CalibrationWrite] Writing calibration file: %s, size=%u bytes\r\n",
               CALIBRATION_FILE_PATH, data_len);

    // 确保目录存在
    fres = f_mkdir(CALIBRATION_DIR_PATH);
    // 忽略目录已存在的错误（FR_EXIST）
    if (fres != FR_OK && fres != FR_EXIST)
    {
        usb_printf("[CalibrationWrite] f_mkdir failed: %d\r\n", fres);
        ret = -1;
        goto send_reply;
    }

    if (data_len == 0)
    {
        // 数据长度为0时删除文件
        usb_printf("[CalibrationWrite] Clearing calibration file\r\n");
        ret = clear_file_content(CALIBRATION_FILE_PATH);
        if (ret != RET_OK)
        {
            usb_printf("[CalibrationWrite] Failed to clear file\r\n");
        }
    }
    else
    {
        // 写入文件
        fres = f_open(&fil, CALIBRATION_FILE_PATH, FA_CREATE_ALWAYS | FA_WRITE);
        if (fres != FR_OK)
        {
            usb_printf("[CalibrationWrite] f_open failed: %d\r\n", fres);
            ret = -2;
            goto send_reply;
        }

        fres = f_write_dma_safe(&fil, data_in, data_len, &bw);
        if (fres != FR_OK || bw != data_len)
        {
            usb_printf("[CalibrationWrite] f_write failed: res=%d written=%u expected=%u\r\n",
                       fres, bw, data_len);
            f_close(&fil);
            ret = -3;
            goto send_reply;
        }

        fres = f_close(&fil);
        if (fres != FR_OK)
        {
            usb_printf("[CalibrationWrite] f_close failed: %d\r\n", fres);
            ret = -4;
            goto send_reply;
        }

        usb_printf("[CalibrationWrite] Success: wrote %u bytes\r\n", bw);
    }

send_reply:
    // 构造应答帧
    ;
    FrameHeadInfo reply_frame_head = create_default_frame_head(0);
    UserDataHeadInfo reply_user_head = {0};
    reply_user_head.nIsValidFlag = 0x12345678;
    reply_user_head.nEventID = DVS_FILE_CALIBRATION_WRITE_OK;
    reply_user_head.nSourceType = SOURCE_TYPE_NO_DATA;
    reply_user_head.nDestinationID = DESTINATION_ARM_TO_PC;
    reply_user_head.nDataLength = 0;
    reply_user_head.nNanoSecond = SoftTimeGetEpochNanosecond();
    reply_user_head.nParameters0 = (ret != RET_OK) ? -1 : RET_OK;

    uint32_t packet_len = 0;
    pack_data(NULL, 0, &reply_user_head, &reply_frame_head, &packet_len);
    return packet_len;
}
