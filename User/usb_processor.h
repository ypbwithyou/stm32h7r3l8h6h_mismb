#ifndef __USB_PROCESSOR_H
#define __USB_PROCESSOR_H

#include <stdint.h>
#include "./LIBS/lib_usb_protocol/usb_protocol.h"
#include "collector_processor.h"
/*******************************************************************************/
// #define RS485_BAUDRATE          (115200)
#define SUBDEV_NUM_MAX (8)
#define SUBDEN_NUM (1)

// 设备型号
#define Mini_SliceMicro (250)  // mini base slice
#define Mini_SliceBridge (251) // mini sub slice, bridge
#define Mini_SliceIEPE (252)   // mini sub slice, iepe
#define Mini_SliceAccel (253)  // mini sub slice, accelerometer
#define Mini_SliceARS (254)    // mini sub slice, ars
#define Mini_SliceNano (261)   // mini nano slice

// 设备序列号类型定义
#define SerialNum_Mini_SliceMicro "MISMB"  // mini base slice
#define SerialNum_Mini_SliceBridge "MIS3B" // mini sub slice, bridge
#define SerialNum_Mini_SliceIEPE "MIS3I"   // mini sub slice, iepe
#define SerialNum_Mini_SliceAccel "MIRA3"  // mini sub slice, accelerometer
#define SerialNum_Mini_SliceARS ""         // mini sub slice, ars
#define SerialNum_Mini_SliceNano ""        // mini nano slice

// 设备名称定义
#define Name_Mini_SliceMicro "MISMB"  // mini base slice
#define Name_Mini_SliceBridge "MIS3B" // mini sub slice, bridge
#define Name_Mini_SliceIEPE "MIS3I"   // mini sub slice, iepe
#define Name_Mini_SliceAccel "MIRA3"  // mini sub slice, accelerometer
#define Name_Mini_SliceARS ""         // mini sub slice, ars
#define Name_Mini_SliceNano ""        // mini nano slice

// 设备信息默认值
#define DEFAULT_VERSION "0.0.0.1_20260301"
#define DEFAULT_ACCESSCODE "NTS2026"
#define DEFAULT_ID (0)
#define DEFAULT_SERIALNUMBER "MISMB102501001"
#define DEFAULT_DEVICENAME "MISMB"
#define DEFAULT_DEVICETYPE Mini_SliceMicro
#define DEFAULT_LASTUPDATETIME "2026-03-01 00:00:00"

#define KEY_VALUE_MAX_LEN (32)

/* device_info.ini */
#define IDA_DEVICE_INFO_FILE "../ini/device_info.ini"
#define INI_SECTION_DEVICE_INFO "DeviceInfo"
#define INI_KEY_AccessCode "AccessCode"
#define INI_KEY_ID "ID"
#define INI_KEY_SerialNumber "SerialNumber"
#define INI_KEY_DeviceName "DeviceName"
#define INI_KEY_DeviceType "DeviceType"
#define INI_KEY_SoftwareVersion "SoftwareVersion"
#define INI_KEY_DriverVersion "DriverVersion"
#define INI_KEY_PlogicVersion "PlogicVersion"
#define INI_KEY_FlogicVersion "FlogicVersion"
#define INI_KEY_DspVersion "DspVersion"
#define INI_KEY_SystemID "SystemID"
#define INI_KEY_SystemName "SystemName"
#define INI_KEY_IsMaster "IsMaster"
#define INI_KEY_MasterID "MasterID"
#define INI_KEY_AIChannelNum "AIChannelNum"
#define INI_KEY_AOChannelNum "AOChannelNum"
#define INI_KEY_DIChannelNum "DIChannelNum"
#define INI_KEY_DOChannelNum "DOChannelNum"
#define INI_KEY_TachoChannelNum "TachoChannelNum"
#define INI_KEY_GPSNum "GPSNum"
#define INI_KEY_HighestSamplingFreq "HighestSamplingFreq"
#define INI_KEY_ModeType "ModeType"
#define INI_KEY_LastUpdateDateTime "LastUpdateDateTime"

/*******************************************************************************/
enum usb_status
{
    USB_IDLE = 0,
    USB_INIT,
    USB_CONNECTED,
    USB_DISCONNECTED
};
typedef struct
{
    float ch_cfg_value;
    uint32_t ch_set_index;
} Dev_ch_cfg_index;
struct UserData
{
    ArmBackFrameHeader data_head;
    short send_frame[SPI_NUM][BLOCK_LEN];
};
struct OffsetUserData
{
    AoLocalColumn data_head;
    short send_frame[SPI_NUM][BLOCK_LEN];
};
// 主卡设备信息
extern DeviceInfo g_dev_info;
// 子卡设备信息
extern SubDevicelnfo g_SubDevicelnfo[SUBDEV_NUM_MAX];
/*******************************************************************************/
void usb_init(void);

void SystemStatusInit(void);

void USB_Display_All(uint32_t run_flag);

void IdaProcessor(void);
void transpose(short src[BLOCK_LEN][SPI_NUM], short dst[SPI_NUM][BLOCK_LEN]);

int read_device_info_file(DeviceInfo *dev_info_data);
int write_device_info_file(DeviceInfo dev_data);
int update_device_info_from_detail(DeviceDetailInfo dev_detail);
void GetDeviceInfo(DeviceInfo *dev_info_data);
void on_frame(uint8_t *frame, uint32_t frame_len);

#endif /* __USB_PROCESSOR_H */
