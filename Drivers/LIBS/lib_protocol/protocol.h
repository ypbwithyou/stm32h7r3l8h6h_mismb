/******************************************************************************
  Copyright (c) 2018-2021  (Xi'an) Testing & Control Technologies Co., Ltd.
  All rights reserved.
  
  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to any 
    third party.
  ------------------------------------------------------------------------
  文件名称：ida_protocol.h
  文件标识：协议处理
  摘    要：
  当前版本：1.0
  作    者：
  创建日期：2019年02月23日
  修改记录：
*******************************************************************************/
#ifndef LIB_PROTOCOL_INCLUDE_PROTOCOL_H_
#define LIB_PROTOCOL_INCLUDE_PROTOCOL_H_

/******************************************************************************
* Include files.
******************************************************************************/
#include <stdint.h>
#include "./LIBS/lib_link_list/link_list.h"

#ifdef __cplusplus
extern "C" {
#endif

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

#define FRAME_PL_HEAD		(0x55555555)
#define FRAME_PL_END		(0xAAAAAAAA)
#define FRAME_HEAD          (0x3C3C3C3C)			// 帧头"<<<<"
#define FRAME_END           (0x3E3E3E3E)			// 帧尾">>>>"
// #define PROTOCOL_CRC_TAG    (0X3D3D)            // crc校验TAG

// 包头数据长度（包括帧头及帧属性数据，不包含帧头和用户数据长度）
#define FREAM_HEAD_LENGTH        (sizeof(FrameHeadInfo))        // data length for frame_head
#define FREAM_LEN_LENGTH         (sizeof(uint32_t))         	// data length for fream_data_length
#define FREAM_CHECK_LENGTH       (sizeof(int16_t))         		// data length for fream_check
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

// 帧数据最大长度
// #define FRAME_MAX_SIZE          (1024)
// 用户数据最大长度
// #define DMA_BD_LEN					(10*1024*1000)	// DMA单个BD设置长度
// #define FRAME_USER_DATA_MAX_SIZE    (3*DMA_BD_LEN)	// DSP上传的用户数据最大长度
// #define DMA_LEN_MAX					(5*DMA_BD_LEN)	// DMA最大可读写长度
#define DMA_BD_LEN					(2*1024*1000)	// DMA单个BD设置长度
#define FRAME_USER_DATA_MAX_SIZE    (15*DMA_BD_LEN)	// DSP上传的用户数据最大长度
#define DMA_LEN_MAX					(25*DMA_BD_LEN)	// DMA最大可读写长度

// #define DMA_BD_LEN					(3*1024*1000)	// DMA单个BD设置长度
// #define FRAME_USER_DATA_MAX_SIZE    (10*DMA_BD_LEN)	// DSP上传的用户数据最大长度
// #define DMA_LEN_MAX					(16*DMA_BD_LEN)	// DMA最大可读写长度

#define CH_FREQ_MAX         (204800)
#define TIME_BAG            (50)
// #define CH_MAX              (ANALOG_IN_CH_MAX+(ROTATE_SPEED_CH_MAX>0?1:0))
// #define DATA_LEN            (sizeof(float))
// #define DATA_BAG_HEAD_LEN   (1)     // 原始数据包，包头(0xbeef+包序号)长度（所占数据个数）
// #define DATA_BAG_END_LEN    (1)     // 原始数据包，包尾（时间戳）长度（所占数据个数）
// #define DATA_BAG_LEN_MAX    (CH_FREQ_MAX * CH_MAX * TIME_BAG / 1000 + DATA_BAG_HEAD_LEN + DATA_BAG_END_LEN) // 原始数据包最大长度，单位int32_t
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
typedef struct 
{
    uint32_t    protocol_version;   // 协议版本
    uint32_t    frame_encrypt;      // 加密标志
    uint32_t    check_enable;       // 校验标志
    uint32_t    data_bit_wide;      // 数据位宽
    uint32_t    data_align_type;    // 数据对齐方式
    uint32_t    frame_num;          // 帧序号
}Frame_Attribute;

typedef struct 
{
    int     data_index;
    int     data_len;
    // char    data[0];
    void*   data;
}ProtocolStr;

typedef struct 
{
    uint32_t frame_tag;                     // 帧ID
    Frame_Attribute frame_attribute;        // 数据属性
    ProtocolStr* frame_user_encrypt;        // 用户数据密文：指针和长度
    ProtocolStr* frame_user_decode;         // 用户数据明文：指针和长度
    // void* frame_user_data;
    ProtocolStr* frame_user_data;
}FrameReply;

typedef struct 
{
    int32_t data_max;
    int32_t data_min;
}NormalDataRange;

typedef struct 
{
    uint32_t	hd_head;				// 帧头（0x3c3c3c3c）
	uint8_t		hd_attribute_ver;		// 帧头属性——版本(主+次)
	uint8_t		hd_attribute_flag;		// 帧头属性——标志（加密标志、校验标志）
	uint8_t		hd_data_status;			// 数据属性（位宽、对齐方式）
	uint8_t 	hd_reserved1;			// 预留数据1
	uint16_t 	hd_serial_num;			// 包序号
	uint8_t 	hd_reserved2;
	uint8_t 	hd_reserved3;
	uint8_t 	hd_reserved4;
	uint8_t 	hd_reserved5;
	uint8_t 	hd_reserved6;
	uint8_t 	hd_reserved7;
	int32_t		hd_reserved8;
}FrameHeadInfo;     // 帧头

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
	char	username[STR_32];
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
    EVTSEG_INIT	            			= 0x00001000,
    EVTSEG_RUN	            			= 0x00002000,           // communication during running
    EVTSEG_CSP	            			= 0x00003000,           // 参数更新和控制csp control loop
    EVTSEG_DISP	            			= 0x00004000,           // display control loop event
	EVTSEG_PREPROC						= 0x00005000,			// 预处理控制事件
	EVTSEG_FILE							= 0x00006000,			// 文件记录与下载

    DVS_INIT_DETECT	     				= EVTSEG_INIT + (0X01),     // PC->ARM
    DVS_INIT_DETECT_OK	 				= EVTSEG_INIT + (0X02),     // ARM->PC
    DVS_INIT_CONNECT     				= EVTSEG_INIT + (0X03),     // ARM->PC
    DVS_INIT_CONNECT_OK  				= EVTSEG_INIT + (0X04),     // PC->ARM 
 	DVS_INIT_DISCONNECT					= EVTSEG_INIT + (0X05),		// PC->ARM->DSP 断开连接
 	DVS_INIT_DISCONNECT_OK				= EVTSEG_INIT + (0X06),		// DSP->ARM->PC，断开连接
    DVS_INIT_DEVCONFIG_UPDATE  	  		= EVTSEG_INIT + (0X07),     // PC->ARM 
    DVS_INIT_DEVCONFIG_UPDATE_OK  		= EVTSEG_INIT + (0X08),     // ARM->PC 
	DVS_INIT_DEVCONFIG_UPDATE_Done		= EVTSEG_INIT + (0X09),     // PC->ARM 
    DVS_INIT_DEVCONFIG_UPDATE_Done_OK	= EVTSEG_INIT + (0X0A),     // ARM->PC 
	DVS_INIT_CONNECT_DETECT				= EVTSEG_INIT + (0X0B),		// PC->ARM 心跳
 	DVS_INIT_CONNECT_DETECT_OK			= EVTSEG_INIT + (0X0C),		// ARM->PC 心跳
 	DVS_INIT_DEBUG_MODE					= EVTSEG_INIT + (0X0D),		// PC->ARM->DSP（测试用）
 	DVS_INIT_DEBUG_MODE_OK				= EVTSEG_INIT + (0X0E),		// DSP->ARM->PC
 	DVS_INIT_SET_CLOCK					= EVTSEG_INIT + (0X0F),		// PC->ARM->DSP, 时间信息放在MSGStruct的DataFlag
 	DVS_INIT_SET_CLOCK_OK				= EVTSEG_INIT + (0X10),		// DSP->ARM->PC
 	DVS_INIT_GET_CLOCK					= EVTSEG_INIT + (0X11),		// PC->ARM->DSP
 	DVS_INIT_GET_CLOCK_OK				= EVTSEG_INIT + (0X12),		// DSP->ARM->PC, 时间信息放在MSGStruct的DataFlag
	DVS_INIT_GET_DEVCONFIG 				= EVTSEG_INIT + (0X13),		// PC->ARM->DSP
	DVS_INIT_GET_DEVCONFIG_OK			= EVTSEG_INIT + (0X14),		// DSP->ARM->PC
	// DVS_INIT_DSP_UPDATE_FIRMWARE_DONE	= EVTSEG_INIT + (0X17),		// PC->DSP  DSP版本升级
	// DVS_INIT_DSP_UPDATE_FIRMWARE_DONE_OK	= EVTSEG_INIT + (0X18),		// DSP->PC
	// DVS_INIT_DSP_GET_FIRMWARE			= EVTSEG_INIT + (0X19),		// PC->DSP
	// DVS_INIT_DSP_GET_FIRMWARE_DONE		= EVTSEG_INIT + (0X1A),		// DSP->PC
	// DVS_INIT_DSP_GET_FIRMWARE_DONE_OK	= EVTSEG_INIT + (0X1B),		// PC->DSP
	// DVS_INIT_DSP_GET_FIRMWARE_OK			= EVTSEG_INIT + (0X1C),		// DSP->PC
	// DVS_INIT_DSP_RESET 					= EVTSEG_INIT + (0X1D),		// PC->DSP
	// DVS_INIT_DSP_RESET_OK				= EVTSEG_INIT + (0X1E),		// DSP->PC
	DVS_INIT_GET_VERSION				= EVTSEG_INIT + (0X1F),		// ARM->DSP
	// DVS_INIT_GET_VERSION_DONE			= EVTSEG_INIT + (0X20),		// DSP->PC
	// DVS_INIT_GET_VERSION_DONE_OK			= EVTSEG_INIT + (0X21),		// PC->DSP
	DVS_INIT_GET_VERSION_OK				= EVTSEG_INIT + (0X22),		// DSP->ARM
	DVS_INIT_SET_VERSION 				= EVTSEG_INIT + (0X23),		// PC->ARM->DSP 设置版本信息
	DVS_INIT_SET_VERSION_OK				= EVTSEG_INIT + (0X24),		// DSP->ARM->PC
	
	DVS_INIT_DSP_TIMESYNC_SYNC	        = EVTSEG_INIT + (0X2B),     // ARM->DSP 时间同步
	DVS_INIT_DSP_TIMESYNC_SYNC_OK		= EVTSEG_INIT + (0X2E),     // DSP->ARM 时间同步返回

	DVS_INIT_ARM_TIMESYNC_SYNC 			= EVTSEG_INIT + (0X2f),  	// Master记录发送时刻T1,发送带T1消息给Slave，Slave收到消息并记录本地接收时刻T2。
	DVS_INIT_ARM_TIMESYNC_DELAYREQ 		= EVTSEG_INIT + (0X30), 	// Slave发送给Master，Slave记录发送时刻T3,
	DVS_INIT_ARM_TIMESYNC_DELAYRESP 	= EVTSEG_INIT + (0X31), 	// master接收到DELAYREQ包，记录接收时刻T4，然后将t4通过DELAYRESP发送给slave,slave接收到包含T4的DELAYRESP包，将T4保存计算并应用Offset=(t2+t3−t1−t4)/2
	DVS_INIT_AMR_TIMESYNC_SYNC_OK 		= EVTSEG_INIT + (0X32),		// 通知Master同步完成
	DVS_INIT_ARM_UPGRADE_REQ  			= EVTSEG_INIT + (0X33),		// PC->ARM  升级请求
	DVS_INIT_ARM_UPGRADE_REQ_OK 		= EVTSEG_INIT + (0X34),		// ARM->PC	升级请求应答，携带用户名、密码、存储路径等
	DVS_INIT_ARM_UPGRADE_DONE  			= EVTSEG_INIT + (0X35),  	// PC->ARM  准备启动升级
	DVS_INIT_ARM_UPGRADE_DONE_OK 		= EVTSEG_INIT + (0X36), 	// ARM->PC	返回升级包校验结果
	DVS_INIT_ARM_RESET  				= EVTSEG_INIT + (0X37), 	// 暂不使用 20220606
	DVS_INIT_ARM_RESET_OK 				= EVTSEG_INIT + (0X38), 
	DVS_INIT_DEVICES_TIMESYNC 			= EVTSEG_INIT + (0X39),  	// 同步信号，PC-->ARM-->FPGA
	DVS_INIT_DEVICES_TIMESYNC_OK 		= EVTSEG_INIT + (0X3A),  	//

	DVS_INIT_ARM_GETWORKEMOD      		= EVTSEG_INIT + (0x3B),		 
	DVS_INIT_ARM_GETWORKMODE_OK        	= EVTSEG_INIT + (0x3C),		
	DVS_INIT_ARM_WORKMODECHANGED     	= EVTSEG_INIT + (0x3D),		// 设备离/在线状态上报
	DVS_INIT_ARM_WORKMODECHANGED_OK    	= EVTSEG_INIT + (0x3E),		
	DVS_INIT_GET_DEVSTATUS     			= EVTSEG_INIT + (0x3F),		// 获取设备运行时各功能和接口的状态
	DVS_INIT_GET_DEVSTATUS_OK    		= EVTSEG_INIT + (0x40),	
	DVS_INIT_CONNECT_REQUEST			= EVTSEG_INIT + (0X41),		// PC->ARM 心跳请求	
	DVS_INIT_CONNECT_REQUEST_OK			= EVTSEG_INIT + (0X42),		// ARM->PC 心跳请求	

	DVS_INIT_WIFI_CONNECT				= EVTSEG_INIT + (0X45),		// PC->ARM wifi配置	
	DVS_INIT_WIFI_CONNECT_OK			= EVTSEG_INIT + (0X46),		// ARM->PC 

	DVS_READ_TEDS_INFO	    			= EVTSEG_INIT + (0X47),      // PC->Hardware
    DVS_READ_TEDS_INFO_OK				= EVTSEG_INIT + (0X48),      // Hardware->PC

	DVS_INIT_DSP2DSP                    = EVTSEG_INIT + (0X4B),      // Hardware->PC

    DVS_RUN	                			= EVTSEG_RUN + (0X01),		// PC->Hardware
    DVS_RUN_OK	            			= EVTSEG_RUN + (0X02),      // Hardware->PC
    DVS_PAUSE	            			= EVTSEG_RUN + (0X03),      // PC->Hardware
    DVS_PAUSE_OK	        			= EVTSEG_RUN + (0X04),      // Hardware->PC
    DVS_CONTINUE	        			= EVTSEG_RUN + (0X05),      // PC->Hardware
    DVS_CONTINUE_OK	        			= EVTSEG_RUN + (0X06),      // Hardware->PC
    DVS_STOP	            			= EVTSEG_RUN + (0X07),      // PC->Hardware 
    DVS_STOP_OK	            			= EVTSEG_RUN + (0X08),      // Hardware->PC
    DVS_RUN_ERROR	    				= EVTSEG_RUN + (0X09),      // PC->Hardware
    DVS_RUN_ERROR_OK					= EVTSEG_RUN + (0X0A),      // Hardware->PC
	
	DVS_DSP_RUN   						= EVTSEG_RUN + (0x0D),		// mode offline: arm->dsp
	DVS_DSP_RUN_OK   					= EVTSEG_RUN + (0x0E),		// mode offline: dsp->arm

	DVS_DSP_OUTPUT_ENABLE				= EVTSEG_RUN + (0x21),		// mode offline: dsp->arm 离线模式下模拟输出通道使能
	DVS_DSP_OUTPUT_ENABLE_OK			= EVTSEG_RUN + (0x22),		// mode offline: arm->dsp
	DVS_ARM_DIGITOUTPUT 				= EVTSEG_RUN + (0x23),	  	 // ARM操作数字输出通道
	DVS_ARM_DIGITOUTPUT_OK 				= EVTSEG_RUN + (0x24),	
	DVS_ARM_DIGITINPUT 					= EVTSEG_RUN + (0x25),	  	 // ARM收到数字输入通道
	DVS_ARM_DIGITINPUT_OK 				= EVTSEG_RUN + (0x26),	
	DVS_ARM_STATUSLIGHT_SET 			= EVTSEG_RUN + (0x31),  	 // 设置ARM设备的运行状态灯，目前是在自启动时使用，
																		// 1=未连接或断连，
																		// 2=已连接但未运行，
																		// 3=仅运行，未记录，
																		// 4=记录中，
																		// 5=运行过程异常，未断连
	DVS_ARM_STATUSLIGHT_SET_OK 			= EVTSEG_RUN + (0x32),

	DVS_EMG_STOP                        = EVTSEG_RUN + (0x38),       // 紧急停止
	DVS_EMG_STOP_OK                     = EVTSEG_RUN + (0x39),

    DVS_CSP_START	        			= EVTSEG_CSP + (0X01),       // PC->Hardware 采集预配置指令
    DVS_CSP_START_OK	    			= EVTSEG_CSP + (0X02),       // Hardware->PC 
	DVS_CSP_START_Done	    			= EVTSEG_CSP + (0X03),       // PC->Hardware 采集配置指令
    DVS_CSP_START_Done_OK				= EVTSEG_CSP + (0X04),       // Hardware->PC 
	DVS_CSP_ARM_CONFIG  				= EVTSEG_CSP + (0X1F),		 // dev config,to arm
	DVS_CSP_ARM_CONFIG_OK  				= EVTSEG_CSP + (0X20),

	DVS_CSP_OFFLINE_GETCONFIG    		= EVTSEG_CSP + (0X25),		 // mode offline: 获取离线模式的配置参数 PC->ARM
	DVS_CSP_OFFLINE_GETCONFIG_OK  		= EVTSEG_CSP + (0X26),		 
	DVS_CSP_OFFLINE_SETCONFIG    		= EVTSEG_CSP + (0X27),		 // mode offline: 设置离线模式的配置参数 PC->ARM
	DVS_CSP_OFFLINE_SETCONFIG_OK  		= EVTSEG_CSP + (0X28),		 
	DVS_CSP_ARM_CONFIG_INERTIAL 		= EVTSEG_CSP + (0X2b),		 // 惯导参数to arm
	DVS_CSP_ARM_CONFIG_INERTIAL_OK 		= EVTSEG_CSP + (0X2c),		

	DVS_ARM_UPDATE						= EVTSEG_CSP + (0X81),       // PC->Hardware ARM升级指令
	DVS_ARM_UPDATE_OK					= EVTSEG_CSP + (0X82),       // Hardware->PC 升级结果应答

    DVS_DISPNEXT	        			= EVTSEG_DISP + (0X01),      // PC->Hardware 信号显示指令
    DVS_DISPNEXT_DONE       			= EVTSEG_DISP + (0X02),      // DSP->PS
    DVS_DISPNEXT_DONE_OK    			= EVTSEG_DISP + (0X03),      // PS->DSP
    DVS_DISPNEXT_OK	        			= EVTSEG_DISP + (0X04),      // Hardware->PC 
    DVS_DISPNEXT_PARA       			= EVTSEG_DISP + (0X05),      // PC->Hardware 待显示信号预配置指令
    DVS_DISPNEXT_PARA_OK    			= EVTSEG_DISP + (0X06),      // Hardware->PC 
    DVS_DISPNEXT_PARA_DONE  			= EVTSEG_DISP + (0X07),      // PC->Hardware 待显示信号配置指令
    DVS_DISPNEXT_PARA_DONE_OK   		= EVTSEG_DISP + (0X08),      // Hardware->PC 
    DVS_ARM_DISPNEXT   					= EVTSEG_DISP + (0X09),      // can设备、gps设备、通用串口外接设备数据读取
    DVS_ARM_DISPNEXT_OK   				= EVTSEG_DISP + (0X0a),      // 

	DVS_PREPROC_RUN_OUTPUT				= EVTSEG_PREPROC + (0X01),	 // PC->ARM->DSP 信号源输出启动
	DVS_PREPROC_RUN_OUTPUT_OK			= EVTSEG_PREPROC + (0X02),	 // DSP->ARM->PC
	DVS_PREPROC_STOP_OUTPUT				= EVTSEG_PREPROC + (0X03),	 // PC->ARM->DSP
	DVS_PREPROC_STOP_OUTPUT_OK			= EVTSEG_PREPROC + (0X04),	 // DSP->ARM->PC
	DVS_PREPROC_OUTPUT_UPDATE			= EVTSEG_PREPROC + (0X05),	 // PC->ARM->DSP
	DVS_PREPROC_OUTPUT_UPDATE_OK		= EVTSEG_PREPROC + (0X06),	 // DSP->ARM->PC
	DVS_PREPROC_OUTPUT_UPDATE_DONE		= EVTSEG_PREPROC + (0X07),	 // PC->ARM->DSP
	DVS_PREPROC_OUTPUT_UPDATE_DONE_OK	= EVTSEG_PREPROC + (0X08),	 // DSP->ARM->PC

	DVS_FILE_RECORD_BEGIN_DONE			= EVTSEG_FILE + (0X03),		 // PC->PS->DSP 开始记录
	DVS_FILE_RECORD_BEGIN_DONE_OK		= EVTSEG_FILE + (0X04),		 // DSP->PS->PC 
	DVS_FILE_RECORD_DSP_DATA			= EVTSEG_FILE + (0X07),		 // PC->PS->DSP 发送记录数据
	DVS_FILE_RECORD_DSP_DATA_OK			= EVTSEG_FILE + (0X08),		 // DSP->PS->PC 应答记录结果
	DVS_FILE_RECORD_STATUS				= EVTSEG_FILE + (0X09),		 // PC->PS->DSP 
	DVS_FILE_RECORD_STATUS_OK			= EVTSEG_FILE + (0X0a),		 // DSP->PS->PC 上报记录状态
	DVS_FILE_RECORD_PAUSE				= EVTSEG_FILE + (0X0b),		 // PC->PS->DSP 
	DVS_FILE_RECORD_PAUSE_OK			= EVTSEG_FILE + (0X0c),		 // DSP->PS->PC 
	DVS_FILE_RECORD_CONTINUE			= EVTSEG_FILE + (0X0d),		 // PC->PS->DSP  
	DVS_FILE_RECORD_CONTINUE_OK			= EVTSEG_FILE + (0X0e),		 // DSP->PS->PC 
	DVS_FILE_RECORD_END					= EVTSEG_FILE + (0X0f),		 // PC->PS->DSP 停止记录
	DVS_FILE_RECORD_END_OK				= EVTSEG_FILE + (0X10),		 // DSP->PS->PC 

	DVS_FILE_CALIBRATION_READ		    = EVTSEG_FILE + (0x11),
	DVS_FILE_CALIBRATION_READ_OK	    = EVTSEG_FILE + (0x12),
	DVS_FILE_CALIBRATION_WRITE		    = EVTSEG_FILE + (0x13),
	DVS_FILE_CALIBRATION_WRITE_OK	    = EVTSEG_FILE + (0x14),
};

