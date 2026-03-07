/******************************************************************************
  Copyright (c) 2019-2021  (Xi'an) Testing & Control Technologies Co., Ltd.
  All rights reserved.

  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to 
    any third party.
  ------------------------------------------------------------------------

  *  Filename:      processor_cfg.c
  *
  *  Description:   
  * 
  *  Author:
  *
  *  $Revision:     1.0 $
  *
*******************************************************************************/


/******************************************************************************
* Include files.
******************************************************************************/
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "ini.h"
#include "cfg_opt.h"
#include "board.h"
/******************************************************************************
* Macros.
******************************************************************************/
// #define IDA_DEVICE_LOG_FILE                     "../ini/device_log.ini"
// #define INI_KEY_PATH                            "path"
// #define INI_KEY_LEVEL                           "level"

/******************************************************************************
* Variables.
******************************************************************************/
struct log_level_name {
    char log_name[MAX_LOG_LEVEL_LEN];
    LogLevel log_level;
};

static struct log_level_name g_log_level_name[] = {
    {"DEBUG",   LOG_LEVEL_DEBUG},
    {"INFO",    LOG_LEVEL_INFO },
    {"WARN",    LOG_LEVEL_WARN },
    {"ERROR",   LOG_LEVEL_ERROR},
    {"ALL",     LOG_LEVEL_ALL  },
};

/******************************************************************************
* Functions.
******************************************************************************/
int WriteIniStr(const char *path, const char *sector, const char *key, char* value_str)
{
    int ret = 0;
    
    ret = LoadConfFile(path);
    if (ret) {
        LOG_ERROR("load config file [%s] fail when update ad config paramaters", path);
        return RET_ERROR;
    } else {
        /* write */
        if (INI_RET_OK != SetSecStr(sector, key, value_str)) {
            return RET_ERROR;
        }
        return RET_OK;
    }
}

int WriteIniInt(const char *path, const char *sector, const char *key, int* value_int)
{
    int ret = 0;
    
    ret = LoadConfFile(path);
    if (ret) {
        LOG_ERROR("load config file [%s] fail when update ad config paramaters", path);
        return RET_ERROR;
    } else {
        /* write */
        if (INI_RET_OK != SetSecInt(sector, key, value_int)) {
            return RET_ERROR;
        }
        return RET_OK;
    }
}

int WriteIniFloat(const char *path, const char *sector, const char *key, float* value_float)
{
    int ret = 0;
    
    ret = LoadConfFile(path);
    if (ret) {
        LOG_ERROR("load config file [%s] failed.", path);
        return RET_ERROR;
    } else {
        /* write */
        if (INI_RET_OK != SetSecFloat(sector, key, value_float)) {
            return RET_ERROR;
        }
        return RET_OK;
    }
}

int WriteIniDouble(const char *path, const char *sector, const char *key, double* value_double)
{
    int ret = 0;
    
    ret = LoadConfFile(path);
    if (ret) {
        LOG_ERROR("load config file [%s] failed.", path);
        return RET_ERROR;
    } else {
        /* write */
        if (INI_RET_OK != SetSecDouble(sector, key, value_double)) {
            return RET_ERROR;
        }
        return RET_OK;
    }
}

int ReadLogCfg(const char *sector, LogCfg* temp)
{
    int ret = RET_OK;
    char tmp_value[64] = "";
    ret = LoadConfFile(IDA_DEVICE_LOG_FILE);
    if (ret) {
        printf("load config file [%s] fail !\n", IDA_DEVICE_LOG_FILE);
        return RET_ERROR;
    } else {
        /* get config info from .ini */
        if (INI_RET_OK != GetSecStr(sector, INI_KEY_PATH, tmp_value)) {
            printf("ReadLogCfg [%s/%s = %s] \n", sector, INI_KEY_PATH, tmp_value);
            return RET_ERROR;
        }
        strcpy(temp->log_path, tmp_value);
        printf("ReadLogCfg [%s/%s = %s] \n", sector, INI_KEY_PATH, tmp_value);

        if (INI_RET_OK != GetSecStr(sector, INI_KEY_LEVEL, tmp_value)) {
            printf("ReadLogCfg [%s/%s = %s] \n", sector, INI_KEY_LEVEL, tmp_value);
            return RET_ERROR;
        }
        strcpy(temp->log_level, tmp_value);
        printf("ReadLogCfg [%s/%s = %s] \n", sector, INI_KEY_LEVEL, tmp_value);

        for(uint16_t i=0; i<(sizeof(g_log_level_name)/sizeof(g_log_level_name[0])); i++)
        {
            if (0 == strcmp(temp->log_level, g_log_level_name[i].log_name)) {
                temp->log_level_int = g_log_level_name[i].log_level;
                break;
            }
        }
        printf("get log config message ok ! [tmp_value = %s %d] \n", temp->log_level, temp->log_level_int);
    }
    return RET_OK;
}

