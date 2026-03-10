#ifndef __LIB_USB_PROTOCOL_H
#define __LIB_USB_PROTOCOL_H

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>


/******************************************************************************
* Macros.
******************************************************************************/
#define CPLD_ENABLE         (0)

#define RET_OK              (0)
#define RET_ERROR           (-1)

#define STR_04              (4)
#define STR_08              (8)
#define STR_16              (16)
#define STR_32              (32)
#define STR_64              (64)
#define STR_128             (128)
#define STR_256             (256)
#define STR_512             (512)
#define STR_1024            (1024)

#define NtoH_or_HtoN	(0)
#define TRANSFER_DIRECT	(1)

// #define FRAME_DATA_MAX      (1460)

#define USER_NAME			"root"
#define USER_PASSWORD		"root"
#define USER_PATH			"/home/root"

#define FRAME_HEAD          (0x3C3C3C3C)			// 帧头"<<<<"
#define FRAME_END           (0x3E3E3E3E)			// 帧尾">>>>"

// 包头数据长度（包括帧头及帧属性数据，不包含帧头和用户数据长度）
#define FREAM_HEAD_LENGTH        (sizeof(FrameHeadInfo))        // data length for frame_head
#define FREAM_LEN_LENGTH         (sizeof(uint32_t))         	// data length for fream_data_length
#define FREAM_CHECK_LENGTH       (sizeof(int32_t))         		// data length for fream_check
#define FREAM_END_LENGTH         (sizeof(uint32_t))         	// data length for fream_end
// 帧头(12bytes)+用户数据长度(4bytes)+校验(2bytes)+帧尾(2bytes)
#define FRAME_MIN_SIZE           (FREAM_HEAD_LENGTH \
                                 + FREAM_LEN_LENGTH \
                                 + FREAM_CHECK_LENGTH \
                                 + FREAM_END_LENGTH)
// tag length
#define FREAM_TAG_LENGTH         (sizeof(int32_t))         		// data length for envent ID length
// 用户数据有效校验码
#define USER_DATA_VALID_CHECK_FLAG  (0X12345678)
#define USER_DATA_FREAM_CHECK		(0X7654321)
#define PROTOCOL_VERSION            (0x10)      				// 协议版本

#define DMA_BD_LEN					(2*1024*1000)	// DMA单个BD设置长度
#define FRAME_USER_DATA_MAX_SIZE    (15*DMA_BD_LEN)	// DSP上传的用户数据最大长度
#define DMA_LEN_MAX					(25*DMA_BD_LEN)	// DMA最大可读写长度

#define CH_FREQ_MAX         (204800)
#define TIME_BAG            (50)
#define DATA_BAG_LEN_MAX    (FRAME_USER_DATA_MAX_SIZE / sizeof(int)) // 原始数据包最大长度，单位int32_t
#define DATA_CH_NUM_LEN     (1)     // 待发送数据包，通道号长度（所占数据个数）
#define DATA_CH_DATA_LEN    (1)     // 待发送数据包，数据长度长度（所占数据个数）
#define DATA_CH_TIME_LEN    (2)     // 待发送数据包，时戳长度（所占数据个数）
#define DATA_CH_PACKAGE_LEN (1)     // 待发送数据包，包序号（所占数据个数）
#define DATA_CH_LEN         (CH_FREQ_MAX * TIME_BAG / 1000) // 单通道纯采集数据最大长度，单位int32_t
#define DATA_CH_LEN_MAX     (DATA_CH_NUM_LEN + DATA_CH_DATA_LEN + CH_FREQ_MAX * TIME_BAG / 1000 + DATA_CH_TIME_LEN + DATA_CH_PACKAGE_LEN) // 单通道数据最大长度，单位int32_t
/******************************************************************************
* Variables.
******************************************************************************/

// 使用1字节对齐
#pragma pack(push, 1)

// 帧头结构体
typedef struct 
{
    uint32_t    hd_head;                // 帧头（0x3c3c3c3c）
    uint8_t     hd_attribute_ver;       // 帧头属性——版本(主+次)
    uint8_t     hd_attribute_flag;      // 帧头属性——标志
    uint8_t     hd_data_status;         // 数据属性
    uint8_t     hd_reserved1;           // 预留数据1
    uint16_t    hd_serial_num;          // 包序号
    uint8_t     hd_reserved2;
    uint8_t     hd_reserved3;
    uint8_t     hd_reserved4;
    uint8_t     hd_reserved5;
    uint8_t     hd_reserved6;
    uint8_t     hd_reserved7;
    int32_t     hd_reserved8;
} FrameHeadInfo;

typedef struct 
{
    int32_t data_max;
    int32_t data_min;
}NormalDataRange;

typedef struct 
{
    FrameHeadInfo  fraem_head;      	// 帧头
    uint32_t  fream_data_length;    	// 帧数据长度
    uint32_t  fream_check;          	// 帧校验
    uint32_t  fream_end;            	// 帧尾（0X3E3E3E3E）
}FrameStructInfo;     // 帧数据结构

typedef struct 
{
    int     net_status;     			// 数采板网络dhcp状态，0：disable;1：enable
    int     net_ip_type;    			// ip类型，4：IPV4；6：IPV6
    int     net_ip[STR_04];    			// ip，整型
    int     net_mask;       			// 掩码
    int     net_gateway;    			// 网关
    int     net_tcp_port;				// 端口号
}NetClientCfgInfo;

typedef struct
{
    int     net_status;		    		// Udp服务启用状态，0：disable;1：enable
    int     net_udp_tx_port;			// udp通信数据发送端口(客户端)
    int     net_udp_rx_port;			// udp通信数据接收端口(客户端)
}NetUdpCfgInfo;

typedef struct 
{
	int		wifi_status;
	char    username[STR_32];
	char 	password[STR_32];
}NetWifiCfgInfo;

typedef struct 
{
    uint32_t    log_max;
    uint32_t    coredump_max;
}LogMaxInfo;

typedef struct 
{
    uint32_t threshold_cfg_type;   		// 阈值配置类型 --> ThresholdCfgTag    
    NormalDataRange threshold_info;		// 告警阈值
}NormalThresholdCfgInfo;

typedef struct 
{
    int   ch_num;      					// 通道号，从0开始
    float ch_gain;     					// 通道增益
    float ch_offset;   					// 通道偏执
}ChAdjust;