//FrameData_head structure : FrameData_head
typedef struct UserDataHeadInfo
{
	uint32_t    nIsValidFlag;					//Checkifvalid
	uint32_t    nEventID;
	uint32_t    nSourceType;        			/* 消息的类型，消息，带数据的消息
												0：无数据
												1：有数据
												2：通知帧无数据，通知下一帧带数据，携带数据长度在nDataLength中 */
	uint32_t    nDestinationID;	    			// 消息的传递源和目标
	int32_t     nParameters0;					// nSourceType=0，即无数据时，存放错误码
	int32_t     nParameters1;
	int32_t     nParameters2;
	int32_t     nParameters3;
	int32_t     nDataLength;        			// 实际用户数据长度
    uint32_t    DataFlag;           			// 数据包标识
	long long 	nNanoSecond;
}UserDataHeadInfo;

//FrameData_data structure : FrameData_data_DevDiscover
typedef struct DeviceInfo
{ 
	char    Address[STR_32];
	char    Name[STR_32];
	char    AccessCode[STR_32]; 
	char    SerialNumber[STR_32];
	int32_t DeviceType; 
	int32_t IsConnected;
	char    ConnectedPCAddress[STR_32]; 
	unsigned short SubDeviceNum;	// 子节点数据长度
	unsigned short AvailablePcNum;	// 可用PC数量,用于许可控制
	// int32_t Reserved1;
	int32_t Reserved2;
}DeviceInfo;