int ReadUartCfg(const char *sector, UartCfg* temp)
{
    int ret = RET_OK;
    int tmp_value = 0;
    char tmp_str[8] = "";
    ret = LoadConfFile(IDA_DEVICE_UART_FILE);
    if (ret) {
        LOG_INFO("load config file [%s] fail.", IDA_DEVICE_UART_FILE);
        return RET_ERROR;
    } else {
        /* get config info from .ini */
        if (INI_RET_OK != GetSecInt(sector, INI_KEY_UART_RATE, &tmp_value)) {
            LOG_ERROR("ReadUartCfg [%s/%s = %d]", sector, INI_KEY_UART_RATE, tmp_value);
            return RET_ERROR;
        }
        temp->rate = tmp_value;
        LOG_INFO("ReadUartCfg [%s/%s = %d]", sector, INI_KEY_UART_RATE, tmp_value);

        if (INI_RET_OK != GetSecInt(sector, INI_KEY_UART_BIT, &tmp_value)) {
            LOG_ERROR("ReadUartCfg [%s/%s = %d]", sector, INI_KEY_UART_BIT, tmp_value);
            return RET_ERROR;
        }
        temp->databit = tmp_value;
        LOG_INFO("ReadUartCfg [%s/%s = %d]", sector, INI_KEY_UART_BIT, tmp_value);

        if (INI_RET_OK != GetSecInt(sector, INI_KEY_UART_STOP, &tmp_value)) {
            LOG_ERROR("ReadUartCfg [%s/%s = %d]", sector, INI_KEY_UART_STOP, tmp_value);
            return RET_ERROR;
        }
        temp->datastop = tmp_value;
        LOG_INFO("ReadUartCfg [%s/%s = %d]", sector, INI_KEY_UART_STOP, tmp_value);

        if (INI_RET_OK != GetSecStr(sector, INI_KEY_UART_CHECK, tmp_str)) {
            LOG_ERROR("ReadUartCfg [%s/%s = %s]", sector, INI_KEY_UART_CHECK, tmp_str);
            return RET_ERROR;
        }
        // temp->datacheck = tmp_value;
        // strcpy(&(temp->datacheck), tmp_str);
        memcpy(&(temp->datacheck), tmp_str, sizeof(int));
        LOG_INFO("ReadUartCfg [%s/%s = %s] 0x%04x", sector, INI_KEY_UART_CHECK, tmp_str, temp->datacheck);
    }
    return RET_OK;
}