typedef struct 
{
    uint32_t time_s;
    uint32_t time_us;
}SystemTime;

typedef struct
{
    uint16_t normal_reply;
}NorReply;

/********************************************** 
* 帧ID : EventsId
**********************************************/
enum EventsId
{
    EVTSEG_INIT                         = 0x1000,
    EVTSEG_RUN                          = 0x2000,
    EVTSEG_CSP                          = 0x3000,
    EVTSEG_DISP                         = 0x4000,
    EVTSEG_PREPROC                      = 0x5000,
    EVTSEG_FILE                         = 0x6000,

    DVS_INIT_DETECT                     = (EVTSEG_INIT + 1),        // 0x01：MCU-->PC：
    DVS_INIT_DETECT_OK                  = (EVTSEG_INIT + 2),        // 0x02：
    DVS_INIT_CONNECT                    = (EVTSEG_INIT + 3),        // 0x03：MCU<--PC：
    DVS_INIT_CONNECT_OK                 = (EVTSEG_INIT + 4),        // 0x04：MCU-->PC：
    DVS_INIT_DISCONNECT                 = (EVTSEG_INIT + 5),        // 0x05：MCU<--PC：
    DVS_INIT_DISCONNECT_OK              = (EVTSEG_INIT + 6),        // 0x06：MCU-->PC：
    DVS_INIT_DEVCONFIG_UPDATE_Done		= (EVTSEG_INIT + 9),        // 0x00：PC->ARM 
    DVS_INIT_DEVCONFIG_UPDATE_Done_OK	= (EVTSEG_INIT + 10),       // 0x0a：ARM->PC 
    DVS_INIT_CONNECT_DETECT             = (EVTSEG_INIT + 11),       // 0x0b：MCU<--PC：心跳
    DVS_INIT_CONNECT_DETECT_OK          = (EVTSEG_INIT + 12),       // 0x0c：MCU-->PC：
    DVS_RUN_ERROR_OK                    = (EVTSEG_RUN + 10),        // 0x0a：
    DVS_READ_TEDS_INFO_OK               = (EVTSEG_RUN + 12),        // 0x0c：
    
    DVS_CSP_OFFLINE_GETCONFIG           = (EVTSEG_CSP + 37),        //获取离线模式的配置参数
    DVS_CSP_OFFLINE_GETCONFIG_OK        = (EVTSEG_CSP + 38),        
    DVS_CSP_OFFLINE_SETCONFIG           = (EVTSEG_CSP + 39),        //设置离线模式的配置参数
    DVS_CSP_OFFLINE_SETCONFIG_OK        = (EVTSEG_CSP + 40),        
    
    DVS_INIT_ARM_UPGRADE_REQ            = (EVTSEG_INIT + 51),       // 0x33：MCU<--PC：
    DVS_INIT_ARM_UPGRADE_REQ_OK         = (EVTSEG_INIT + 52),       // 0x34：MCU-->PC：
//    DVS_INIT_ARM_UPGRADE_DONE           = (EVTSEG_INIT + 53),       // 0x35：MCU<--PC：
//    DVS_INIT_ARM_UPGRADE_DONE_OK        = (EVTSEG_INIT + 54),       // 0x36：MCU-->PC：
//    DVS_INIT_ARM_UPGRADE_PACK           = (EVTSEG_INIT + 55),       // 0x37：MCU<--PC：
//    DVS_INIT_ARM_UPGRADE_PACK_OK        = (EVTSEG_INIT + 56),       // 0x38：MCU-->PC：
    DVS_INIT_READ_TEDSINFO              = (EVTSEG_INIT + 71),       // 0x47：MCU<--PC：
    DVS_INIT_READ_TEDSINFO_OK           = (EVTSEG_INIT + 72),       // 0x48：MCU-->PC：
    
    DVS_FILE_GET_FILELIST               = (EVTSEG_FILE + 31),     // 0x1f：获取记录文件列表
    DVS_FILE_GET_FILELIST_OK            = (EVTSEG_FILE + 32),     // 0x20：
    DVS_FILE_DELETE                     = (EVTSEG_FILE + 33),     // 0x21：删除指定文件
    DVS_FILE_DELETE_OK                  = (EVTSEG_FILE + 34),     // 0x22：
    DVS_FILE_DOWNLOAD_START             = (EVTSEG_FILE + 35),     // 0x23：开始下载指定记录文件
    DVS_FILE_DOWNLOAD_START_OK          = (EVTSEG_FILE + 36),     // 0x24：
    DVS_FILE_DOWNLOAD_PAUSE             = (EVTSEG_FILE + 37),     // 0x25：
    DVS_FILE_DOWNLOAD_PAUSE_OK          = (EVTSEG_FILE + 38),     // 0x26：
    DVS_FILE_DOWNLOAD_CONTINUE          = (EVTSEG_FILE + 39),     // 0x27：
    DVS_FILE_DOWNLOAD_CONTINUE_OK       = (EVTSEG_FILE + 40),     // 0x28：
    DVS_FILE_DOWNLOAD_STOP              = (EVTSEG_FILE + 41),     // 0x29：终止文件下载
    DVS_FILE_DOWNLOAD_STOP_OK           = (EVTSEG_FILE + 42),     // 0x2a：

    DVS_FILE_DOWNLOAD_DATA              = (EVTSEG_FILE + 43),     // 0x2b：MCU->PC 下载数据分包
    DVS_FILE_DOWNLOAD_DATA_ACK          = (EVTSEG_FILE + 44),     // 0x2c：PC->MCU 数据包确认/要求重传

    ARM_EVTSEG_INIT                     = 0x10000,
    ARM_EVTSEG_RUN                      = 0x20000,
    ARM_EVTSEG_CSP                      = 0x30000,
    ARM_EVTSEG_DISP                     = 0x40000,