// UDP send msg
typedef struct UdpMsgDevDiscover
{
	UserDataHeadInfo data_head;
	DeviceInfo	data_detail;
}UdpMsgDevDiscover;

typedef struct UdpMsgRecv
{
	UserDataHeadInfo data_head;
	char AccessCode[STR_32];
}UdpMsgRecv;

//FrameData_data structure : FrameData_data_DeviceDetailInfo
typedef struct DeviceDetailInfo
{  
	char    AccessCode[STR_32]; 
	uint16_t ID;
	int16_t Reserved3;
	char    SerialNumber[STR_32];
	char    DeviceName[STR_32]; 
	int32_t DeviceType; 
	int32_t IsConnected;// 
	char    ConnectedPCAddress[STR_32]; 
	//version
	char    DriverVersion[STR_32];
	char    PlogicVersion[STR_32];
	char    DSPVersion[STR_32]; 
	char    SoftwareVersion[STR_32]; 
	//主从信息
	int32_t SystemID;
	char    SystemName[STR_32];
	int32_t IsMaster;
	int16_t MasterID; 
	int16_t ClientCount; 
	int16_t ClientsID[STR_128]; 
	//network info
	char    Address[STR_32]; 
	char    SubnetMask[STR_32]; 
	char    Gateway[STR_32]; 
	char    MACAddress[STR_32]; 
	char    DeviceClock[STR_32]; 
	int32_t AIChannelNum;
	int32_t AOChannelNum;
	int32_t DIChannelNum;
	int32_t DOChannelNum;
	int32_t TachoChannelNum;
 	int32_t GPSNum;
	float   HighestSamplingFreq;
	int32_t ModeType;			// 设备离/在线状态
	char    LastUpdateDateTime[STR_32]; 
	char    LastCalibrationDateTime[STR_32]; 
	
	unsigned short AuxNum;		//8个int Reserved= sReserved1[32]
	unsigned short VideoNum;
	unsigned short MobileNum;	//5G
	unsigned short WiFiNum;
	unsigned short CANIEnable;
	unsigned short CANlType;	//0 :Can,1:Can FD
	unsigned short CAN2Enable;
	unsigned short CAN2Type;	//0 :Can, 1:Can FD
	unsigned short SerialPortNum;
	unsigned short FlexRayNum;
	unsigned short TriggerNum;
	unsigned short Reserved1;
	float Reserved6;		// 磁盘空间大小
	float Reserved7;		// 磁盘剩余大小
	int Reseryed8;
	int Reserved9;
	
	char    sReserved2[STR_32]; 
}DeviceDetailInfo;