int ReadNetTcpCfg(NetTcpCfg* temp)
{
    int ret = RET_OK;
    int tmp_value = 0;
    char tmp_str[64] = "";
    ret = LoadConfFile(IDA_DEVICE_NET_FILE);
    if (ret) {
        LOG_INFO("load config file [%s] fail.", IDA_DEVICE_NET_FILE);
        return RET_ERROR;
    } else {
        /* get config info from .ini */
        // read server_ip
        if (INI_RET_OK != GetSecStr(INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_SERVER_IP, tmp_str)) {
            LOG_ERROR("ReadNetCfg [%s/%s = %s]", INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_SERVER_IP, tmp_str);
            return RET_ERROR;
        }
        strcpy(temp->server_ip, tmp_str);
        LOG_INFO("ReadNetCfg [%s/%s = %s]", INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_SERVER_IP, tmp_str);
        // read local_ip
        if (INI_RET_OK != GetSecStr(INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_LOCAL_IP, tmp_str)) {
            LOG_ERROR("ReadNetCfg [%s/%s = %s]", INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_LOCAL_IP, tmp_str);
            return RET_ERROR;
        }
        strcpy(temp->local_ip, tmp_str);
        LOG_INFO("ReadNetCfg [%s/%s = %s]", INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_LOCAL_IP, tmp_str);
        // read mask
        if (INI_RET_OK != GetSecStr(INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_MASK, tmp_str)) {
            LOG_ERROR("ReadNetCfg [%s/%s = %s]", INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_MASK, tmp_str);
            return RET_ERROR;
        }
        strcpy(temp->mask, tmp_str);
        LOG_INFO("ReadNetCfg [%s/%s = %s]", INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_MASK, tmp_str);
        // read gateway
        if (INI_RET_OK != GetSecStr(INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_GATEWAY, tmp_str)) {
            LOG_ERROR("ReadNetCfg [%s/%s = %s]", INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_GATEWAY, tmp_str);
            return RET_ERROR;
        }
        strcpy(temp->gateway, tmp_str);
        LOG_INFO("ReadNetCfg [%s/%s = %s]", INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_GATEWAY, tmp_str);

        // read dhcp_status
        if (INI_RET_OK != GetSecInt(INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_DHCP_STATUS, &tmp_value)) {
            LOG_ERROR("ReadNetCfg [%s/%s = %d]", INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_DHCP_STATUS, tmp_value);
            return RET_ERROR;
        }
        temp->dhcp_status = tmp_value;
        LOG_INFO("ReadNetCfg [%s/%s = %d]", INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_DHCP_STATUS, tmp_value);
        // read server_port
        if (INI_RET_OK != GetSecInt(INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_SERVER_PORT, &tmp_value)) {
            LOG_ERROR("ReadNetCfg [%s/%s = %d]", INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_SERVER_PORT, tmp_value);
            return RET_ERROR;
        }
        temp->server_port = tmp_value;
        LOG_INFO("ReadNetCfg [%s/%s = %d]", INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_SERVER_PORT, tmp_value);
    }
    return RET_OK;
}

int ReadNetUdpCfg(NetUdpCfg* temp)
{
    int ret = RET_OK;
    int tmp_value = 0;
    ret = LoadConfFile(IDA_DEVICE_NET_FILE);
    if (ret) {
        LOG_INFO("load config file [%s] fail.", IDA_DEVICE_NET_FILE);
        return RET_ERROR;
    } else {
        /* get config info from .ini */
        // read udp_status
        if (INI_RET_OK != GetSecInt(INI_SECTION_SERVER_UDP, INI_KEY_SERVER_UDP_STATUS, &tmp_value)) {
            LOG_ERROR("ReadNetCfg [%s/%s = %d]", INI_SECTION_SERVER_UDP, INI_KEY_SERVER_UDP_STATUS, tmp_value);
            return RET_ERROR;
        }
        temp->udp_status = tmp_value;
        LOG_INFO("ReadNetCfg [%s/%s = %d]", INI_SECTION_SERVER_UDP, INI_KEY_SERVER_UDP_STATUS, tmp_value);
        // read udp_rx_port
        if (INI_RET_OK != GetSecInt(INI_SECTION_SERVER_UDP, INI_KEY_SERVER_UDP_RX_PORT, &tmp_value)) {
            LOG_ERROR("ReadNetCfg [%s/%s = %d]", INI_SECTION_SERVER_UDP, INI_KEY_SERVER_UDP_RX_PORT, tmp_value);
            return RET_ERROR;
        }
        temp->udp_rx_port = tmp_value;
        LOG_INFO("ReadNetCfg [%s/%s = %d]", INI_SECTION_SERVER_UDP, INI_KEY_SERVER_UDP_RX_PORT, tmp_value);
        // read udp_tx_port
        if (INI_RET_OK != GetSecInt(INI_SECTION_SERVER_UDP, INI_KEY_SERVER_UDP_TX_PORT, &tmp_value)) {
            LOG_ERROR("ReadNetCfg [%s/%s = %d]", INI_SECTION_SERVER_UDP, INI_KEY_SERVER_UDP_TX_PORT, tmp_value);
            return RET_ERROR;
        }
        temp->udp_tx_port = tmp_value;
        LOG_INFO("ReadNetCfg [%s/%s = %d]", INI_SECTION_SERVER_UDP, INI_KEY_SERVER_UDP_TX_PORT, tmp_value);
    }
    return RET_OK;
}