    DVSARM_RUN                          = (ARM_EVTSEG_RUN + 1),    // 0x01：MCU<--PC：
    DVSARM_RUN_OK                       = (ARM_EVTSEG_RUN + 2),    // 0x02：MCU-->PC：
    DVSARM_PAUSE                        = (ARM_EVTSEG_RUN + 3),    // 0x03：
    DVSARM_PAUSE_OK                     = (ARM_EVTSEG_RUN + 4),    // 0x04：
    DVSARM_CONTINUE                     = (ARM_EVTSEG_RUN + 5),    // 0x05：
    DVSARM_CONTINUE_OK                  = (ARM_EVTSEG_RUN + 6),    // 0x06：
    DVSARM_STOP                         = (ARM_EVTSEG_RUN + 7),    // 0x07：MCU<--PC：
    DVSARM_STOP_OK                      = (ARM_EVTSEG_RUN + 8),    // 0x08：MCU-->PC：
    DVSARM_CSP_START                    = (ARM_EVTSEG_CSP + 1),    // 0x01：MCU<--PC：
    DVSARM_CSP_START_OK                 = (ARM_EVTSEG_CSP + 2),    // 0x02：MCU-->PC：
    DVSARM_DISPNEXT                     = (ARM_EVTSEG_DISP + 1),   // 0x01：MCU<--PC：
    DVSARM_DISPNEXT_OK                  = (ARM_EVTSEG_DISP + 2),   // 0x02：MCU-->PC：
};

//FrameData_head structure : FrameData_head
typedef struct UserDataHeadInfo
{
    uint32_t    nIsValidFlag;               //Checkifvalid
    uint32_t    nEventID;
    uint32_t    nSourceType;                /* 消息的类型，消息，带数据的消息
                                                0：无数据
                                                1：有数据
                                                2：通知帧无数据，通知下一帧带数据，携带数据长度在nDataLength中 */
    uint32_t    nDestinationID;             // 消息的传递源和目标
    int32_t     nParameters0;               // nSourceType=0，即无数据时，存放错误码
    int32_t     nParameters1;
    int32_t     nParameters2;
    int32_t     nParameters3;
    int32_t     nDataLength;                // 实际用户数据长度
    uint32_t    DataFlag;                   // 数据包标识
    long long 	nNanoSecond;
}UserDataHeadInfo;

////FrameData_data structure : FrameData_data_DevDiscover
//typedef struct DeviceInfo
//{ 
//    int8_t      Version[STR_32];                // 软件版本号：xx.xx.xx.xx_20xxxxxx
//    int8_t      Name[STR_32];                   // 设备名称:hunter mini slice
//    int8_t      AccessCode[STR_32];             // 设备类别:NTS2026
//    int8_t      SerialNumber[STR_32];           // 设备序列号
//    int32_t     DeviceType;                     // 设备类型:
//    int32_t     IsConnected;                    // 连接状态
//    int32_t     Reserved[8];
//    uint16_t    SubDeviceNum;                   // Reserved1改为子卡个数
//    int8_t      Reserved2[6];
//}DeviceInfo;

//FrameData_data structure : FrameData_data_DevDiscover
typedef struct DeviceInfo
{ 
    int8_t      Version[STR_32];                // 软件版本号：xx.xx.xx.xx_20xxxxxx
    int8_t      DeviceName[STR_32];             // 设备名称:hunter mini slice
    int8_t      AccessCode[STR_32];             // 设备类别:NTS2026
    int8_t      SerialNumber[STR_32];           // 设备序列号
    int32_t     DeviceType;                     // 设备类型:
    int32_t     IsConnected;                    // 连接状态
    uint16_t    AvailablePcNum;
    uint16_t    SubDeviceNum;                   //  
    int16_t     ID;
    int16_t     ClientCount;
    int32_t     IsMaster;
    int32_t     ModeType;
    
    int8_t     LastUpdateDateTime[STR_32];
    int8_t     LastCalibrationDateTime[STR_32];
    float      fTotalDiskSapce;                 //磁盘空间大小,单位Kb
    float      fFreeDiskSpace;                  //磁盘空间剩余大小,单位Kb
    int32_t    Reserved[8];
}DeviceInfo;

// DeviceDetailInfo,DeviceInfo 
typedef struct tagSubDevicelnfo
{
    int8_t      SerialNumber[32];          // 序列号
    int8_t      DeviceName[32];            // 设备名称
    int32_t     DeviceType;                 // 设备类型
    int32_t     SlotId;                     // 子板序号
    int8_t      Version[24];               // 软件版本号：xx.xx.xx.xx_20xxxxxx
    uint8_t     Sensitivity;                // 传感器灵敏度索引（仅MIRA3）
                                            /* 0：无
                                               1：50.0
                                               2：25.0
                                               3：12.5
                                               4：6.3
                                               5：2.50
                                               6：1.25
                                               7：0.63 */
    int8_t      Reserved[11];               // 预留
} SubDevicelnfo;

typedef struct tagChannelTableHeader
{
    int32_t nTotalChannelNum;       // 总启用通道个数，用于解析每个通道参数
    float   fHardwareSampleRate;    // 默认采样率10240
    int32_t nFlagIntDiff;

    int32_t nReserved1;             //
    int32_t nReserved2;             // Deciamtion stage in use
    int32_t nReserved3;
} ChannelTableHeader;