typedef struct UserData_connect
{
    UserDataHeadInfo data_head;
    DeviceDetailInfo data_detail;
}UserData_connect;

//FrameData_data structure : FrameData_data_ChannelTableHeader
typedef struct ChannelTableHeader
{
	int32_t nTotalChannelNum; 			// 总启用通道个数，用于解析每个通道参数
	int32_t nHardwareSampleRateIndex;	// 默认采样率10240 
	int32_t nFlagIntDiff;				// 默认是否做微分和积分标识Differentiation & Intergration Flag
	int32_t nICPCurrentIndex;			// 默认ICP电压，默认4.5mA，4.5mA or 9mA 
	int32_t nReserved1;	    			// Deciamtion stage in use
    int32_t nReserved2;  
}ChannelTableHeader;

//FrameData_data structure : FrameData_data_ChannelTableElem
typedef struct ChannelTableElem
{
	int32_t nChannelID;					// Channel ID from,start 0
	int32_t nInputRange;				/* Input range index 输入通道量程
											0: 10V、
											1: 3.16V、
											2: 1V、
											3: 316mV、
											4: 100mV、
											5: 10mv */
	int32_t nChannelStatus;				/* low 7 bit indicates the all kinds of channel status
		    			    				bit 0: 1:单端输入   0: 差分输入	       信号输入模式
		    			    				bit 1: 1:DC耦合     0: AC耦合		  耦合方式
		    			    				bit 2: 1:校准开启  	0: 校准关闭		   校准状态
		    			    				bit 3: 1:TEDs开启  	0: TEDs关闭		  TEDS状态
		    			    				bit 4: 1:ICP开启  	0: ICP关闭 		  ICP状态
											bit 5: 1:9.5mA ICP	0: 4.5mA ICP	 ICP电流
											bit 6: 1:100kΩ		0: 1mΩ			 输入阻抗
											bit 7: 1:通道使能	 0: 通道不使能 	   通道状态 
											bit 8: 1:外部校准类型	0: 内部校准类型 	bit2校准开启时有效
											bit 9: 1:电荷输入开启	 0: 电荷输入关闭		注意：电荷的开启与ICP开启互斥
											bit 10:  1: 外部电荷输入	0: 内部电荷输入*/
	int32_t	nGroupID;					/* 通道分组类型：
											0: 振动(ICP),
											1: 转速(电压),
											2: 应变, */
	float	fSampleRateIndex;			// 通道采样率
	float	fHighPassFreq;				// 高通截止频率
	float	fCalibGain;					// 校准增益
	float	fCalibOffset;				// 校准偏移
	char	Unit[28];					// 测量单位，由物理量纲决定
	int		UnitId;
	char	Quantity[28]; 				// 测量的物理量纲
	int 	QuantityId;
	float	fSensitivity;   			// 灵敏度，来源于传感器属性
	float	fSensorRange;   			// 传感器测量范围，来源于传感器属性
	float	fFlagIntDiff;				/* Differentiation & Intergration Flag
											0 -> Null
											1 -> Differentiation
											2 -> Double Differetiation
											3 -> Integrationlow
											4 -> Double Integrationlow */
	int32_t nChannelType;				/* 通道类型，包括：0=控制/激励，1=控制+限制，2=响应，3=响应+限制
											0: Control only
											1: Control+Limit
											2: Monitor only
											3: Monitor+Limit
											4: COLA */
	float   fReserved1;					// TachoEdgeValue = 1;//每转脉冲值
										// 电荷输入开启时，电荷输入的增益，单位：mV/pC

	float   fReserved2;					/* 低通截止频率
										    00: direct
										    01: 20kHz 
										    10: 10kHz 
											11: 1kHz*/
	float   fReserved3;					// 
	float   fReserved4;					// 
	// float   fReserved2;					// TachoPulsePerRevolution = 1;//脉冲边缘值，范围（-10.0--+10.0）
	// float   fReserved3;					// TachoDetectionRevolution = 1;//脉冲检测分辨率
	// float   fReserved4;					// TachoGearRate = 1; //齿轮比

	int nBoolVarStored;					// nChannelStatus的扩展位
	int nSensorType;					// 传感器类型， 物理量纲是温度时，1: PT1000, 0: PT100，其它：热电偶

	float 	fWeighting;

	// int32_t	nReserved3;
    int32_t  nG1oba1Physica1I;          // 在整个系统(所有设备)里面的唯一ID，从0开始

	int32_t	nReserved4;
	float	fRealSampleRate;			// 真实的采样率，FPGA不支持则DSP降采，即界面显示的采样率
	float	fReserved5;

	/* nGroupID不同，则nVar定义不同*/
	int32_t	nVar1; 						// 应力应变：bit15--bit0 --> 桥路类型索引nBridgeType（1-8），bit31--bit16 --> 分流校准类型（0--10）;
										// 当nGroupID=1，且nVar1=1时，表示使用专用通道测转速 20240125
	int32_t	nVar2; 						// 应力应变：线制索引nQBMode;0=无线制,1=2线制,2=3线制,3=4线制,4=6线制
	int32_t	nVar3; 						// 应力应变：平衡电阻fGaugeR;0,120,350,1000
										// 转速：触发方式，0：Rising, 1: Falling
	int32_t	nVar4; 						// 应力应变：激励方式和系数nExTypeValue，第17位是激励方式：0(电压激励)、1(电流激励)；低16位是激励系数：0--65535之间的一个整数值
	int32_t	nVar5; 						// 转速：Tacho：PowerMode，0：5V, 1：10V
	int32_t	nVar6;						// IVS16/IVS8D：恒流源时，1：push-push模式，0：push-pull模式，-1：未设置
	int32_t	nVar7;
	int32_t	nVar8;
	// int32_t	nVar9;
	// int32_t	nVar10;		
	// int32_t	nVar11;		
	// int32_t	nVar12;		

	float	fVar1; 						// 应力应变：灵敏度系数fGaugeFator;其他用途
	float	fVar2; 						// 应力应变：最小应变值fMinStrain
	float	fVar3; 						// 应力应变：最大应变值fMaxStrain
	float	fVar4; 						// 应力应变：泊松比fPoissonRatio
	float	fVar5; 						// 应力应变：弹性模量fYoungsFactor
	float	fVar6; 						// 应力应变：导线电阻fLeadR
	float	fVar7; 						// 应力应变：分流器的电阻fShuntR
	float	fVar8;						// 应力应变：温度通道，分固定值和通道索引，通道索引为-50000到-51000	
	float	fVar9;						// 应力应变：参考温度			
	float	fVar10;						// 应力应变：灵敏度系数温度变化率K，
	float	fVar11;						// 应力应变：热膨胀系数
	float	fVar12;						// 应力应变：材料的热膨胀系数
	float	fVar13;						// 应力应变：多项式系数1
	float	fVar14;						// 应力应变：多项式系数2
	float	fVar15;						// 应力应变：多项式系数3
	float	fVar16;						// 应力应变：多项式系数4
	float	fVar17;						// 应力应变：多项式系数5
	float	fVar18;						// 应力应变：
	float	fVar19;						// 应力应变：
	float	fVar20;						// 应力应变：

	char	Label[STR_16];
} ChannelTableElem;