int ReadNetWifiCfg(NetWifiCfg* temp)
{
    int ret = RET_OK;
    int tmp_value = 0;
    char tmp_str[32] = "";
    ret = LoadConfFile(IDA_DEVICE_NET_FILE);
    if (ret) {
        LOG_INFO("load config file [%s] fail.", IDA_DEVICE_NET_FILE);
        return RET_ERROR;
    } else {
        /* get config info from .ini */
        // read wifi status
        if (INI_RET_OK != GetSecInt(INI_SECTION_WIFI_INFO, INI_KEY_WIFI_STATUS, &tmp_value)) {
            LOG_ERROR("ReadNetCfg [%s/%s = %d]", INI_SECTION_WIFI_INFO, INI_KEY_WIFI_STATUS, tmp_value);
            return RET_ERROR;
        }
        temp->wifi_status = tmp_value;
        LOG_INFO("ReadNetCfg [%s/%s = %d]", INI_SECTION_WIFI_INFO, INI_KEY_WIFI_STATUS, tmp_value);
        // read wifi username
        if (INI_RET_OK != GetSecStr(INI_SECTION_WIFI_INFO, INI_KEY_WIFI_USERNAME, tmp_str)) {
            LOG_ERROR("ReadNetCfg [%s/%s = %s]", INI_SECTION_WIFI_INFO, INI_KEY_WIFI_USERNAME, tmp_str);
            return RET_ERROR;
        }
        strcpy(temp->username, tmp_str);
        LOG_INFO("ReadNetCfg [%s/%s = %s]", INI_SECTION_WIFI_INFO, INI_KEY_WIFI_USERNAME, tmp_str);
        // read wifi password
        if (INI_RET_OK != GetSecStr(INI_SECTION_WIFI_INFO, INI_KEY_WIFI_PASSWORD, tmp_str)) {
            LOG_ERROR("ReadNetCfg [%s/%s = %s]", INI_SECTION_WIFI_INFO, INI_KEY_WIFI_PASSWORD, tmp_str);
            return RET_ERROR;
        }
        strcpy(temp->password, tmp_str);
        LOG_INFO("ReadNetCfg [%s/%s = %s]", INI_SECTION_WIFI_INFO, INI_KEY_WIFI_PASSWORD, tmp_str);
    }
    return RET_OK;
}

int ReadDeviceInfoCfg(const char *sector, DevInfoCfg* temp)
{
    int ret = RET_OK;
    int tmp_value = 0;
    char tmp_str[64] = "";
    ret = LoadConfFile(IDA_DEVICE_INFO_FILE);
    if (ret) {
        LOG_INFO("load config file [%s] fail.", IDA_DEVICE_INFO_FILE);
        return RET_ERROR;
    } else {
        /* get config info from .ini */
        if (INI_RET_OK != GetSecStr(sector, INI_KEY_DEV_TYPE, tmp_str)) {
            LOG_ERROR("ReadDeviceInfoCfg [%s/%s = %s]", sector, INI_KEY_DEV_TYPE, tmp_str);
            return RET_ERROR;
        }
        strcpy(temp->dev_type, tmp_str);
        LOG_INFO("ReadDeviceInfoCfg [%s/%s = %s]", sector, INI_KEY_DEV_TYPE, tmp_str);

        if (INI_RET_OK != GetSecStr(sector, INI_KEY_DEV_ID, tmp_str)) {
            LOG_ERROR("ReadDeviceInfoCfg [%s/%s = %s]", sector, INI_KEY_DEV_ID, tmp_str);
            return RET_ERROR;
        }
        strcpy(temp->dev_id, tmp_str);
        LOG_INFO("ReadDeviceInfoCfg [%s/%s = %s]", sector, INI_KEY_DEV_ID, tmp_str);

        if (INI_RET_OK != GetSecStr(sector, INI_KEY_DEV_HARD_VER, tmp_str)) {
            LOG_ERROR("ReadDeviceInfoCfg [%s/%s = %s]", sector, INI_KEY_DEV_HARD_VER, tmp_str);
            return RET_ERROR;
        }
        strcpy(temp->dev_hard_version, tmp_str);
        LOG_INFO("ReadDeviceInfoCfg [%s/%s = %s]", sector, INI_KEY_DEV_HARD_VER, tmp_str);

        if (INI_RET_OK != GetSecStr(sector, INI_KEY_DEV_SOFT_VER, tmp_str)) {
            LOG_ERROR("ReadDeviceInfoCfg [%s/%s = %s]", sector, INI_KEY_DEV_SOFT_VER, tmp_str);
            return RET_ERROR;
        }
        strcpy(temp->dev_soft_version, tmp_str);
        LOG_INFO("ReadDeviceInfoCfg [%s/%s = %s]", sector, INI_KEY_DEV_SOFT_VER, tmp_str);

        if (INI_RET_OK != GetSecStr(sector, INI_KEY_DEV_DRIVER_VER, tmp_str)) {
            LOG_ERROR("ReadDeviceInfoCfg [%s/%s = %s]", sector, INI_KEY_DEV_DRIVER_VER, tmp_str);
            return RET_ERROR;
        }
        strcpy(temp->dev_driver_version, tmp_str);
        LOG_INFO("ReadDeviceInfoCfg [%s/%s = %s]", sector, INI_KEY_DEV_DRIVER_VER, tmp_str);

        if (INI_RET_OK != GetSecStr(sector, INI_KEY_DEV_FPGA_VER, tmp_str)) {
            LOG_ERROR("ReadDeviceInfoCfg [%s/%s = %s]", sector, INI_KEY_DEV_FPGA_VER, tmp_str);
            return RET_ERROR;
        }
        strcpy(temp->dev_fpga_version, tmp_str);
        LOG_INFO("ReadDeviceInfoCfg [%s/%s = %s]", sector, INI_KEY_DEV_FPGA_VER, tmp_str);

        if (INI_RET_OK != GetSecInt(sector, INI_KEY_DEV_NUM, &tmp_value)) {
            LOG_ERROR("ReadDeviceInfoCfg [%s/%s = %d]", sector, INI_KEY_DEV_NUM, tmp_value);
            return RET_ERROR;
        }
        temp->dev_num = tmp_value;
        LOG_INFO("ReadDeviceInfoCfg [%s/%s = %d]", sector, INI_KEY_DEV_NUM, tmp_value);
        
    }
    return RET_OK;
}

