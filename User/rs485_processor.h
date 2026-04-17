#ifndef __RS485_PROCESSOR_H
#define __RS485_PROCESSOR_H

#include <stdint.h>

#define RS485_BAUDRATE (115200U)
#define RS485_PROTO_MAX_PAYLOAD 128U
#define RS485_STARTUP_SCAN_INTERVAL_MS 100U
#define RS485_SUBDEV_MAX 8U

enum Event
{
    DEVINFO_READ_REQ = 0x00,
    DEVINFO_READ_REQ_ACK = 0x01,

    DEVINFO_WRITE_REQ = 0x02,
    DEVINFO_WRITE_REQ_ACK = 0x03,

    BRIDGE_SET = 0x04,
    BRIDGE_SET_ACK = 0x05,

    BRIDGE_GAIN_SET = 0x06,
    BRIDGE_GAIN_SET_ACK = 0x07,

    BRIDGE_DAC_SET = 0x08,
    BRIDGE_DAC_SET_ACK = 0x09,

    BRIDGE_PWM_SET = 0x0A,
    BRIDGE_PWM_SET_ACK = 0x0B,
};

typedef struct
{
    uint8_t address;
    uint8_t function;
    uint16_t data_len;
    const uint8_t *data;
} rs485_packet_t;

/* 校准设置结构体 (DAC_SET) */
typedef struct
{
    uint8_t bitflag; // 0-2bit对应通道1-3, bit=1表示设置该通道; 全0=禁用DAC, 全1=统一设置
    float voltage0;  // 通道0电压值
    float voltage1;  // 通道1电压值
    float voltage2;  // 通道2电压值
} __attribute__((packed)) dac_set_payload_t;

/* 桥路设置结构体 (BRIDGE_SET)
 * @note 桥路类型映射 (nVar1 bit15-0, 1-8):
 *       bridge字段编码: bit0-1=通道0, bit2-3=通道1, bit4-5=通道2
 *       每个通道2bits: 0=全桥, 1=半桥, 2=1/4桥
 *       nVar1映射: 1-2(1/4桥) -> 2, 3-4(1/2桥) -> 1, 5-8(全桥) -> 0
 * @note 分流校准类型映射 (nVar1 bit31-16, 0-10):
 *       bridgeShunt字段: bit0=通道0, bit1=通道1, bit2=通道2
 *       每个通道1bit: 0=不接, 1=接入
 *       只允许设置R2(值为2)，其他值视为不接
 */
typedef struct
{
    uint8_t exc_en;        // 激励使能: 0=失能, 1=使能
    uint8_t bridge;        // bit0-1:通道0, bit2-3:通道1, bit4-5:通道2; 0=全桥, 1=半桥, 2=1/4桥
    uint8_t bridgeShunt;   // bit0:通道0, bit1:通道1, bit2:通道2; 0=不接, 1=接入R2
} __attribute__((packed)) bridge_set_payload_t;

/* 增益设置结构体 (GAIN_SET)
 * @note 量程增益映射 (nInputRange 0-5):
 *       gain字段: bit0=通道0, bit1=通道1, bit2=通道2; 0=1倍, 1=10倍
 *       pga字段: bit0-2=PGA0, bit3-5=PGA1, bit6-8=PGA2
 *       PGA编码: 0=1倍, 1=2倍, 2=4倍, 4=8倍, 7=128倍
 *       nInputRange映射:
 *         0 -> gain=0(1倍), pga=1 (2.5V)
 *         1 -> gain=0(1倍), pga=2 (1.25V)
 *         2 -> gain=1(10倍), pga=10 (0.25V)
 *         3 -> gain=1(10倍), pga=20 (0.125V)
 *         4 -> gain=1(10倍), pga=128 (0.01953125V)
 *         5 -> gain=1(10倍), pga=1280 (0.001953125V)
 */
typedef struct
{
    uint8_t gain;   // bit0=通道0, bit1=通道1, bit2=通道2; 0=1倍, 1=10倍
    uint16_t pga;   // bit0-2=PGA0, bit3-5=PGA1, bit6-8=PGA2; 编码0-7对应1,2,4,8,16,32,64,128倍
} __attribute__((packed)) gain_set_payload_t;

/* Protocol layer */
int8_t rs485_build_frame(const rs485_packet_t *packet, uint8_t *out, uint16_t out_size, uint16_t *out_len);
int8_t rs485_parse_packet(const uint8_t *frame, uint16_t frame_len, rs485_packet_t *packet_out);
int8_t rs485_send_frame(uint8_t dev_num, uint8_t cmd, const uint8_t *data, uint16_t len);
void rs485_parse_frame(uint8_t *frame, uint16_t frame_len);
void rs485_processor_poll(void);

/* sub-device validity state by slave address 1..8 => index 0..7 */
extern uint8_t g_subdev_valid[RS485_SUBDEV_MAX];
extern uint32_t g_subdev_last_tick[RS485_SUBDEV_MAX];
/* write ack status: -1=no response yet, 0=success, >0=error code from device */
extern int8_t g_subdev_write_ack[RS485_SUBDEV_MAX];
uint8_t rs485_subdev_is_valid(uint8_t addr);
int8_t rs485_subdev_get_write_ack(uint8_t addr);
void rs485_subdev_clear_write_ack(uint8_t addr);
void rs485_subdev_scan_reset(void);
void rs485_subdev_scan_once(void);

/* 子板配置命令发送接口 */
int8_t rs485_subdev_set_dac(uint8_t addr, const dac_set_payload_t *dac_cfg);
int8_t rs485_subdev_set_bridge(uint8_t addr, const bridge_set_payload_t *bridge_cfg);
int8_t rs485_subdev_set_gain(uint8_t addr, const gain_set_payload_t *gain_cfg);

/* 测试函数 */
void rs485_subdev_config_test(void);

#endif