typedef struct DSAGlobalParam
{
	int32_t	nBlockSize;
	int32_t	nWindowType;
	int32_t	nAverageType;				//0:linear	//1:exponential //2:peak_hold
	int32_t	nAverageNum;			
	int32_t	nOverlapRatio;				//	0:	no overlap
	int32_t	nAcqMode;					//	0:	Free Run
										//	1:	trigger
										//	2:	 time
	int32_t	nTriggerSourceID;			//	
	int32_t	nTriggerType;				//	0:	High edge
										//	1:	Low Edge
										//	2:	High Level
										//	3:	Low level
	int32_t	nTriggerOffset;				//	block size
	float	fTriggerHighLevel;			//trigger high level
	float	fWinParam0;
	float	fWinParam1;
	float	fWinParam2;
	float	fTriggerLowLevel;			//trigger low level
	int32_t	nOctaveType;				//0:	1/1
										//1:	1/3
										//2:	1/6
										//3:	1/12
										//4:	1/24
										//5:	1/48
										//default:1
	int32_t	nLogAnalysis;				//0:Linear
										//1:Log
										//Default:1
	float	fLowFreq;       			// 5, Edit(Hz) 
	                        			// default: 5
										// Minimum is 0.1 Hz, Maximum is High Frequency
	float	fHighFreq;      			// 1000, Edit(Hz)
	                        			// default: 1000
	                        			// Minimum is Low Frequency, Maximum is 20 kHz
	float	fReferenceFreq;				//Between High and low freq

	float	fDampingRatio;				// 
	
	float	fEdgeValue;					//default 1.0
	float	fPulsePerRev;				//default 1.0 
	float	fSLMRefreshInt; 			//for SLM, block signal refresh interval;
	int32_t	nRMSPeakSampleRate; 		// 
	int32_t	nFFTAvgOff;					// 0:FFT average on, 1:FFT average off	
	int32_t	nAcqDeciUsedFlag;			//for DSA, Acquisition module decimation stage used flag. 0:not used(default); 1:used
	float	fLowPassCutoffFreq; 		//Range 0Hz-Fa,default 0: off
	int32_t	nSignalCount; 
	int32_t	nReserved0;
	int32_t	nReserved1;
	int32_t	nReserved2;
	int32_t	nReserved3;
	int32_t	nReserved4;
	int32_t	nReserved5;
	int32_t	nReserved6;
	int32_t	nReserved7;
	int32_t	nReserved8;
	float	fReserved1;
	float	fReserved2;
	float	fReserved3;
	float	fReserved4;
} DSAGlobalParam;
// 轴向属性
typedef struct AoLocalColumn {

	double minimum;
	double maximum;
	int32_t measurement_quantity;
	int32_t raw_datatype;				//数据类型，0：int，1：FLOAT，2：DOUBLE，3：COMPLEX
	double gp0;							//预留参数1，X轴保存起始
	double gp1;							//预留参数2，X轴保存间隔
	double gp2;
	double gp3;
	double gp4;
	double gp5;
	double gp6;
	double gp7;
} AoLocalColumn;
// Live Signal List 信号列表
typedef struct 
{
	int32_t	nSignalID;		
	int32_t	nSignalType;				// NVH type：APS，FFT，TimeBlock等等
	char    sName[STR_32];				// 信号名称
	int32_t	bDisplay;					// 是否显示
	int32_t	bRecord;					// 是否记录
	int32_t	bSave;						// 是否保存
	int32_t	nBlockSize;					// 数据大小
	int32_t	nCurrentSize;				// 当前大小
	int32_t nInputsId[STR_08];	// 所有输入信号的ID
	int32_t	referChanId;				// 参考通道ID
	int32_t	respsChanId;				// 响应通道ID
	int32_t	displayFormat;				// 显示格式
	AoLocalColumn localColumnX;			// X轴结构体
	AoLocalColumn localColumnY;			// Y轴结构体
	int32_t	nReserved1;		
	int32_t	nReserved2;		
	int32_t	nReserved3;		
	int32_t	nReserved4;	
	char    sReserved[STR_32];	
} SignalDataSource;
// 回传的每帧信息头
typedef struct DisplayParam {
	int32_t nDisplayFlag;				//帧头信息，常数（0x7654321）
	int32_t nAcceptedFrameNum; 			//
	int32_t nTotalFrameNum;				//计算的总帧数

	int32_t nAverageNum; 				//
	float   fOutputPeak;

	int32_t nDIOStatus;
	int32_t nChannelStatusSize;

	int32_t nSysClockStatus;
	int32_t DisplaySignalCount; 		//DSPFrame的个数
	long 	nFreeSectorNum;				//磁盘剩余

	int32_t nReserved1;
	int32_t nReserved2;
	int32_t nReserved3;
	int32_t nReserved4;
	int32_t nReserved5;
	int32_t nReserved6;
	int32_t nReserved7;
	int32_t nReserved8;
	float 	fReserved1;
	float 	fReserved2;
} DisplayParam;
// 回传的信号头信息
// FrameData_data structure : FrameData_BackDataHead
typedef struct FrameDataSource {
	int32_t nSignalID;
	int32_t nSignalType;

	int32_t nBlockSize;
	int32_t nCurrentSize;				// buffer 点数

	int32_t nReserved1;
	int32_t nReserved2;
	int32_t nReserved3;
	int32_t nReserved4;
	AoLocalColumn localColumnX;
	AoLocalColumn localColumnY;
} FrameDataSource;
// 配置的待显示信号
typedef struct DisplaySignalID {
	int32_t nSignalID;					// 信号ID
} DisplaySignalID;

// 升级指令格式
typedef struct DoneloadFileInfo
{
    char 		name[STR_64];            	// 待更新软件名称
    char 		version[STR_32];      		// 待更新软件版本
    uint32_t 	size;                		// 待更新软件大小
    char 		path[STR_128];				// 下载地址
} DoneloadFileInfo;
typedef struct IdaInfo
{
    char UserName[STR_32];         			// 用户名
    char UserPassword[STR_32];     			// 密码
	char UserPath[STR_128];					// 存储路径
} IdaInfo;
typedef struct UpdateInfo
{
    uint32_t	update_way;         		// 升级方式
    DoneloadFileInfo	update_info;     	// 待升级文件信息
    IdaInfo  	user_info;          		// 身份信息
} UpdateInfo;

typedef struct FileInfo
{
    char 		FileName[STR_64];   		// 待更新软件名称
    uint32_t 	FileSize;           		// 待更新软件大小
} FileInfo;

typedef struct SendDevMsg
{
	UserDataHeadInfo head;
	IdaInfo	dev_msg;
} SendDevMsg;