int ReadThreshlodCfg(const char *sector, ThreshlodCfg* temp)
{
    int ret = RET_OK;
    int tmp_value = 0;
    ret = LoadConfFile(IDA_DEVICE_THRESHOLD_FILE);
    if (ret) {
        LOG_INFO("load config file [%s] fail.", IDA_DEVICE_THRESHOLD_FILE);
        return RET_ERROR;
    } else {
        /* get config info from .ini */
        if (INI_RET_OK != GetSecInt(sector, INI_KEY_MAX, &tmp_value)) {
            LOG_ERROR("ReadLogCfg [%s/%s = %d]", sector, INI_KEY_MAX, tmp_value);
            return RET_ERROR;
        }
        temp->max = tmp_value;
        LOG_INFO("ReadLogCfg [%s/%s = %d]", sector, INI_KEY_MAX, tmp_value);
        
        if (INI_RET_OK != GetSecInt(sector, INI_KEY_MIN, &tmp_value)) {
            LOG_ERROR("ReadLogCfg [%s/%s = %d]", sector, INI_KEY_MIN, tmp_value);
            return RET_ERROR;
        }
        temp->min = tmp_value;
        LOG_INFO("ReadLogCfg [%s/%s = %d]", sector, INI_KEY_MIN, tmp_value);
    }
    return RET_OK;
}

/* get software version file */
int get_software_version(char *str_data)
{
    char str[MAX_VERSION_LEN] = {0};

    int fd = open(SOFTWARE_VERSION_FILE, O_RDWR);
    if (fd < 0)
    {
        LOG_ERROR("get_software_version() Open %s err", SOFTWARE_VERSION_FILE);
        return RET_ERROR;
    }
    int ret = read(fd, str, MAX_VERSION_LEN);
    if (ret<0)
    {
        LOG_ERROR("Failed to read software version! ret:%d", ret);
        ret = RET_ERROR;
    }
    else {
        strncpy(str_data, str, strlen(str)-1);
        LOG_DEBUG("[soft_ver: %s]", str_data);
        ret = RET_OK;
    }
    close(fd);
    return ret;
}
/* get driver version file */
int get_driver_version(char *str_data)
{
    char str[MAX_VERSION_LEN] = {0};

    int fd = open(DRIVER_VERSION_FILE, O_RDWR);
    if (fd < 0)
    {
        LOG_ERROR("get_driver_version() Open %s err", DRIVER_VERSION_FILE);
        return RET_ERROR;
    }
    int ret = read(fd, str, MAX_VERSION_LEN);
    if (ret<0)
    {
        LOG_ERROR("Failed to read driver version! ret:%d", ret);
        ret = RET_ERROR;
    }
    else {
        strncpy(str_data, str, strlen(str)-1);
        LOG_DEBUG("[soft_ver: %s]", str_data);
        ret = RET_OK;
    }
    close(fd);
    return ret;
}

