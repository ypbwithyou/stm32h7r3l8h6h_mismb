


/****************************************************************************** 
  Copyright (c) 2022-2025  (Xi'an) Testing & Control Technologies Co., Ltd. 
  All rights reserved. 
 
  * This software can only be used for  production/manufacturing equipments. 
  * Without permission, it cannot be copied and used in any form or provided to  
    any third party. 
  ------------------------------------------------------------------------ 
 
  *  Filename:      ErrCode.h 
  * 
  *  Description:    
  *  
  *  Author:        
  * 
  *  $Revision:     1.0 $ 
  * 
*******************************************************************************/ 
#ifndef MCU_APP_PROCESSOR_INCLUDE_ERRCODE_H_ 
#define MCU_APP_PROCESSOR_INCLUDE_ERRCODE_H_ 
 
 
/****************************************************************************** 
* Include files. 
******************************************************************************/ 
#include <stdint.h> 
 
#ifdef __cplusplus 
extern "C" { 
#endif 
/****************************************************************************** 
* Macros. 
******************************************************************************/ 
#define DAQcfgError_UserDataLength             	(-24100)	// 用户数据长度异常
#define DAQcfgError_TotalChannelNum            	(-24101)	// 启用通道数配置异常
#define DAQcfgError_DataCheck                  	(-24102)	// 通道配置数据校验异常
#define DAQcfgError_PLStop                     	(-24103)	// PL侧执行STOP异常
#define DAQcfgError_PLCfg                      	(-24104)	// PL侧配置异常
#define DAQcfgError_PLAdcChannelEnable         	(-24105)	// PL侧通道使能异常
#define DAQcfgError_PLStart                    	(-24106)	// PL侧执行START异常
#define DAQcfgError_DevMode                    	(-24107)	// 设置了错误的设备工作模式
#define DAQcfgError_AnalogOutParaCheck         	(-24108)	// 信号源配置参数校验失败
#define DAQcfgError_PLAnalogOutParaCfg         	(-24109)	// PL侧信号源参数配置失败
#define DAQcfgError_PSGetDevStatus             	(-24110)	// PS获取设备运行状态失败
#define DAQcfgError_PLCfgTimeOff               	(-24111)	// PL侧配置超时
#define DAQcfgError_OfflineGetConfig           	(-24120)	// 获取离线模式配置参数失败
#define DAQcfgError_WifiData             	      (-24130)	// wifi配置参数异常
#define DAQcfgError_WifiConfig           	      (-24131)	// wifi配置失败
#define DAQcfgError_WifiGetIP           	      (-24132)	// wifi获取ip异常

#define DAQDevError_SpiTransfer                 (-24200)	// SPI发送异常
#define DAQDevError_ADCDevMod                   (-24201)	// ADC模式配置异常
#define DAQDevError_DMACfg	                    (-24202)	// DMA接收长度配置异常
#define DAQDevError_SYScalloc                   (-24203)	// 软件申请内存异常
#define DAQDevError_WriteEeprom                 (-24204)	// 软件申请内存异常
#define DAQDevError_ArmUpdate_Download          (-24205)	// 软件升级包下载失败
#define DAQDevError_ArmUpdate	                  (-24206)	// 软件升级失败
#define DAQDevError_MakeRecordFile              (-24210)	// 数据记录文件创建异常
#define DAQDevError_RecordData		              (-24211)	// 数据记录写文件异常
#define DAQDevError_CloseRecordData	            (-24212)	// 数据记录文件关闭异常
#define DAQDevError_SDcardNotExsit	            (-24213)	// 存储卡路径不存在
#define DAQDevError_SDcardSpaceNotEnough        (-24214)	// 磁盘空间不够
// #define DAQDevError_SDcardWriteOver             (-24215)	// 存储卡写操作已结束
#define DAQDevError_DevCan0PramCheck			      (-24220)	// can0设备配置参数校验异常
#define DAQDevError_DevCan1PramCheck			      (-24221)	// can1设备配置参数校验异常
#define DAQDevError_DevCan0PramCfg				      (-24222)	// can0设备配置参数异常
#define DAQDevError_DevCan1PramCfg				      (-24223)	// can1设备配置参数异常
#define DAQDevError_DevGps0PramCfg				      (-24230)	// gps0设备配置参数校验异常
#define DAQDevError_DevGps1PramCfg				      (-24231)	// gps1设备配置参数校验异常
#define DAQDevError_DevGps0InitErr				      (-24232)	// gps0设备不存在或打开设备异常
#define DAQDevError_DevGps1InitErr  			      (-24233)	// gps1设备不存在或打开设备异常
#define DAQDevError_DevUART0PramErr  			      (-24250)	// 通用串口UART0参数配置异常：设备类型与天线数量不符
#define DAQDevError_DevUART1PramErr  			      (-24251)	// 通用串口UART1参数配置异常

#define DAQDevError_DevEepromWriteErr  			    (-24240)	// eeprom读操作失败

#define DAQDspError_SourceType					        (-24300)	// 数据源类型异常
#define DAQDspError_DisplayData					        (-24310)	// 数据显示异常
#define DAQDspError_SpiBusy						          (-24320)	// SPI忙状态超时

/****************************************************************************** 
* Variables. 
******************************************************************************/ 
 
/****************************************************************************** 
* Functions. 
******************************************************************************/ 
 
#ifdef __cplusplus 
} 
#endif 
 
#endif //MCU_APP_PROCESSOR_INCLUDE_ERRCODE_H_ 
 