// 信号源输出配置
typedef struct tagOutputParamsDsp
{
	int 	nIndex;
	char 	sName[STR_32];
	char 	deviceInfo[STR_32];
	int 	nUsage;					// 0=off /1=on,
	int 	nSynchronized;
	int 	outputType;				// DSAOutputType	
	float 	fSamplerate;
	float 	fOutputRange;			// 输出的电压范围
	float 	fFrequency;    			// for sine, triangle, square 
	float 	fAmplitude;    			// for all,if white noise or shaped random ,it represent rms
	float 	fFreqHigh;     			// high freq for sweep sine and chirp
	float 	fFreqLow;      			// low freq for sweep sine and chirp
	float 	fPeriod;       			// time for one period of sweep sine and chirp (second)
	float 	fPercent;  				// for burst random/burst shaped random/burst chirp
	float 	fPhase;	
	int 	nRampTime;				// Ramp Time，for Sine/Triangle/Square/Random/Pink Noise/Brown Noise/DC, Defines the time for the level to reach its specified value as a linear ramp
	float 	fRMS;
	int 	nDuration;
	int  	nSweeptype; 			// Sweep type (0:Linear,1:Log) for Sine Sweep/chirp/burst chirp
	float 	fOffset;
	float 	fGain;
	uint32_t 	nProfileSize;		// index, 0
	uint32_t 	nArbWaveSize;		// index, 1
	uint32_t 	nPlaybackSize;		// index, 2	
	int 		nDurationTime;		// 计划表，输出持续时间，到时间后结束
	int 		nSweepDir;			// 扫频正弦和步进正弦表示 SweepDirection 扫频方向，0=向前(默认)，1=向后
	uint32_t 	nComOutputSize; 	// 用户自定义输出大小
	float 		fRealSampleRate; 	// 真实采样率，及界面显示的采样率
	int 		nPLClear;			// 20230509:控制输出时是否清除当前运行中的输出数据，0--不清除，1--清除
	int 		nReserved[19];
} tagOutputParamsDsp;

/*tagOutputParamsDsp中nComOutputSize为0，表示没有该结构，672表示有这个结构*/
typedef struct CombinedInput
{
	int		nEnable;					// default = 0
	int 	nExpressionType; 		// 表达是类型 0:前缀，1:中缀，2:后缀；default = 1
	char 	expressionData[512];
	int 	nChCount;
	int 	chIndex[32];
	int 	nReserved[5];
} CombinedInput;

// 文件记录：文件头信息
typedef struct tagRecordHeader
{	
	int 		nVersion;			// 版本号	
	int			nReserved0;
	char 		Name[STR_32];		// 文件名	
	long long 	nCreateTime;      	// 文件创建时间	
	uint32_t 	nDeviceChNum;  		// 设备通道数量	
	uint32_t 	nRecordNum;   		// 记录通道数量	
	uint32_t 	nFrameNum;    		// 记录数据帧数量
	uint32_t 	nRecordMask;  		// 记录通道标识：
	      							// bit-x: channel x record enable
	int 		nReserved[STR_128];
} tagRecordHeader;
#define RECORD_FRAME_SIZE (512U)
// 文件记录：原始数据
typedef struct tagRecordFrame
{
	AoLocalColumn LocalColumnX;
	AoLocalColumn LocalColumnY;
	uint32_t 	nValidNum;
	int 		nChID;
	int 		nReserved[STR_04];
	float 		pData[RECORD_FRAME_SIZE];
} tagRecordFrame;
// 文件记录：文件记录状态
typedef struct tagDeviceRecordStatus
{
	uint32_t 	nVersion;					// 版本号
	int 		nStatus;  					// 1，状态中，2，记录完成，<0 异常状态，异常类型待定
	char 		SerialNumber[STR_32];		// 设备序列号
	char 		Name[STR_32];				// 记录文件名称
	uint32_t 	nDeviceChNum;				// 设备通道数量
	uint32_t 	nFrameNum;					// 当前记录数据帧数量
	uint32_t 	nRecordMask;				// 当前通道是否开启 
			 								// bit-x: channel x record enable
	uint32_t 	nTotalTime;					// 总运行时间，秒
	unsigned long long nFileSize;			// 文件大小，字节
	unsigned long long nFreeDiskSpace;		// 磁盘剩余大小
	unsigned long long nTotalDiskSapce;		// 磁盘空间大小

	//新增参数
	unsigned long long nStartTime; 			// 开始记录时间 ; 20221115测试新增
	unsigned int nDelayStartTime;  			// 开始记录延迟时间，界面需要显示则添加
	unsigned int nIntervalTime;  			// 记录间隔时间，界面需要显示则添加

	int 		nReserved[STR_08]; 
} tagDeviceRecordStatus;
typedef struct RecodeStatusReply
{
    UserDataHeadInfo data_head;
    tagDeviceRecordStatus data_detail;
}RecodeStatusReply;
// GPS配置
typedef struct tagHsDevGPSConfig
{ 
	uint32_t	nVersion;				// 版本号
	int32_t		nGPSDevId;				// 1：device2，0：device1
	int32_t		nGPSType;				// 1：北斗，0：GPS
	int32_t 	isEnableGPS;			// 1：使能，0：禁用
	// 以下配置for pc
	int32_t 	nSaveType;				// for pc
	int32_t 	isNeedLatitude;			// to device,dir=纬度N（北纬）
	int32_t 	isNeedLongitude; 		// dir=经度E（东经）
	int32_t 	isNeedSpeed;
	int32_t 	isNeedAltitude;			// 海拔高度
	int32_t 	isNeedHeading;			// 航向
	int32_t 	isNeedUTCTime;			// UTC时间
	int32_t 	isNeedStatus;			// 状态标志位，A：有效，V无效
	int32_t 	isNeedTotalSatellite;	// 当前可见卫星总数，从00到12
	int32_t 	isNeedUsedSatellite;	// 使用卫星数量，从00到12
	int32_t 	nReserved[STR_08]; 
}tagHsDevGPSConfig;
// 运行中GPS的传输结构
typedef struct tagHsDevGPSTransfer
{ 
	uint32_t	nVersion;			// 版本号
	int32_t		nGPSDevId;			// 1：device2，0：device1
	int32_t 	nGPSType;			// 1：北斗，0：GPS
	int32_t		dirLatitude;        // 南北指标(0=N,1=S)
	int32_t 	dirLongitude;       // 东西指标(0=E,1=W)
	float 		dLatitude;			// 纬度
	float 		dLongitude; 		// 经度
	float 		dSpeed;
	float 		dAltitude;			// 海拔高度
	float 		dHeading;			// 航向
	int32_t 	nStatus;			// 状态标志位，A：有效，V无效
	int32_t 	nTotalSatellite;	// 当前可见卫星总数，从00到12
	int32_t 	nSatellite;			// 使用卫星数量，从00到12
	uint32_t	nDate;				// 日期
	uint32_t	nUTCTime;			// UTC时间
	int32_t 	nReserved[STR_08];
} tagHsDevGPSTransfer;

// CANBUS配置
typedef struct tagHsDevCanbusConfig
{
	uint32_t	nVersion;			// 版本号
	uint32_t	isEnableCAN;		// 1:enable 0:disable
	int32_t 	nCanDevId;			// 设备接口ID，设备有2个CAN接口，默认用第一个
	int32_t 	nCanModeType;		// can 协议的类型，0=can ,1= obd2,...
	int32_t 	nBusNum;			// 设备接口中的通道号,2个通道，0或1
	int32_t 	isCanFd;
	float		fDataBaudRate;		// 数据域波特率，单位kbps
	uint32_t	nResistanceSwitch;	// 120欧姆电阻接入状态，0——断开，1——接入
	char 		sCanFdConfig[STR_128-8];
	float 		fBaudRate;			// 固定500kpds,单位Kbps
	int32_t 	nWorkMode;			// 0=正常工作、1=仅侦听模式,2=自测模式（环回模式）
	int32_t 	nPidCount;			// 需要获取的PID个数，PID存在结构后面的PID Buffer中,buffer原始数据类型int
	char		SerialNumber[32];	// 预留
	int32_t		nInterval;			// 预留
	int32_t		nSubPidCount;		// 需要下发的帧结构（tagHsDevSubPidConfig）的数量
	int32_t 	nReserved[6];
} tagHsDevCanbusConfig;
typedef struct tagHsDevSubPidConfig
{
	int32_t 	nSendID;			// 需要发送的帧的ID
	int32_t 	nReceivePid;		// 预留
	char 		nSubInfo[STR_08];	// 需要发送的帧的数据部分;	
	int32_t 	nReserved[4];		// 预留
}tagHsDevSubPidConfig;