typedef struct tagChannelTableElem
{
    int32_t nChannelID;             // Channel ID from,start 0
    int32_t nInputRange;            /* Input range index 输入通道量程
                                        0: 10V、
                                        1: 3.16V、
                                        2: 1V、
                                        3: 316mV、
                                        4: 100mV、
                                        5: 10mv */
    int32_t nChannelStatus;         /* low 7 bit indicates the all kinds of channel status
                                        bit 0: 1:单端输入   0: 差分输入           信号输入模式
                                        bit 1: 1:DC耦合     0: AC耦合          耦合方式
                                        bit 2: 1:校准开启      0: 校准关闭           校准状态
                                        bit 3: 1:TEDs开启      0: TEDs关闭          TEDS状态
                                        bit 4: 1:ICP开启      0: ICP关闭           ICP状态
                                        bit 5: 1:9.5mA ICP    0: 4.5mA ICP     ICP电流
                                        bit 6: 1:100kΩ        0: 1mΩ             输入阻抗
                                        bit 7: 1:通道使能     0: 通道不使能        通道状态 
                                        bit 8: 1:外部校准类型    0: 内部校准类型     bit2校准开启时有效
                                        bit 9: 1:电荷输入开启     0: 电荷输入关闭        注意：电荷的开启与ICP开启互斥
                                        bit 10:  1: 外部电荷输入    0: 内部电荷输入*/
    int32_t    nGroupID;            /* 通道分组类型：
                                        0: 振动(ICP),
                                        1: 转速(电压),
                                        2: 应变, */
    float    fSampleRateIndex;      // 通道采样率
    float    fHighPassFreq;         // 高通截止频率
    float    fCalibGain;            // 校准增益
    float    fCalibOffset;          // 校准偏移
    char    Unit[28];               // 测量单位，由物理量纲决定
    int32_t    UnitId;
    char    Quantity[28];           // 测量的物理量纲
    int32_t QuantityId;
    float    fSensitivity;          // 灵敏度，来源于传感器属性
    float    fSensorRange;          // 传感器测量范围，来源于传感器属性
    float    fFlagIntDiff;          /* Differentiation & Intergration Flag
                                        0 -> Null
                                        1 -> Differentiation
                                        2 -> Double Differetiation
                                        3 -> Integrationlow
                                        4 -> Double Integrationlow */
    int32_t nChannelType;           /* 通道类型，包括：0=控制/激励，1=控制+限制，2=响应，3=响应+限制
                                        0: Control only
                                        1: Control+Limit
                                        2: Monitor only
                                        3: Monitor+Limit
                                        4: COLA */
    float   fReserved1;             // TachoEdgeValue = 1;//每转脉冲值
                                    // 电荷输入开启时，电荷输入的增益，单位：mV/pC

    float   fReserved2;             /* 低通截止频率
                                        00: direct
                                        01: 20kHz 
                                        10: 10kHz 
                                        11: 1kHz*/
    float   fReserved3;             // 
    float   fReserved4;             // 
    int32_t nBoolVarStored;         // nChannelStatus的扩展位
    int32_t nSensorType;            // 传感器类型， 物理量纲是温度时，1: PT1000, 0: PT100，其它：热电偶
    float     fWeighting;
    int32_t nG1oba1Physica1I;       // 在整个系统(所有设备)里面的唯一ID，从0开始

    int32_t  nReserved4;
    float    fRealSampleRate;       // 真实的采样率，FPGA不支持则DSP降采，即界面显示的采样率
    float    fReserved5;

    /* nGroupID不同，则nVar定义不同*/
    int32_t    nVar1;               // 应力应变：bit15--bit0 --> 桥路类型索引nBridgeType（1-8），bit31--bit16 --> 分流校准类型（0--10）;
                                    // 当nGroupID=1，且nVar1=1时，表示使用专用通道测转速 20240125
    int32_t    nVar2;               // 应力应变：线制索引nQBMode;0=无线制,1=2线制,2=3线制,3=4线制,4=6线制
    int32_t    nVar3;               // 应力应变：平衡电阻fGaugeR;0,120,350,1000
                                    // 转速：触发方式，0：Rising, 1: Falling
    int32_t    nVar4;               // 应力应变：激励方式和系数nExTypeValue，第17位是激励方式：0(电压激励)、1(电流激励)；低16位是激励系数：0--65535之间的一个整数值
    int32_t    nVar5;               // 转速：Tacho：PowerMode，0：5V, 1：10V
    int32_t    nVar6;               // IVS16/IVS8D：恒流源时，1：push-push模式，0：push-pull模式，-1：未设置
    int32_t    nVar7;
    int32_t    nVar8;

    float    fVar1;                 // 应力应变：灵敏度系数fGaugeFator;其他用途
    float    fVar2;                 // 应力应变：最小应变值fMinStrain
    float    fVar3;                 // 应力应变：最大应变值fMaxStrain
    float    fVar4;                 // 应力应变：泊松比fPoissonRatio
    float    fVar5;                 // 应力应变：弹性模量fYoungsFactor
    float    fVar6;                 // 应力应变：导线电阻fLeadR
    float    fVar7;                 // 应力应变：分流器的电阻fShuntR
    float    fVar8;                 // 应力应变：温度通道，分固定值和通道索引，通道索引为-50000到-51000    
    float    fVar9;                 // 应力应变：参考温度            
    float    fVar10;                // 应力应变：灵敏度系数温度变化率K，
    float    fVar11;                // 应力应变：热膨胀系数
    float    fVar12;                // 应力应变：材料的热膨胀系数
    float    fVar13;                // 应力应变：多项式系数1
    float    fVar14;                // 应力应变：多项式系数2
    float    fVar15;                // 应力应变：多项式系数3
    float    fVar16;                // 应力应变：多项式系数4
    float    fVar17;                // 应力应变：多项式系数5
    float    fVar18;                // 应力应变：
    float    fVar19;                // 应力应变：
    float    fVar20;                // 应力应变：

    char     Label[STR_16];
} ChannelTableElem;

typedef struct tagDSAGlobalParam
{
    int32_t nBlockSize;
    int32_t nWindowType;
    int32_t nAverageType;           // 0:linear //1:exponential //2:peak_hold, //3:linear_exponential
    int32_t nAverageNum;    
    int32_t nOverlapRatio;          //    0:    no overlap
                                    //    1:    25%
                                    //    2:    50%
                                    //    3:    75%
                                    //    4:    87.5%
                                    //    5:    95%
    int32_t nAcqMode;               //    0:    Free Run
                                    //    1:    trigger
                                    //    2:     time
    int32_t nScheduleCount;         //    change by yanglk 20221123
    int32_t nMeasureType;           // project nMeasureType
    int32_t nDigitInputId;          // Digit input id  -1:Disable
    int32_t nDigitTriggerType;      // Digit trigger type       0:LH,//low high, 1:HL,//high low
    float   fWinParam0;
    float   fWinParam1;
    float   fWinParam2;
    float   fDITriggerInterval;     // DigitInput trigger Interval
    int32_t nOctaveType;
                                    // 0:Linear
                                    // 1:Log
                                    // Default:1
                                    // Default:1
    int32_t nLogAnalysis;
    float   fLowFreq;               // 5, Edit(Hz)
                                    // default: 5
                                    // Minimum is 0.1 Hz, Maximum is High Frequency
    float   fHighFreq;              // 1000, Edit(Hz)
                                    // default: 1000
                                    // Minimum is Low Frequency, Maximum is 20 kHz
    float   fReferenceFreq;         // Between High and low freq
    int16_t nUpLoadMode;            // 0，来回模式，1，只发一次，持续模式
    int16_t nReserve;
    float   fEdgeValue;             // default 1.0
    float   fPulsePerRev;           // default 1.0
    float   fSLMRefreshInt;         // for SLM, block signal refresh interval;
    int32_t nRMSPeakSampleRate;     //
    int32_t nFFTAvgOff;             // 0:FFT average on, 1:FFT average off
    int32_t nAcqDeciUsedFlag;       // for DSA, Acquisition module decimation stage used flag. 0:not used(default); 1:used
    float   fLowPassCutoffFreq;     // Range 0Hz-Fa,default 0: off
    int32_t nSignalCount;
    int32_t nReserved[13];
} DSAGlobalParams;

