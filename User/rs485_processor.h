#ifndef __RS485_PROCESSOR_H
#define __RS485_PROCESSOR_H

#include <stdint.h>

#define RS485_BAUDRATE (115200U)
#define RS485_PROTO_MAX_PAYLOAD 128U

enum Event
{
    DEVINFO_READ_REQ = 0x00,
    DEVINFO_READ_REQ_ACK = 0x01,
    DEVINFO_WRITE_REQ = 0x02,
    DEVINFO_WRITE_REQ_ACK = 0x03,
};

typedef struct
{
    uint8_t address;
    uint8_t function;
    uint16_t data_len;
    const uint8_t *data;
} rs485_packet_t;

/* Protocol layer */
int8_t rs485_build_frame(const rs485_packet_t *packet, uint8_t *out, uint16_t out_size, uint16_t *out_len);
int8_t rs485_parse_packet(const uint8_t *frame, uint16_t frame_len, rs485_packet_t *packet_out);
int8_t rs485_send_frame(uint8_t dev_num, uint8_t cmd, const uint8_t *data, uint16_t len);
void rs485_parse_frame(uint8_t *frame, uint16_t frame_len);
void rs485_processor_poll(void);

#endif
