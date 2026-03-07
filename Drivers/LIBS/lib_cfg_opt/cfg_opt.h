/******************************************************************************
  Copyright (c) 2019-2021  (Xi'an) Testing & Control Technologies Co., Ltd.
  All rights reserved.

  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to 
    any third party.
  ------------------------------------------------------------------------

  *  Filename:      cfg_opt.h
  *
  *  Description:   
  * 
  *  Author:        
  *
  *  $Revision:     1.0 $
  *
*******************************************************************************/
#ifndef MCU_LIB_CFG_OPT_INCLUDE_CFG_OPT_H_
#define MCU_LIB_CFG_OPT_INCLUDE_CFG_OPT_H_


/******************************************************************************
* Include files.
******************************************************************************/
#include <stdio.h>
#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif
/******************************************************************************
* Macros.
******************************************************************************/
#define RET_OK      (0)
#define RET_ERROR   (-1)

#define MAX_LOG_PATH_LEN                (64)
#define MAX_LOG_LEVEL_LEN               (8)

#define MAX_VERSION_LEN                 (32)

#define PATH_AIO_CFG_FILE                        "/home/root/NTS/device_aio.conf"
// #define PATH_AIO_CFG_FILE                        "/home/root/NTS/device_ao.conf"

/* device_log.ini */
#define IDA_DEVICE_LOG_FILE                     "../ini/device_log.ini"
#define INI_SECTION_DAEMON                      "app_daemon"
#define INI_SECTION_MONITOR                     "app_monitor"
#define INI_SECTION_DEVICE                      "app_device"
#define INI_SECTION_PROCESSOR                   "app_processor"
#define INI_SECTION_DATA                        "app_data"
#define INI_KEY_PATH                            "path"
#define INI_KEY_LEVEL                           "level"

/* device_info.ini */
#define IDA_DEVICE_INFO_FILE                    "../ini/device_info.ini"
#define INI_SECTION_DEVICE_INFO                 "DeviceInfo"
#define INI_KEY_DEV_TYPE                        "device_type"
#define INI_KEY_DEV_ID                          "device_id"
#define INI_KEY_DEV_HARD_VER                    "device_hard_version"
#define INI_KEY_DEV_SOFT_VER                    "device_soft_version"
#define INI_KEY_DEV_DRIVER_VER                  "device_driver_version"
#define INI_KEY_DEV_FPGA_VER                    "device_fpga_version"
#define INI_KEY_DEV_NUM                         "device_num"

/* device_uart.ini */
#define IDA_DEVICE_UART_FILE                    "../ini/device_uart.ini"
#define INI_SECTION_UART1                       "UART_1"
#define INI_SECTION_UART2                       "UART_2"
#define INI_SECTION_UART3                       "UART_3"
#define INI_SECTION_UART4                       "UART_4"
#define INI_KEY_UART_RATE                       "rate"
#define INI_KEY_UART_BIT                        "databit"
#define INI_KEY_UART_STOP                       "datastop"
#define INI_KEY_UART_CHECK                      "datacheck"

/* device_net.ini */
#define IDA_DEVICE_NET_FILE                     "../ini/device_net.ini"
#define INI_SECTION_CLIENT_NETWORK              "NetClient"
#define INI_KEY_CLIENT_SERVER_IP                "server_ip"
#define INI_KEY_CLIENT_SERVER_PORT              "server_port"
#define INI_KEY_CLIENT_DHCP_STATUS              "dhcp_status"
#define INI_KEY_CLIENT_LOCAL_IP                 "local_ip"
#define INI_KEY_CLIENT_MASK                     "mask"
#define INI_KEY_CLIENT_GATEWAY                  "gateway"

#define INI_SECTION_SERVER_UDP                  "ServerUdp"
#define INI_KEY_SERVER_UDP_STATUS               "udp_status"
#define INI_KEY_SERVER_UDP_RX_PORT              "udp_rx_port"
#define INI_KEY_SERVER_UDP_TX_PORT              "udp_tx_port"

#define INI_SECTION_WIFI_INFO                   "WifiInfo"
#define INI_KEY_WIFI_STATUS                     "status"
#define INI_KEY_WIFI_USERNAME                   "username"
#define INI_KEY_WIFI_PASSWORD                   "password"