typedef struct tagScheduleParams
{
    /*schedule type, Sine 计划类型  0:Swept Sine  1 : Step Sine  2 : Dwell Sine, 3, 4 : TrackedSine(TrackedAmplitudeSine(3) & TrackedphaseSine(4))
     * 5：HighCycleFatigue
     */
    int32_t nType;
    int32_t nRes;

    float   param0;                 // Random = Schedule running time,Sine sweept = Schedule running time, step Sine = time, Dwell Sine = time, TrackedSine = time, HighCycleFatigue = time
    int32_t param1;                 // Random = control type, Swept Sine = SweptDirection 0:Up ，1:Down, Shock = control type,SRS = control type,RoadLoad = control type
    int32_t param2;                 // Random = Relative running time,  Sine sweept / step Sine / Dwell Sine /TrackedSine = Level type 0:dB,1:%,2:物理量纲
    int32_t param3;                 // Swept Sine = SweptFixTime or SweptFixRate , step Sine = StepTime, Dwell Sine = StepTime, Shock = Number of pulses,HighCycleFatigue = StepTime,RoadLoad = Cycle count
    int32_t param4;                 // Swept Sine = HoldLevel,Step Sine = CalcTHD, Dwell Sine = CalcTHD
    int32_t param5;                 // Sine sweept = SweptType, step Sine = SweptType, TrackedSine = SweptType, HighCycleFatigue = SweptType 0:Log2, 1:Log10, 2:linear
    int32_t param6;                 // Sine sweept = OpenLoop 0:CloseLoop,1:OpenLoop, TrackedSine = 选项(Bit0:共振变化速率,Bit1:幅值量级变化 停止驻留条件) ，HighCycleFatigue: Bit0:步进高周疲劳
    int32_t param7;                 // TrackedSine = FRF Index，HighCycleFatigue = FRF Index
    int32_t param8;                 // TrackedSine = 容差滞后(帧)
    int32_t param9;                 // TrackedSine = Amp 搜索最优振幅 0：不搜索，1：搜索
    float   fparam0;                // Random fLevel, Sine sweept type = LevelParcent, Dwell Sine = LevelParcent, TrackedSine = LevelParcent, HighCycleFatigue = LevelParcent
    float   fparam1;                // Sine sweept type = MinFreq, TrackedSine = MinFreq, HighCycleFatigue = MinFreq
    float   fparam2;                // Sine sweept type =StartFreq, step Sine = StartFreq, TrackedSine = StartFreq, HighCycleFatigue = StartFreq
    float   fparam3;                // Sine sweept type= EndFreq , step Sine = tEndFreq, TrackedSine = EndFreq, HighCycleFatigue = EndFreq
    float   fparam4;                // Sine sweept type = SweepSpeed, TrackedSine = 扫频速率, HighCycleFatigue = 扫频速率
    float   fparam5;                // Swept Sine = Sweep Times, TrackedSine = 跟踪带宽 Hz, HighCycleFatigue = 跟踪带宽 Hz
    float   fparam6;                // Swept Sine = OpenLoop level, TrackedSine = 跟踪相位, HighCycleFatigue = 频率容差 %
    float   fparam7;                // TrackedSine = 共振频率变化率上限 Hz/min, HighCycleFatigue = 步进值 mHz，MimoSine(Step Sine和Sweep Sine): 初始相位
    float   fparam8;                // TrackedSine = 幅值量级变化 [dB], HighCycleFatigue = 最大相位容差deg
    float   fparam9;                // TrackedSine = 幅值容差(驻留跟踪[0~100%] 相位跟踪(°)[0.01~20])，HighCycleFatigue = 平均相位容差deg
    float   fparam10;               // TrackedSine = Quality factor, HighCycleFatigue = Quality factor
    float   fparam11;               // TrackedSine = Amp, HighCycleFatigue = Amp
    float   fparam12;               // TrackedSine = Record interval,HighCycleFatigue = Record interval
    // 幅值跟踪/相位跟踪/高周疲劳 计划以周期数控制 0，1
    int32_t nParam10;
    // 幅值跟踪/相位跟踪/高周疲劳 计划以周期数
    long long nParam11;
    float   fparam13;               // TrackedSine = 跟踪速率, HighCycleFatigue = 跟踪速率
    int32_t nReserved[3];
} ScheduleParams;



// 显示数据头
typedef struct ArmBackFrameHeader
{
    int nVersion;                       // 版本号
    int nDataSource;                    // 帧数据来源,0:模拟数据;1:外设数据
    int nFrameChCount;                  // 当前帧的通道数量
    int nFrameLen;                      // 每个帧每个通道的数据长度，float 数据长度
    unsigned long long nTotalFrameNum;  // ARM发的总帧数，计数值
    unsigned long long nCurNs;          // 当前纳秒时间
    int nReserve[8];
} ArmBackFrameHeader;


// 文件记录：文件头信息
typedef struct tagRecordHeader
{   
    int         nVersion;       // 版本号
    int         nReserved0; 
    char        Name[STR_32];   // 文件名
    long long   nCreateTime;    // 文件创建时间
    uint32_t    nDeviceChNum;   // 设备通道数量
    uint32_t    nRecordNum;     // 记录通道数量
    uint32_t    nFrameNum;      // 记录数据帧数量
    uint32_t    nRecordMask;    // 记录通道标识：
                                    // bit-x: channel x record enable
    int         nReserved[STR_128];
} tagRecordHeader;

