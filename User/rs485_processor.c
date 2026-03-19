#include "rs485_processor.h"
#include "./BSP/RS485/rs485.h"
#include "usb_processor.h"
#include <string.h>

static uint8_t rs485_xor_u8(const uint8_t *data, uint16_t len)
{
    uint8_t x = 0U;
    uint16_t i;
    if (data == NULL)
    {
        return 0U;
    }
    for (i = 0U; i < len; i++)
    {
        x ^= data[i];
    }
    return x;
}

int8_t rs485_build_frame(const rs485_packet_t *packet, uint8_t *out, uint16_t out_size, uint16_t *out_len)
{
    uint16_t frame_len;
    uint8_t xor_v;

    if ((packet == NULL) || (out == NULL) || (out_len == NULL))
    {
        return -1;
    }

    if (packet->data_len > RS485_PROTO_MAX_PAYLOAD)
    {
        return -1;
    }

    frame_len = (uint16_t)(7U + packet->data_len);
    if ((frame_len > out_size) || (frame_len > RS485_RX_BUF_LEN))
    {
        return -1;
    }

    out[0] = RS485_FRAME_HEAD;
    out[1] = packet->address;
    out[2] = packet->function;
    out[3] = (uint8_t)(packet->data_len >> 8);
    out[4] = (uint8_t)(packet->data_len & 0xFFU);

    if ((packet->data_len > 0U) && (packet->data != NULL))
    {
        memcpy(&out[5], packet->data, packet->data_len);
    }

    xor_v = rs485_xor_u8(out, (uint16_t)(5U + packet->data_len));
    out[5U + packet->data_len] = xor_v;
    out[6U + packet->data_len] = RS485_FRAME_END;

    *out_len = frame_len;
    return 0;
}

int8_t rs485_parse_packet(const uint8_t *frame, uint16_t frame_len, rs485_packet_t *packet_out)
{
    uint16_t data_len;
    uint8_t xor_expect;
    uint8_t xor_recv;

    if ((frame == NULL) || (packet_out == NULL))
    {
        return -1;
    }
    if (frame_len < 7U)
    {
        return -1;
    }
    if ((frame[0] != RS485_FRAME_HEAD) || (frame[frame_len - 1U] != RS485_FRAME_END))
    {
        return -1;
    }

    data_len = (uint16_t)(((uint16_t)frame[3] << 8) | frame[4]);
    if (data_len > RS485_PROTO_MAX_PAYLOAD)
    {
        return -1;
    }
    if (frame_len != (uint16_t)(7U + data_len))
    {
        return -1;
    }

    xor_expect = rs485_xor_u8(frame, (uint16_t)(5U + data_len));
    xor_recv = frame[5U + data_len];
    if (xor_expect != xor_recv)
    {
        return -1;
    }

    packet_out->address = frame[1];
    packet_out->function = frame[2];
    packet_out->data_len = data_len;
    packet_out->data = &frame[5];
    return 0;
}

void rs485_parse_frame(uint8_t *frame, uint16_t frame_len)
{
    rs485_packet_t pkt;

    if (rs485_parse_packet(frame, frame_len, &pkt) != 0)
    {
        return;
    }

    if ((pkt.address == 0U) || (pkt.address > SUBDEV_NUM_MAX))
    {
        return;
    }

    switch (pkt.function)
    {
    case DEVINFO_READ_REQ_ACK:
        if (pkt.data_len == sizeof(SubDevicelnfo))
        {
            memcpy(&g_SubDevicelnfo[pkt.address - 1U], pkt.data, pkt.data_len);
        }
        break;
    case DEVINFO_WRITE_REQ_ACK:
        break;
    default:
        break;
    }
}

int8_t rs485_send_frame(uint8_t dev_num, uint8_t cmd, const uint8_t *data, uint16_t len)
{
    rs485_packet_t pkt;
    uint16_t frame_len = 0U;
    uint8_t frame[RS485_RX_BUF_LEN];

    if ((len > 0U) && (data == NULL))
    {
        return -1;
    }

    pkt.address = dev_num;
    pkt.function = cmd;
    pkt.data_len = len;
    pkt.data = data;

    if (rs485_build_frame(&pkt, frame, sizeof(frame), &frame_len) != 0)
    {
        return -1;
    }

    return rs485_send_raw(frame, frame_len);
}

void rs485_processor_poll(void)
{
    uint8_t frame[RS485_RX_BUF_LEN];
    uint16_t frame_len = 0U;
    if (rs485_recv_frame_poll(frame, sizeof(frame), &frame_len, 1U) == 0)
    {
        rs485_parse_frame(frame, frame_len);
    }
}