// 运行中CAN的传输结构
typedef struct tagHsDevCanbusTransfer
{
	uint32_t	nVersion;			// 版本号
	int32_t		nCanDevId;			// 设备接口ID，设备有2个CAN接口，0=第一个，1=第二个
	int32_t		nPidCount;			// 需要获取的PID个数，PID存在结构后面的PID Buffer中，buffer原始数据类型float
	int32_t	 	nCanModeType; 		// can 协议的类型，0=can,1=obd2, 2=DBC... 
	int32_t	 	nFrameNumber;
	int32_t		nReserved[6]; 
}tagHsDevCanbusTransfer;

typedef struct tagHsDevSerialPortConfig
{
	uint32_t 	nVersion;			// 版本号
	int32_t 	isEnable;			// 0-Disable;1-Enabled, 所有串口的总开关，设备中每个串口值一样
	int32_t 	driveType;			// 0-惯性导航；1-方向盘。
	int32_t 	state;				// 0-Disable;1-Enabled;当前串口启用状态
	uint32_t	parameter;			// 惯性导航设备天线数量，目前仅支持单天线、双天线；
	int32_t 	nReserved[STR_08-1];
} tagHsDevSerialPortConfig;

typedef struct CanDbcData		// 当nCanModeType = DBC数据时，回传数据buffer结构
{
	uint32_t id;
	char can_data[STR_64];
} CanDbcData;

typedef struct CanDbcDatas		// 当nCanModeType = DBC数据时，回传数据buffer结构
{
	uint32_t id;
	unsigned char can_dlc;		// 有效数据字节数，暂时不用
	unsigned char can_na[3];	// 预留
	char can_data[STR_64];
	unsigned long long nTime;
} CanDbcDatas;

// arm设备数据传输结构头
typedef struct tagHsDevArmTransferHeader
{
	uint32_t	nVersion;			// 版本号
	int32_t		nGpsEnable;			// 高16位：GPS2，低16位：GPS1；1：使能，0：不使能
	int32_t		nCanbusEnable;		// 高16位：CAN2，低16位：CAN1；1：使能，0：不使能
	int32_t 	nSerialPortEnable;	// 按设备类型 (DeviceType) 决定串口占用位 (bit). (DeviceType== 50:0-惯性导航,1-方向盘.)

	uint32_t nGpsFrameLen[2]; 		// 携带的2路gps数据的长度：n* HsDevGPSTransfer
	uint32_t nCanFrameLen[2]; 		// 携带的2路can数据的长度：tagHsDevCanbusTransfer+n* tagInertialNavigationData
	uint32_t nUartFrameLen[2]; 		// 携带的2路串口数据的长度：n* tagHsDevSteeringWheelData

	int32_t		nReserved[9]; 
}tagHsDevArmTransferHeader;
// 设备状态检测项目数据结构
typedef struct tagHsDevStatusMonitor
{
    uint32_t nVersion;				// 版本号 
    float fArmTemperature;			// Arm 温度
    float fArmCpuUsage;             // ARM CPU使用率
    float fArmMemoryUsage;          // Arm 内存使用率
    float fDspTemperature;
    float fDspUsage;
    float fDspMemoryUsage;
    float fFpgaTemperature;
    float fFpgaUsage;
    float fFpgaMemoryUsage;
    float fNetBandwidth;             // 网络带宽
    float fNetUsage;                 // 网络使用率 
    float fBatteryCapacity;          // 电池容量
    float fBatteryUsage;             // 电池使用量
    int32_t nGpsStatus;              // bit0: gps 1  enable; bit1: gps 2  enable
    int32_t nCanStatus;              // bit0: can 1  enable; bit1: can 2  enable
    int32_t nAuxStatus;
    int32_t nVideoStatus;
    int32_t nMobileStatus;
    int32_t nWiFiStatus;
    uint32_t nAIStatus;           // 当前输入通道是否开启                  
                                      // bit0: analog channel 1  enable
                                      // bit1: analog channel 2  enable
                                      // ...
    uint32_t nAOStatus;           // 当前输出通道是否开启
                                      // bit0: analog channel 1  enable
                                      // bit1: analog channel 2  enable
                                      // ...
    uint32_t nTachoStatus;        // 当前通道是否开启 
                                      // bit0: Tacho channel 1  enable
                                      // bit1: Tacho channel 2  enable
                                      // ...
    unsigned long long nDiskSpaceUsage;        //磁盘空间已使用大小
    unsigned long long nTotalDiskSapce;        //磁盘空间大小

    int32_t nParam0;
    int32_t nParam1;
    int32_t nParam2;
    int32_t nParam3;
    int32_t nParam4;
    int32_t nParam5;
    int32_t nParam6;
    float fParam0;
    float fParam1;
    float fParam2;
    float fParam3;
    float fParam4;
    float fParam5;
    float fParam6;
    float fParam7;
} tagHsDevStatusMonitor;

// 惯导相关数据
// 导航数据
typedef struct tagHsDevNavigationData
{
	float fPitchAngle;		//俯仰
	float fRollAngle;		//横滚
	float fHeading;			//航向
	float fEastSpeed;		//东向速度
	float fNorthSpeed;		//北向速度
	float fCelestialSpeed;	//天向速度
	float fLongitude;		//经度
	float fLatitude;		//纬度
	float fAltitude;		//高度
	float fPressAltitude;	//气压高度

	int nMode;				//模式
	int nFrameFmt;			//帧格式
	unsigned long long nTime;	//帧时间
	
	int nReserved[2];
} tagHsDevNavigationData;
// 传感器数据
typedef struct tagHsDevSensorData
{
	float fGyroscopeX;		//陀螺X
	float fGyroscopeY;		//陀螺Y
	float fGyroscopeZ;		//陀螺Z
	float fAccelerationX;	//加速度X
	float fAccelerationY;	//加速度Y
	float fAccelerationZ;	//加速度Z
	float fGeomagnetismX;	//地磁X
	float fGeomagnetismY;	//地磁Y
	float fGeomagnetismZ;	//地磁Z
	float fTemperatureX;	//温度X
	float fTemperatureY;	//温度Y
	float fTemperatureZ;	//温度Z
	
	int nReserved[4];
} tagHsDevSensorData;
// GPS数据
typedef struct tagHsDevGPSData
{
	float fHeading;			//航向
	float fLongitude;		//经度
	float fLatitude;		//纬度
	float fEastSpeed;		//东向速度
	float fNorthSpeed;		//北向速度
	float fCelestialSpeed;	//天向速度
	float fAltitude;		//高度
	
	int nMode;				//模式
	int nTotalSatellite;	//卫星数
	float nPDOP;			//PDOP
	
	float fGroundSpeed;		// 地速
	int nReserved[3];
} tagHsDevGPSData;
// 方向盘转向角转向力检测仪数据
typedef struct tagHsDevSteeringWheelData
{
	float fSteeringAngleRT;		//转向角实时值，范围：±1080°
	float fSteeringAngleMax;	//转向角最大值

	float fSteeringForceRT;		//转向力实时值
	float fSteeringForceMax;	//转向力最大值，范围：±450N

	double fTime;				//消息时间，用于显示帧数据区分，该字段可不赋值由上面获取消息时间赋值

	int nReserved[6];
} tagHsDevSteeringWheelData;
typedef struct tagHsDevUartGpsData
{
	tagHsDevNavigationData data_Navigation;
	tagHsDevSensorData data_Sensor;
	tagHsDevGPSData data_UartGPS;
	// tagHsDevSteeringWheelData data_UartSteering;
} tagHsDevUartGpsData;

typedef struct tagCalibrationFileHeader
{
    unsigned int Version;  // 版本号
    int nDeviceType;       // 设备类型
    char Name[40];          // file name
    char Description[40];   // file discription
    char Guid[40];          // 唯一标志
    char CreateTime[32];    // 时间 yyyy-MM-dd HH:mm:ss
    char SerialNumber[32];  // 设备序列号
    int nInputChannelNum;  // default:	8
    int nInputRangeNum;    // default:	5
    int nOutputChannelNum; // default:	4
    int nOutputRangeNum;   // default:	1
    int nCalibrationFileSize ;
    unsigned int nCalibFlag; // 校准标志 bit0-bit23 输入通道校准标, bit24-bit31输出通道校准标志
    int nReserved[6];         // 8字节对齐
 }CalibrationFileHeader;