// 文件记录：文件记录状态
typedef struct tagDeviceRecordStatus
{
    uint32_t 	nVersion;                   // 版本号
    int 		nStatus;                    // 1，状态中，2，记录完成，<0 异常状态，异常类型待定
    char 		SerialNumber[STR_32];       // 设备序列号
    char 		Name[STR_32];               // 记录文件名称
    uint32_t 	nDeviceChNum;               // 设备通道数量
    uint32_t 	nFrameNum;	                // 当前记录数据帧数量
    uint32_t 	nRecordMask;                // 当前通道是否开启 
                                            // bit-x: channel x record enable
    uint32_t 	nTotalTime;                 // 总运行时间，秒
    unsigned long long nFileSize;           // 文件大小，字节
    unsigned long long nFreeDiskSpace;      // 磁盘剩余大小
    unsigned long long nTotalDiskSapce;     // 磁盘空间大小
    
    //新增参数
    unsigned long long nStartTime;          // 开始记录时间 ; 20221115测试新增
    unsigned int nDelayStartTime;           // 开始记录延迟时间，界面需要显示则添加
    unsigned int nIntervalTime;             // 记录间隔时间，界面需要显示则添加
    
    int 		nReserved[STR_08]; 
} tagDeviceRecordStatus;
typedef struct RecodeStatusReply
{
    UserDataHeadInfo data_head;
    tagDeviceRecordStatus data_detail;
}RecodeStatusReply;

typedef struct tagCalibrationFileHeader
{
    unsigned int Version;                   // 版本号
    int nDeviceType;                        // 设备类型
    char Name[40];                          // file name
    char Description[40];                   // file discription
    char Guid[40];                          // 唯一标志
    char CreateTime[32];                    // 时间 yyyy-MM-dd HH:mm:ss
    char SerialNumber[32];                  // 设备序列号
    int nInputChannelNum;                   // default:	8
    int nInputRangeNum;                     // default:	5
    int nOutputChannelNum;                  // default:	4
    int nOutputRangeNum;                    // default:	1
    int nCalibrationFileSize ;
    unsigned int nCalibFlag;                // 校准标志 bit0-bit23 输入通道校准标, bit24-bit31输出通道校准标志
    int nReserved[6];                       // 8字节对齐
 }CalibrationFileHeader;
/////////////////////////// 文件格式 /////////////////////////////
// RECORD_FILE_HEADER                //576
// ChannelTableHeader chHeader        //24
// ChannelTableElem*chHeader.nTotalChannelNum    //284*ChannelCount
// DeviceDetailInfo    devInfo            //896
// DSAGlobalParams    globalParams                //164
// SignalDataSource * globalParams.nSignalCount    //344*count
// TriggerParamHeaderDSP triggerHeader
// ImpactTriggerParamDSP * triggerHeader.nNumber
 //} 总共有nReserved1个文件头部分数据

typedef struct DeviceDetailInfo
{
    char AccessCode[STR_32];
    uint16_t ID;          // 槽位号，0开始
    int16_t SubDeviceNum; // 子设备数量,主板中标注其子设备数量，子设备中标注为0
    char SerialNumber[STR_32];
    char DeviceName[STR_32];
    int32_t DeviceType;
    int32_t IsConnected; //
    char ConnectedPCAddress[STR_32];
    // version
    char DriverVersion[STR_32];
    char PlogicVersion[STR_32]; // FPGA版本号
    char DSPVersion[STR_32];
    char SoftwareVersion[STR_32];
    // 主从信息
    int32_t SystemID;
    char SystemName[STR_32];
    int32_t IsMaster; // 1 master
    int16_t MasterID;
    int16_t ClientCount;
    int16_t ClientsID[STR_128];
    // network info
    char Address[STR_32];
    char SubnetMask[STR_32];
    char Gateway[STR_32];
    char MACAddress[STR_32];
    char DeviceClock[STR_32];
    int32_t AIChannelNum;
    int32_t AOChannelNum;
    int32_t DIChannelNum;
    int32_t DOChannelNum;
    int32_t TachoChannelNum;
    int32_t GPSNum;
    float HighestSamplingFreq;
    int32_t ModeType; // 设备离/在线状态
    char LastUpdateDateTime[STR_32];
    char LastCalibrationDateTime[STR_32];

    unsigned short AuxNum; // 8个int Reserved= sReserved1[32]
    unsigned short VideoNum;
    unsigned short MobileNum; // 5G
    unsigned short WiFiNum;
    unsigned short CANIEnable;
    unsigned short CANlType; // 0 :Can,1:Can FD
    unsigned short CAN2Enable;
    unsigned short CAN2Type; // 0 :Can, 1:Can FD
    unsigned short SerialPortNum;
    unsigned short FlexRayNum;
    unsigned short TriggerNum;
    unsigned short Reserved1;
    float Reserved6; // 磁盘空间大小
    float Reserved7; // 磁盘剩余大小
    int Reseryed8;
    int Reserved9;

    char sReserved2[STR_32];
} DeviceDetailInfo;

typedef struct tagAoLocalColumn
{
    int nVersion;                       // 版本号
    int nDataSource;                    // 帧数据来源,0:模拟数据;1:外设数据
    int nFrameChCount;                  // 当前帧的通道数量
    int nFrameLen;                      // 每个帧每个通道的数据长度，float 数据长度
    unsigned long long nTotalFrameNum;  // ARM发的总帧数，计数值
    unsigned long long nCurNs;          // 当前纳秒时间
    double gp0;                     // 预留参数1
    double gp1;                     // 预留参数2，X轴保存间隔
    char gpStrFlag[16];     // 预留参数3, 16字节,TimeStream时用于给数据打标记，标记可以是文本和时间，长度16个字节
    float gp4;                     //
    float gp5;                     // 分频控制时低频降采样次数
    float gp6;                     // 分频控制信号为1
    float gp7;
    float gp8;
    float gp9;
    int gp10; // 记录 有效数据长度
    int gp11; // 用于记录发下去的当前通道Id，方便后续合并数据时区分不同设备的数据包（仅占用localColumnX）
} AoLocalColumn;

typedef struct tagRecordFrameHeader
{
	AoLocalColumn RecLocalColumn;
	unsigned int nValidNum; // 文件记录数据个数
	int nChID;
	int nReserved[4];
	// float pData[nValidNum]; // 记录通道数据，长度为nValidNum
} RECORD_FRAMEHEADER;

typedef struct tagSignalDataSource
{
    int nSignalID; // nSignalID
    int nSignalType;
    char sName[32];

    int bDisplay;
    int bRecord;
    int bSave;
    int nBlockSize; // data size
    int nCurrentSize;
    int nInputsId[8];
    int referChanId; //  reference id
    int respsChanId; // response id
    int displayFormat;
    AoLocalColumn localColumnX;
    AoLocalColumn localColumnY;

    int nSubSignalType; // 信号子类型
    int nStrategy;        //     控制策略0:滤波，1:Peak，2:Rms，3:均值
    int nReserved3;
    int nReserved4;
    char sReserved[32];
} SignalDataSource;