/* device_threshold.ini */
#define IDA_DEVICE_THRESHOLD_FILE               "../ini/device_threshold.ini"
#define INI_SECTION_LOG                         "LogLimit"
#define INI_SECTION_CORE                        "CoredumpLimit"
#define INI_KEY_MAX                             "max"
#define INI_KEY_MIN                             "min"

/* device_status.ini */
#define IDA_DEVICE_STATUS_FILE                  "../ini/device_status.ini"
#define INI_SECTION_PS                          "PS_monitor"
#define INI_KEY_TEMP                            "temperature"
#define INI_KEY_MEM_OCCUPY                      "mem_occupy"
#define INI_KEY_CPU_OCCUPY                      "cpu_occupy"

// 
#define SOFTWARE_VERSION_FILE                "../ini/version.ini"
#define DRIVER_VERSION_FILE                  "/etc/version"
/******************************************************************************
* Variables.
******************************************************************************/
typedef struct LogCfg{
    char log_path[MAX_LOG_PATH_LEN];
    char log_level[MAX_LOG_LEVEL_LEN];
    LogLevel log_level_int;
}LogCfg;

typedef struct UartCfg{
    int rate;
    int databit;
    int datastop;
    int datacheck;
}UartCfg;

// typedef struct NetCfg{
//     char    ip[16];
//     int     port;
// }NetCfg;

typedef struct 
{
    char server_ip[16];
    int dhcp_status;
    char local_ip[16];
    char mask[16];
    char gateway[16];
    int server_port;
}NetTcpCfg;
typedef struct
{
    int     udp_status;		    // Udp服务启用状态，0：disable;1：enable
    int     udp_rx_port;	    // udp通信数据发送端口(客户端)
    int     udp_tx_port;	    // udp通信数据接收端口(客户端)
}NetUdpCfg;
typedef struct 
{
    int		wifi_status;
	char	username[32];
	char 	password[32];
}NetWifiCfg;

typedef struct DevInfoCfg{
    char dev_type[MAX_LOG_PATH_LEN];
    char dev_id[MAX_LOG_PATH_LEN];
    char dev_hard_version[MAX_LOG_PATH_LEN];
    char dev_soft_version[MAX_LOG_PATH_LEN];
    char dev_driver_version[MAX_LOG_PATH_LEN];
    char dev_fpga_version[MAX_LOG_PATH_LEN];
    int  dev_num;    
}DevInfoCfg;
typedef struct ThreshlodCfg{
    int max;
    int min;
}ThreshlodCfg;

typedef struct DevStatusPram
{
    float ps_temp;
    float ps_mem_occupy;
    float ps_cpu_occupy;
}DevStatusPram;

 
/******************************************************************************
* Functions.
******************************************************************************/
int ReadLogCfg(const char *sector, LogCfg* temp);
int ReadUartCfg(const char *sector, UartCfg* temp);
int ReadNetTcpCfg(NetTcpCfg* temp);
int ReadNetUdpCfg(NetUdpCfg* temp);
int ReadDeviceInfoCfg(const char *sector, DevInfoCfg* temp);
int ReadThreshlodCfg(const char *sector, ThreshlodCfg* temp);
int ReadNetWifiCfg(NetWifiCfg* temp);

int WriteNetTcpCfg(NetTcpCfg* temp);
int WriteNetUdpCfg(NetUdpCfg* temp);
int WriteDeviceInfoCfg(const char *sector, DevInfoCfg* temp);
int WriteNetWifiCfg(NetWifiCfg* temp);

int get_software_version(char *str_data);
int get_driver_version(char *str_data);
void get_fpga_version(char *str_data);
int ReadDeviceStatusFile(DevStatusPram* temp);

int WriteIniStr(const char *path, const char *sector, const char *key, char* value_str);
int WriteIniInt(const char *path, const char *sector, const char *key, int* value_int);
int WriteIniFloat(const char *path, const char *sector, const char *key, float* value_float);
int WriteIniDouble(const char *path, const char *sector, const char *key, double* value_double);

// void LogInit(const char *sector);

#ifdef __cplusplus
}
#endif

#endif //MCU_LIB_CFG_OPT_INCLUDE_CFG_OPT_H_