/* get fpga version file */
void get_fpga_version(char *str_data)
{
    char str[MAX_VERSION_LEN] = {0};
    unsigned int f_ver = 0;
    memset(str, 0, sizeof(str));
    f_ver = PsGetFpgaConfigInterface(GET_FPGA_VERSION);
    if (f_ver == F_RET_ERR) {
        str_data = NULL;
        return;
    }
    snprintf(str, MAX_VERSION_LEN, "%x", f_ver);
    // Get_FPGA_version(str);
    strncpy(str_data, str, strlen(str));
}

int WriteNetTcpCfg(NetTcpCfg* temp)
{
    int ret = 0;
    char *tmp_str;
    int tmp_data = 0;

    ret = LoadConfFile(IDA_DEVICE_NET_FILE);
    if (ret) {
        LOG_ERROR("load config file [%s] fail when update net information", IDA_DEVICE_NET_FILE);
        return RET_ERROR;
    }
    else{
        ret = 0;
        /* write NetClient config info */
        // server_ip
        tmp_str = temp->server_ip;
        if (INI_RET_OK != SetSecStr(INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_SERVER_IP, tmp_str)) {
            LOG_ERROR("update %s = %s failed!", INI_KEY_CLIENT_SERVER_IP, tmp_str);
            ret = 1;        
        } else {
            LOG_INFO("write %s = %s ok!", INI_KEY_CLIENT_SERVER_IP, tmp_str);
        }
        // dhcp_status
        tmp_data = temp->dhcp_status;
        if (INI_RET_OK != SetSecInt(INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_DHCP_STATUS, &tmp_data)) {
            LOG_ERROR("update %s = %d failed!", INI_KEY_CLIENT_DHCP_STATUS, tmp_data);
            ret = 1;        
        } else {
            LOG_INFO("write %s = %d ok!", INI_KEY_CLIENT_DHCP_STATUS, tmp_data);
        }
        // local_ip
        tmp_str = temp->local_ip;
        if (INI_RET_OK != SetSecStr(INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_LOCAL_IP, tmp_str)) {
            LOG_ERROR("update %s = %d failed!", INI_KEY_CLIENT_LOCAL_IP, tmp_str);
            ret = 1;        
        } else {
            LOG_INFO("write %s = %s ok!", INI_KEY_CLIENT_LOCAL_IP, tmp_str);
        }
        // mask
        tmp_str = temp->mask;
        if (INI_RET_OK != SetSecStr(INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_MASK, tmp_str)) {
            LOG_ERROR("update %s = %d failed!", INI_KEY_CLIENT_MASK, tmp_str);
            ret = 1;        
        } else {
            LOG_INFO("write %s = %s ok!", INI_KEY_CLIENT_MASK, tmp_str);
        }
        // gateway
        tmp_str = temp->gateway;
        if (INI_RET_OK != SetSecStr(INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_GATEWAY, tmp_str)) {
            LOG_ERROR("update %s = %d failed!", INI_KEY_CLIENT_GATEWAY, tmp_str);
            ret = 1;        
        } else {
            LOG_INFO("write %s = %s ok!", INI_KEY_CLIENT_GATEWAY, tmp_str);
        }
        // server_port
        tmp_data = temp->server_port;
        if (INI_RET_OK != SetSecInt(INI_SECTION_CLIENT_NETWORK, INI_KEY_CLIENT_SERVER_PORT, &tmp_data)) {
            LOG_ERROR("update %s = %s failed!", INI_KEY_CLIENT_SERVER_PORT, tmp_data);
            ret = 1;        
        } else {
            LOG_INFO("write %s = %d ok!", INI_KEY_CLIENT_SERVER_PORT, tmp_data);
        }
        
        if (0 == ret) {
            LOG_INFO("update tcp info file successfully!", NULL);
            return RET_OK;
        }
        return RET_ERROR;
    }    
}
int WriteNetUdpCfg(NetUdpCfg* temp)
{
    int ret = 0;
    int tmp_data = 0;

    ret = LoadConfFile(IDA_DEVICE_NET_FILE);
    if (ret) {
        LOG_ERROR("load config file [%s] fail when update net information", IDA_DEVICE_NET_FILE);
        return RET_ERROR;
    }
    else{
        ret = 0;
        /* write ServerUdp config info */
        // udp_status
        tmp_data = temp->udp_status;
        if (INI_RET_OK != SetSecInt(INI_SECTION_SERVER_UDP, INI_KEY_SERVER_UDP_STATUS, &tmp_data)) {
            LOG_ERROR("update %s = %s failed!", INI_KEY_SERVER_UDP_STATUS, tmp_data);
            ret = 1;        
        } else {
            LOG_INFO("write %s = %d ok!", INI_KEY_SERVER_UDP_STATUS, tmp_data);
        }
        // udp_rx_port
        tmp_data = temp->udp_rx_port;
        if (INI_RET_OK != SetSecInt(INI_SECTION_SERVER_UDP, INI_KEY_SERVER_UDP_RX_PORT, &tmp_data)) {
            LOG_ERROR("update %s = %s failed!", INI_KEY_SERVER_UDP_RX_PORT, tmp_data);
            ret = 1;        
        } else {
            LOG_INFO("write %s = %d ok!", INI_KEY_SERVER_UDP_RX_PORT, tmp_data);
        }
        // udp_tx_port
        tmp_data = temp->udp_tx_port;
        if (INI_RET_OK != SetSecInt(INI_SECTION_SERVER_UDP, INI_KEY_SERVER_UDP_TX_PORT, &tmp_data)) {
            LOG_ERROR("update %s = %s failed!", INI_KEY_SERVER_UDP_TX_PORT, tmp_data);
            ret = 1;        
        } else {
            LOG_INFO("write %s = %d ok!", INI_KEY_SERVER_UDP_TX_PORT, tmp_data);
        }
        if (0 == ret) {
            LOG_INFO("update udp info file successfully!", NULL);
            return RET_OK;
        }
        return RET_ERROR;
    }
}