// 每个设备的Trigger
typedef struct tagTriggerParamHeaderDSP
{
    // 手动触发，自动触发，触发后继续
    int nMode;
    // 触发器数量
    int nNumber;
    int nOnOff;
    // 触发后的操作,最多支持4个操作
    int nOperation[4];
    int nTriggerVersion; // 0:锤击法用Trigger，其他值新用新结构
    int nReserved2;
    float fReserved1;
} TriggerParamHeaderDSP;

// 记录数据分包发送——数据头
typedef struct SendRecordPackHead_t
{
    uint32_t totalPackNum;    // 总包数（从 1 开始计数）
    uint32_t currentPackIdx;  // 当前包序号（从 1 到 totalPackNum）
    uint32_t currentPackAddr; // 当前包的地址 (字节)
    uint32_t packSize;        // 本包实际有效数据长度（字节）
    uint32_t crc32;           // 本包数据的 CRC32 校验值  
    uint32_t reserved;        // 
} SendRecordPackHead;

typedef struct SendRecordPack_t
{
    SendRecordPackHead head;
    uint8_t data[1]; // 柔性数组，实际大小为 head.packSize
} SendRecordPack;

/**********************************************
* 文件下载协议说明
* 
* DVS_FILE_DOWNLOAD_START: 接收文件名字符串 (char *filename)
*   - 返回值: UserDataHeadInfo.nParameters0
*     - 0: 成功，可开始数据下载
*     - 负数: 错误码
*
* DVS_FILE_DOWNLOAD_DATA: 使用 SendRecordPack_t 发送分包数据
*   - head.totalPackNum: 总包数
*   - head.currentPackIdx: 当前包序号
*   - head.packSize: 本包实际数据长度
*   - head.crc32: 本包CRC32校验值
*
* DVS_FILE_DOWNLOAD_STOP: 终止下载
*   - 返回值: UserDataHeadInfo.nParameters0
*     - 0: 成功完成
*     - 负数: 错误码
*
**********************************************/
 
#pragma pack(pop)
/**********************************************
* 数据包加密参数
**********************************************/
enum FreamEncryp
{	
    ENCRYP_DISABLE = 0,     // 不加密
    ENCRYP_RSS,             // RSS加密
};
/**********************************************
* 数据包校验参数
**********************************************/
enum FreamCheck
{	
    CHECK_DISABLE = 0,      // 不校验
    CHECK_CRC16_HL,         // crc16校验，高位在前
    CHECK_CRC16_LH,         // crc16校验，低位在前
    CHECK_SUM_8,            // 和校验
    CHECK_MD5,              // MD5校验
    CHECK_INIT = 0x5aa5,    // 固定16进制数据
};
/**********************************************
* 采集数据位宽定义
**********************************************/
enum FreamDataWide
{
    DATA_WORD = 0,          // 2字节
    DATA_DWORD = 1,         // 4字节
    DATA_LWORD = 2,         // 8字节
};
/**********************************************
* 采集数据对齐方式定义
**********************************************/
enum FreamAlign
{	
    DATA_RIGHT_ALIGN = 0,           // 右对齐
    DATA_LEFT_ALIGN  = 1,           // 左对齐
};

