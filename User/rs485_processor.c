#include "rs485_processor.h"
#include "./BSP/RS485/rs485.h"
#include "usb_processor.h"
#include <string.h>
#include "usbd_cdc_if.h"

uint8_t g_subdev_valid[RS485_SUBDEV_MAX] = {0};
uint32_t g_subdev_last_tick[RS485_SUBDEV_MAX] = {0};

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

static uint8_t rs485_subdev_payload_is_valid(uint8_t addr, const SubDevicelnfo *info)
{
    if ((addr < RS485_SLAVE_ADDR_MIN) || (addr > RS485_SLAVE_ADDR_MAX) || (info == NULL))
    {
        return 0U;
    }

    if ((info->SlotId < RS485_SLAVE_ADDR_MIN) || (info->SlotId > RS485_SLAVE_ADDR_MAX))
    {
        return 0U;
    }
    if (info->SerialNumber[0] == 0)
    {
        return 0U;
    }

    return 1U;
}

uint8_t rs485_subdev_is_valid(uint8_t addr)
{
    if ((addr < RS485_SLAVE_ADDR_MIN) || (addr > RS485_SLAVE_ADDR_MAX))
    {
        return 0U;
    }
    return g_subdev_valid[addr - 1U];
}

void rs485_subdev_scan_reset(void)
{
    uint8_t i;

    for (i = 0U; i < RS485_SUBDEV_MAX; i++)
    {
        g_subdev_valid[i] = 0U;
        g_subdev_last_tick[i] = 0U;
        memset(&g_SubDevicelnfo[i], 0, sizeof(SubDevicelnfo));
    }
}

void rs485_subdev_scan_once(void)
{
    uint8_t addr;
    uint32_t start_tick;
    uint8_t frame[RS485_RX_BUF_LEN];
    uint16_t frame_len;

    rs485_subdev_scan_reset();

    for (addr = RS485_SLAVE_ADDR_MIN; addr <= RS485_SLAVE_ADDR_MAX; addr++)
    {
        usb_printf("rs485_send_frame DEVINFO_READ_REQ %d \n", addr);
        (void)rs485_send_frame(addr, DEVINFO_READ_REQ, NULL, 0U);

        start_tick = HAL_GetTick();
        while ((HAL_GetTick() - start_tick) < RS485_STARTUP_SCAN_INTERVAL_MS)
        {
            if (rs485_read_raw_frame(frame, sizeof(frame), &frame_len) == 0)
            {
                rs485_parse_frame(frame, frame_len);
            }
        }
    }
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
    /* little-endian length: low byte first, then high byte */
    out[3] = (uint8_t)(packet->data_len & 0xFFU);
    out[4] = (uint8_t)(packet->data_len >> 8);

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

    /* little-endian length: low byte first, then high byte */
    data_len = (uint16_t)(((uint16_t)frame[4] << 8) | frame[3]);
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

    if ((pkt.address < RS485_SLAVE_ADDR_MIN) || (pkt.address > RS485_SLAVE_ADDR_MAX))
    {
        return;
    }

    switch (pkt.function)
    {
    case DEVINFO_READ_REQ_ACK:
        usb_printf("rs485_parse_frame DEVINFO_READ_REQ_ACK %d \n", pkt.address);
        if (pkt.data_len == sizeof(SubDevicelnfo))
        {
            const SubDevicelnfo *info = (const SubDevicelnfo *)pkt.data;
            if (rs485_subdev_payload_is_valid(pkt.address, info))
            {
                memcpy(&g_SubDevicelnfo[pkt.address - 1U], info, sizeof(SubDevicelnfo));
                g_subdev_valid[pkt.address - 1U] = 1U;
                g_subdev_last_tick[pkt.address - 1U] = HAL_GetTick();
            }
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
    if ((dev_num < RS485_SLAVE_ADDR_MIN) || (dev_num > RS485_SLAVE_ADDR_MAX))
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

    if (rs485_read_raw_frame(frame, sizeof(frame), &frame_len) == 0)
    {
        rs485_parse_frame(frame, frame_len);
    }
}