int WriteNetWifiCfg(NetWifiCfg* temp)
{
    int ret = 0;
    int tmp_data = 0;
    char *tmp_str;

    ret = LoadConfFile(IDA_DEVICE_NET_FILE);
    if (ret) {
        LOG_ERROR("load config file [%s] fail when update net information", IDA_DEVICE_NET_FILE);
        return RET_ERROR;
    }
    else{
        ret = 0;
        /* write wifi config info */
        // wifi_status
        tmp_data = temp->wifi_status;
        if (INI_RET_OK != SetSecInt(INI_SECTION_WIFI_INFO, INI_KEY_WIFI_STATUS, &tmp_data)) {
            LOG_ERROR("update %s = %d failed!", INI_KEY_WIFI_STATUS, tmp_data);
            ret = 1;        
        } else {
            LOG_INFO("write %s = %d ok!", INI_KEY_WIFI_STATUS, tmp_data);
        }
        // username
        tmp_str = temp->username;
        if (INI_RET_OK != SetSecStr(INI_SECTION_WIFI_INFO, INI_KEY_WIFI_USERNAME, tmp_str)) {
            LOG_ERROR("update %s = %d failed!", INI_KEY_WIFI_USERNAME, tmp_str);
            ret = 1;        
        } else {
            LOG_INFO("write %s = %s ok!", INI_KEY_WIFI_USERNAME, tmp_str);
        }
        // password
        tmp_str = temp->password;
        if (INI_RET_OK != SetSecStr(INI_SECTION_WIFI_INFO, INI_KEY_WIFI_PASSWORD, tmp_str)) {
            LOG_ERROR("update %s = %d failed!", INI_KEY_WIFI_PASSWORD, tmp_str);
            ret = 1;        
        } else {
            LOG_INFO("write %s = %s ok!", INI_KEY_WIFI_PASSWORD, tmp_str);
        }
        if (0 == ret) {
            LOG_INFO("update wifi info file successfully!", NULL);
            return RET_OK;
        }
        return RET_ERROR;
    }
}