/**********************************************
* 数据包加密参数
**********************************************/
enum FreamEncryp
{	
    ENCRYP_DISABLE = 0,         // 不加密
    ENCRYP_RSS,                 // RSS加密
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
    DATA_WORD = 0,		// 2字节
    DATA_DWORD = 1,		// 4字节
    DATA_LWORD = 2,		// 8字节
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
	GROUP_VIBRATION_SIGNAL_IN = 0,		// 振动信号
	GROUP_SPEED_SIGNAL_IN = 1,			// 转速信号
	GROUP_STRESS_STRAIN = 2,			// 应力应变信号
	GROUP_ACOUSTIC,						// 声学信号
	GROUP_OTHER,						// 其它信号
};
enum
{
	RISING = 0,		// 上升沿
	FALLING = 1,	// 下降沿
};
enum{
	INPUT_RANGE_10000mV = 0,		// 10V
	INPUT_RANGE_3160mV = 1,			// 3.16V
	INPUT_RANGE_1000mV = 2,			// 1V
	INPUT_RANGE_316mV = 3,			// 316MV
	INPUT_RANGE_100mV = 4,			// 100MV
	INPUT_RANGE_10mV = 5,			// 10MV
};
enum{
	SAMPLE_RATE_INDEX_512HZ 	= 0X04,		// 512HZ	
	SAMPLE_RATE_INDEX_1024HZ 	= 0X05,		// 1024HZ	
	SAMPLE_RATE_INDEX_2048HZ 	= 0X06,		// 2048HZ	
	SAMPLE_RATE_INDEX_4096HZ 	= 0X07,		// 4096HZ	
	SAMPLE_RATE_INDEX_8192HZ 	= 0X08,		// 8192HZ	
	SAMPLE_RATE_INDEX_16384HZ 	= 0X09,		// 16384HZ
	SAMPLE_RATE_INDEX_32768HZ 	= 0X0A,		// 32768HZ
	SAMPLE_RATE_INDEX_65536HZ 	= 0X0B,		// 65536HZ
	SAMPLE_RATE_INDEX_131072HZ 	= 0X0C,		// 131072HZ
	SAMPLE_RATE_INDEX_50HZ 		= 0X10,		// 50HZ	
	SAMPLE_RATE_INDEX_100H 		= 0X11,		// 100HZ	
	SAMPLE_RATE_INDEX_200H 		= 0X12,		// 200HZ	
	SAMPLE_RATE_INDEX_400H 		= 0X13,		// 400HZ	
	SAMPLE_RATE_INDEX_800HZ 	= 0X14,		// 800HZ	
	SAMPLE_RATE_INDEX_1600HZ 	= 0X15,		// 1600HZ	
	SAMPLE_RATE_INDEX_3200HZ 	= 0X16,		// 3200HZ	
	SAMPLE_RATE_INDEX_6400HZ 	= 0X17,		// 6400HZ	
	SAMPLE_RATE_INDEX_12800HZ 	= 0X18,		// 12800HZ
	SAMPLE_RATE_INDEX_25600HZ 	= 0X19,		// 25600HZ
	SAMPLE_RATE_INDEX_51200HZ 	= 0X1A,		// 51200HZ
	SAMPLE_RATE_INDEX_102400HZ 	= 0X1B,		// 102400HZ
	SAMPLE_RATE_INDEX_204800HZ 	= 0X1C,		// 204800HZ
	SAMPLE_RATE_INDEX_256000HZ  = 0X1D,		// 256000HZ
};
enum{
	HIGH_PASS_FREQ_005hz	= 0, 			// 0.05hz	
	HIGH_PASS_FREQ_05hz		= 1, 			// 0.5hz	
	HIGH_PASS_FREQ_1hz		= 2, 			// 1hz	
	HIGH_PASS_FREQ_10hz		= 3, 			// 10hz	
	HIGH_PASS_FREQ_20hz		= 4, 			// 20hz	
};
// 低通截止频率
enum{
	LOW_PASS_FREQ_110kHz    = 0, 	// 110kHz/direct
	LOW_PASS_FREQ_20Hz	    = 1, 	// 20Hz
	LOW_PASS_FREQ_1kHz	    = 2, 	// 1kHz
	LOW_PASS_FREQ_10kHz	    = 3, 	// 10kHz	
	LOW_PASS_FREQ_20kHz	    = 4, 	// 20kHz	
};
enum {
	SOURCE_TYPE_NO_DATA		= 0,		// 无数据
	SOURCE_TYPE_WITH_DATAS  = 1,		// 有数据
	SOURCE_TYPE_INFORM		= 2,		// 通知帧，无数据
};
enum {
	DESTINATION_DSP_TO_PC		= 0,		// 0:dsp to PC
	DESTINATION_DSP_TO_ARM		= 1,		// 1:dsp to arm
	DESTINATION_DSP_TO_ARM_PC	= 2,		// 2:dsp to arm and pc
	DESTINATION_ARM_TO_PC		= 3,		// 3:ARM to PC
	DESTINATION_ARM_TO_DSP		= 4,		// 4:ARM to DSP
	DESTINATION_ARM_TO_DSP_PC	= 5,		// 5:arm to dsp and pc
	DESTINATION_PC_TO_DSP		= 6,		// 6:PC to DSP
	DESTINATION_PC_TO_ARM		= 7,		// 7:PC to ARM
	DESTINATION_PC_TO_ARM_DSP	= 8,		// 8:pc to dsp and arm
};
enum {
	CHANNEL_TYPE_CONTRL_EXCITATION	= 0,	// 0:控制/激励，
	CHANNEL_TYPE_CONTRL_LIMIT	= 1,		// 1:控制+限制，
	CHANNEL_TYPE_RESPONSE	= 2,			// 2:响应（默认），
	CHANNEL_TYPE_RESPONSE_LIMIT	= 3,		// 3:响应+限制，
};
enum {
	NO_ERROR = 0,			// 0:正常
	WITH_ERROR = -1,		// -1:异常
};
enum {
	MODE_WORK = 0,			// 0:正常工作模式
	MODE_DEBUG = 1,			// 1:DEBUG模式
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
enum {
	DEV_DISABLE	= 0,
	DEV_ENABLE = 1,
};
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
	INPUTCH_QUANTITY_ANGULARACCELERATION,//
	INPUTCH_QUANTITY_ANGULARVELOCITY,//
	INPUTCH_QUANTITY_ANGULARDISPLACEMENT,//
	INPUTCH_QUANTITY_MOMENT,//TORQUE和MOMENT OF FORCE指的是一个意思: 力矩,MOMENT更广泛，MOMENT OF XX 指的就是某[距离]乘以某个量
	INPUTCH_QUANTITY_STRAINGAGE,
	INPUTCH_QUANTITY_STRESS,//单位用PRESSURE
	INPUTCH_QUANTITY_TEMPERATURE,
	INPUTCH_QUANTITY_RESISTANCE,//
	INPUTCH_QUANTITY_HUMIDITY,
};
enum {
	CONNECT_STATUS_NEVER,		// 未被连接
	CONNECT_STATUS_LINE,		// 有线连接中
	CONNECT_STATUS_WIFI,		// wifi连接中
};

enum  DIOSignalTypei
{
	H,		//high
	L,		//low 	
	LHL, 	//low high low
	HLH,	//high low high
	LH,		//low high
	HL,		//high low
};

enum CanModeType
{
	CANMODE_CAN = 0,
	CANMODE_OBD2,
	CANMODE_DBC,
};

enum TempSensorType
{
	PT100	= 0,
	PT1000	= 1,
	TC		= 2,
};
/******************************************************************************
* Functions.
******************************************************************************/
void SetFrameAttribute(Frame_Attribute *set_attribute);
ProtocolStr* GetProtocolStrtoSend(ProtocolStr* data_in, int destination);
ProtocolStr* GetFrameDataFromList(LinkList* data_receive);
FrameReply* GetProtocolReplyFrameData(ProtocolStr* frame_data);
#if NtoH_or_HtoN
void SwitchHtoN_UserDataHead(UserDataHeadInfo* data_in);
void SwitchHtoN_DeviceInfo(DeviceInfo* data_in);
void SwitchNtoH_DeviceDetailInfo(DeviceDetailInfo* data_in);
#endif
#ifdef __cplusplus
}
#endif

#endif  // LIB_PROTOCOL_INCLUDE_PROTOCOL_H_