enum NetType
{
    NET_SERVER_TYPE_TCP = 0,
    NET_SERVER_TYPE_UDP = 1,
    NET_CFG_MAX = 4,
    NET_DISABLE = 0,
    NET_ENABLE = 1,
    NET_IPV4 = 4,
    NET_IPV6 = 6,
};
enum
{
    GROUP_VIBRATION_SIGNAL_IN = 0,      // 振动信号
    GROUP_SPEED_SIGNAL_IN = 1,          // 转速信号
    GROUP_STRESS_STRAIN = 2,            // 应力应变信号
    GROUP_ACOUSTIC,                     // 声学信号
    GROUP_OTHER,                        // 其它信号
};
enum
{
    RISING = 0,     // 上升沿
    FALLING = 1,    // 下降沿
};
enum{
    INPUT_RANGE_10000mV = 0,                // 10V
    INPUT_RANGE_3160mV = 1,                 // 3.16V
    INPUT_RANGE_1000mV = 2,                 // 1V
    INPUT_RANGE_316mV = 3,                  // 316MV
    INPUT_RANGE_100mV = 4,                  // 100MV
    INPUT_RANGE_10mV = 5,                   // 10MV
};
enum{
    SAMPLE_RATE_INDEX_512HZ     = 0X04,     // 512HZ
    SAMPLE_RATE_INDEX_1024HZ    = 0X05,     // 1024HZ
    SAMPLE_RATE_INDEX_2048HZ    = 0X06,     // 2048HZ
    SAMPLE_RATE_INDEX_4096HZ    = 0X07,     // 4096HZ
    SAMPLE_RATE_INDEX_8192HZ    = 0X08,     // 8192HZ
    SAMPLE_RATE_INDEX_16384HZ   = 0X09,     // 16384HZ
    SAMPLE_RATE_INDEX_32768HZ   = 0X0A,     // 32768HZ
    SAMPLE_RATE_INDEX_65536HZ   = 0X0B,     // 65536HZ
    SAMPLE_RATE_INDEX_131072HZ  = 0X0C,     // 131072HZ
    SAMPLE_RATE_INDEX_50HZ      = 0X10,     // 50HZ
    SAMPLE_RATE_INDEX_100H      = 0X11,     // 100HZ
    SAMPLE_RATE_INDEX_200H      = 0X12,     // 200HZ
    SAMPLE_RATE_INDEX_400H      = 0X13,     // 400HZ
    SAMPLE_RATE_INDEX_800HZ     = 0X14,     // 800HZ
    SAMPLE_RATE_INDEX_1600HZ    = 0X15,     // 1600HZ
    SAMPLE_RATE_INDEX_3200HZ    = 0X16,     // 3200HZ
    SAMPLE_RATE_INDEX_6400HZ    = 0X17,     // 6400HZ
    SAMPLE_RATE_INDEX_12800HZ   = 0X18,     // 12800HZ
    SAMPLE_RATE_INDEX_25600HZ   = 0X19,     // 25600HZ
    SAMPLE_RATE_INDEX_51200HZ   = 0X1A,     // 51200HZ
    SAMPLE_RATE_INDEX_102400HZ  = 0X1B,     // 102400HZ
    SAMPLE_RATE_INDEX_204800HZ  = 0X1C,     // 204800HZ
    SAMPLE_RATE_INDEX_256000HZ  = 0X1D,     // 256000HZ
};
enum{
    HIGH_PASS_FREQ_005hz    = 0,            // 0.05hz
    HIGH_PASS_FREQ_05hz     = 1,            // 0.5hz
    HIGH_PASS_FREQ_1hz      = 2,            // 1hz
    HIGH_PASS_FREQ_10hz     = 3,            // 10hz
    HIGH_PASS_FREQ_20hz     = 4,            // 20hz
};
// 低通截止频率
enum{
    LOW_PASS_FREQ_110kHz    = 0,            // 110kHz/direct
    LOW_PASS_FREQ_20Hz	    = 1,            // 20Hz
    LOW_PASS_FREQ_1kHz	    = 2,            // 1kHz
    LOW_PASS_FREQ_10kHz	    = 3,            // 10kHz
    LOW_PASS_FREQ_20kHz	    = 4,            // 20kHz
};
enum {
    SOURCE_TYPE_NO_DATA     = 0,            // 无数据
    SOURCE_TYPE_WITH_DATAS  = 1,            // 有数据
    SOURCE_TYPE_INFORM      = 2,            // 通知帧，无数据
};
enum {
    DESTINATION_DSP_TO_PC       = 0,        // 0:dsp to PC
    DESTINATION_DSP_TO_ARM      = 1,        // 1:dsp to arm
    DESTINATION_DSP_TO_ARM_PC   = 2,        // 2:dsp to arm and pc
    DESTINATION_ARM_TO_PC       = 3,        // 3:ARM to PC
    DESTINATION_ARM_TO_DSP      = 4,        // 4:ARM to DSP
    DESTINATION_ARM_TO_DSP_PC   = 5,        // 5:arm to dsp and pc
    DESTINATION_PC_TO_DSP       = 6,        // 6:PC to DSP
    DESTINATION_PC_TO_ARM       = 7,        // 7:PC to ARM
    DESTINATION_PC_TO_ARM_DSP   = 8,        // 8:pc to dsp and arm
};
enum {
    CHANNEL_TYPE_CONTRL_EXCITATION  = 0,    // 0:控制/激励，
    CHANNEL_TYPE_CONTRL_LIMIT   = 1,        // 1:控制+限制，
    CHANNEL_TYPE_RESPONSE   = 2,            // 2:响应（默认），
    CHANNEL_TYPE_RESPONSE_LIMIT	= 3,        // 3:响应+限制，
};
enum {
    NO_ERROR = 0,           // 0:正常
    WITH_ERROR = -1,        // -1:异常
};
enum {
    MODE_WORK = 0,          // 0:正常工作模式
    MODE_DEBUG = 1,         // 1:DEBUG模式
};
enum {
    RECORDE_STATUS_ERR = -1,
    RECORDE_STATUS_ING = 1,
    RECORDE_STATUS_OK = 2,
};
enum {
    DEV_CAN_WORK_MODE = 0,
    DEV_CAN_LISTENING_MODE = 1,
    DEV_CAN_SELFTEST_MODE = 2,
};
enum {
    LOCATION_GPS = 0,
    LOCATION_BD = 1,
};
//enum {
//    DISABLE	= 0,
//    ENABLE = 1,
//};
enum INPUTCH_QUANTITY_TYPE_ENUM
{
    INPUTCH_QUANTITY_VOLTAGE,
    INPUTCH_QUANTITY_ACCELERATION,
    INPUTCH_QUANTITY_VELOCITY,
    INPUTCH_QUANTITY_DISPLACEMENT,
    INPUTCH_QUANTITY_PRESSURE,
    INPUTCH_QUANTITY_FORCE,
    INPUTCH_QUANTITY_CURRENT,
    //SOUNDPRESSURE,//单位用PRESSURE
    INPUTCH_QUANTITY_TIME,
    INPUTCH_QUANTITY_FREQUENCY,
    INPUTCH_QUANTITY_MASS,
    
    INPUTCH_QUANTITY_TACHO,
    INPUTCH_QUANTITY_ANGLE,
    INPUTCH_QUANTITY_ANGULARACCELERATION,   //
    INPUTCH_QUANTITY_ANGULARVELOCITY,       //
    INPUTCH_QUANTITY_ANGULARDISPLACEMENT,   //
    INPUTCH_QUANTITY_MOMENT,                //TORQUE和MOMENT OF FORCE指的是一个意思: 力矩,MOMENT更广泛，MOMENT OF XX 指的就是某[距离]乘以某个量
    INPUTCH_QUANTITY_STRAINGAGE,
    INPUTCH_QUANTITY_STRESS,                //单位用PRESSURE
    INPUTCH_QUANTITY_TEMPERATURE,
    INPUTCH_QUANTITY_RESISTANCE,//
    INPUTCH_QUANTITY_HUMIDITY,
};
enum {
    CONNECT_STATUS_NEVER,       // 未被连接
    CONNECT_STATUS_LINE,        // 有线连接中
    CONNECT_STATUS_WIFI,        // wifi连接中
};

enum TempSensorType
{
    PT100   = 0,
    PT1000  = 1,
    TC      = 2,
};


/* 函数声明 */
uint8_t* pack_data(
    const uint8_t *user_data,
    uint32_t user_data_len,
    const UserDataHeadInfo *head_info,
    const FrameHeadInfo *frame_head,
    uint32_t *packet_len);
    
int unpack_data(
    const uint8_t *packet,
    uint32_t packet_len,
    uint8_t **out_user_data,
    UserDataHeadInfo *out_head_info,
    FrameHeadInfo *out_frame_head,
    uint32_t *out_user_data_len);
    
void free_unpacked_user_data(uint8_t *user_data);
FrameHeadInfo create_default_frame_head(uint16_t serial_num);
UserDataHeadInfo create_user_data_head(
    uint32_t event_id,
    uint32_t source_type,
    uint32_t dest_id,
    uint32_t data_len);

#endif // __LIB_USB_PROTOCOL_H