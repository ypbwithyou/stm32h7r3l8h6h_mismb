#include "rs485_processor.h"
#include "./BSP/RS485/rs485.h"
#include "string.h"
#include "usb_processor.h"
#include "./LIBS/lib_usb_protocol/usb_protocol.h"

static uint8_t rs485_calculate_checksum(uint8_t *data, uint8_t len);
static uint8_t rs485_calculate_checkxor(const uint8_t *data, uint32_t length);

/**
 * @brief       解析RS485数据帧
 * @param       data: 数据指针（不包括帧头帧尾）
 * @param       len: 数据长度
 * @retval      无
 */
void rs485_parse_frame(uint8_t *data, uint8_t len)
{
    /* 检查数据有效性 */
    if (data == NULL || len == 0) {
        return;
    }
    
    /* 根据协议解析数据
       示例协议：
       字节1: 帧头（0x3c）
       字节2: 设备号（1--8）
       字节3: 功能码
       字节4、5: 数据长度N
       字节5--(5+N-2): 数据N
       倒数第二字节: 异或校验结果
       最后一个字节: 帧尾（0x3e）
    */
    
    if (len < 7) { /* 至少帧头+设备号+功能码+长度+校验+帧尾 */
        return;
    }
    uint8_t head = data[0];
    uint8_t dev_num = data[1];
    uint8_t command = data[2];
    uint16_t data_len = data[3];
    data_len = ((data_len<<8)|data[4]);
    uint8_t check_received = data[len - 2];
    uint8_t end = data[len - 1];
    
    /* 校验头 */
    if (head != 0x3c) {
        return;
    }
    
    /* 校验设备号范围 */
    if (dev_num<=0 || dev_num>8) {
        return;
    }
    
    /* 验证数据长度 */
    if (data_len != (len - 7)) { /* 总长度减去帧头、设备号、命令码、长度字节、校验和帧尾 */
        /* 长度不匹配，记录错误 */
        return;
    }
    
    /* 计算异或校验（除校验字节外的所有数据） */
    uint8_t check = rs485_calculate_checkxor(data, len - 2);
    
    if (check != check_received) {
        /* 校验和错误，记录错误 */
        return;
    }
    
    /* 校验帧尾*/
    if (end != 0x3e) {
        return;
    }
    
    /* 根据命令码处理数据 */
    switch (command) {
        case 0x01: /* 读设备信息帧应答 */
            if (data_len != sizeof(SubDevicelnfo)) return;
            memcpy(&g_SubDevicelnfo[dev_num-1], &data[5], data_len);
            break;
            
        case 0x03: /* 写设备信息帧应答 */
            /* 处理写入命令 */
            break;
            
        default:
            /* 未知命令 */
            break;
    }
    
    /* 可选：发送响应 */
    // uint8_t response[] = {command, 0x00, 0xAA}; /* 示例响应 */
    // rs485_send_data(response, sizeof(response));
}

/**
 * @brief       计算校验和（简单累加和）
 * @param       data: 数据指针
 * @param       len: 数据长度
 * @retval      校验和
 */
static uint8_t rs485_calculate_checksum(uint8_t *data, uint8_t len)
{
    uint8_t checksum = 0;
    
    for (uint8_t i = 0; i < len; i++) {
        checksum += data[i];
    }
    
    return checksum;
}
/**
 * @brief       计算校验异或（简单异或）
 * @param       data: 数据指针
 * @param       len: 数据长度
 * @retval      校验异或值
 */
static uint8_t rs485_calculate_checkxor(const uint8_t *data, uint32_t length)
{
    if (data == NULL || length == 0)
    {
        return 0x00; // 异常情况返回 0，或根据需求返回固定值
    }

    uint8_t xor_result = 0;

    for (uint32_t i = 0; i < length; i++)
    {
        xor_result ^= data[i];
    }

    return xor_result;
}

/**
 * @brief       发送RS485数据（对外接口）
 * @param       dev_num: 设备号（1--8）
 * @param       cmd: 命令码
 * @param       data: 数据
 * @param       len: 数据长度
 * @retval      0:成功, -1:失败
 */
int8_t rs485_send_frame(uint8_t dev_num, uint8_t cmd, uint8_t *data, uint16_t len)
{
    if (len > (RS485_RX_BUF_LEN - 7)) {
        return -1; /* 参数无效 */
    }
    
    /* 组织待发送数据 */
    uint8_t tx_buffer[RS485_RX_BUF_LEN];
    tx_buffer[0] = 0x3c;
    tx_buffer[1] = dev_num;
    tx_buffer[2] = cmd;
    if (len > 0) {
        tx_buffer[3] = len >> 8;
        tx_buffer[4] = len;
        /* 复制数据 */
        memcpy(&tx_buffer[5], data, len);        
    } else {
        tx_buffer[3] = 0;
        tx_buffer[4] = 0;
    }
    uint8_t checkxor = rs485_calculate_checkxor(tx_buffer, len+5);
    tx_buffer[5+len] = checkxor;
    tx_buffer[5+len+1] = 0x3e;
    
    /* 发送数据（会自动添加帧头帧尾） */
    rs485_send_data(tx_buffer, len + 7); /* 数据+校验 */
    
    return 0;
}