/* update device_info_file */
int WriteDeviceInfoCfg(const char *sector, DevInfoCfg* dev_data)
{
    int i = 0, ret = 0;
    
    ret = LoadConfFile(IDA_DEVICE_INFO_FILE);
    if (ret) {
        LOG_ERROR("load config file [%s] fail when update ad config paramaters", IDA_DEVICE_INFO_FILE);
        return RET_ERROR;
    }
    else{
        /* write ida device infomation */
        // dev_type
        if (INI_RET_OK != SetSecStr(sector, INI_KEY_DEV_TYPE, dev_data->dev_type)) {
            LOG_ERROR("update [ %s ] failed! [dev_type = %s]", IDA_DEVICE_INFO_FILE, dev_data->dev_type);
            i |= 0x01;
        }
        // dev_id
        if (INI_RET_OK != SetSecStr(sector, INI_KEY_DEV_ID, dev_data->dev_id))
        {
            LOG_ERROR("update [ %s ] failed! [dev_id = %s]", IDA_DEVICE_INFO_FILE, dev_data->dev_id);
            i |= 0x02;
        }
        // dev_hard_version
        if (INI_RET_OK != SetSecStr(sector, INI_KEY_DEV_HARD_VER, dev_data->dev_hard_version))
        {
            LOG_ERROR("update [ %s ] failed! [dev_hard_ver = %s]", IDA_DEVICE_INFO_FILE, dev_data->dev_hard_version);
            i |= 0x02;
        }
        // dev_soft_version
        if (INI_RET_OK != SetSecStr(sector, INI_KEY_DEV_SOFT_VER, dev_data->dev_soft_version))
        {
            LOG_ERROR("update [ %s ] failed! [dev_soft_ver = %s]", IDA_DEVICE_INFO_FILE, dev_data->dev_soft_version);
            i |= 0x04;
        }
        // dev_driver_version
        if (INI_RET_OK != SetSecStr(sector, INI_KEY_DEV_DRIVER_VER, dev_data->dev_driver_version))
        {
            LOG_ERROR("update [ %s ] failed! [dev_diver_ver = %s]", IDA_DEVICE_INFO_FILE, dev_data->dev_driver_version);
            i |= 0x08;
        }
        // dev_fpga_version
        if (INI_RET_OK != SetSecStr(sector, INI_KEY_DEV_FPGA_VER, dev_data->dev_fpga_version))
        {
            LOG_ERROR("update [ %s ] failed! [dev_fpga_ver = %s]", IDA_DEVICE_INFO_FILE, dev_data->dev_fpga_version);
            i |= 0x10;
        }
        // dev_num
        if (INI_RET_OK != SetSecInt(sector, INI_KEY_DEV_NUM, &(dev_data->dev_num)))
        {
            LOG_ERROR("update [ %s ] failed! [dev_num = %s]", IDA_DEVICE_INFO_FILE, dev_data->dev_num);
            i |= 0x11;
        }
        if (0 != i)
        {
            LOG_ERROR("update [ %s ] failed!", IDA_DEVICE_INFO_FILE);
            return RET_ERROR;
        }
        LOG_INFO("update [ %s ] successfully!", IDA_DEVICE_INFO_FILE);
        return RET_OK;
    }
}

int ReadDeviceStatusFile(DevStatusPram* temp)
{
    int ret = RET_OK;
    float tmp_value = 0;
    ret = LoadConfFile(IDA_DEVICE_STATUS_FILE);
    if (ret) {
        LOG_INFO("load config file [%s] fail.", IDA_DEVICE_STATUS_FILE);
        return RET_ERROR;
    } else {
        /* get devide status info from .ini */
        if (INI_RET_OK != GetSecFloat(INI_SECTION_PS, INI_KEY_TEMP, &tmp_value)) {
            LOG_ERROR("ReadDeviceStatusFile [%s/%s = %f]", INI_SECTION_PS, INI_KEY_TEMP, tmp_value);
            return RET_ERROR;
        }
        temp->ps_temp = tmp_value;
        LOG_INFO("ReadDeviceStatusFile [%s/%s = %f]", INI_SECTION_PS, INI_KEY_TEMP, tmp_value);
        
        if (INI_RET_OK != GetSecFloat(INI_SECTION_PS, INI_KEY_MEM_OCCUPY, &tmp_value)) {
            LOG_ERROR("ReadDeviceStatusFile [%s/%s = %f]", INI_SECTION_PS, INI_KEY_MEM_OCCUPY, tmp_value);
            return RET_ERROR;
        }
        temp->ps_mem_occupy = tmp_value;
        LOG_INFO("ReadDeviceStatusFile [%s/%s = %f]", INI_SECTION_PS, INI_KEY_MEM_OCCUPY, tmp_value);

        if (INI_RET_OK != GetSecFloat(INI_SECTION_PS, INI_KEY_CPU_OCCUPY, &tmp_value)) {
            LOG_ERROR("ReadDeviceStatusFile [%s/%s = %f]", INI_SECTION_PS, INI_KEY_CPU_OCCUPY, tmp_value);
            return RET_ERROR;
        }
        temp->ps_cpu_occupy = tmp_value;
        LOG_INFO("ReadDeviceStatusFile [%s/%s = %f]", INI_SECTION_PS, INI_KEY_CPU_OCCUPY, tmp_value);
    }
    return RET_OK;
}


