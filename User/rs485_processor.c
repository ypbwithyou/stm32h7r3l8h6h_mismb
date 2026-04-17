#include "rs485_processor.h"
#include "./BSP/RS485/rs485.h"
#include "usb_processor.h"
#include <string.h>
#include "usbd_cdc_if.h"

uint8_t g_subdev_valid[RS485_SUBDEV_MAX] = {0};
uint32_t g_subdev_last_tick[RS485_SUBDEV_MAX] = {0};
/* write ack status: -1=no response yet, 0=success, >0=error code from device */
int8_t g_subdev_write_ack[RS485_SUBDEV_MAX] = {-1, -1, -1, -1, -1, -1, -1, -1};

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

int8_t rs485_subdev_get_write_ack(uint8_t addr)
{
    if ((addr < RS485_SLAVE_ADDR_MIN) || (addr > RS485_SLAVE_ADDR_MAX))
    {
        return -1;
    }
    return g_subdev_write_ack[addr - 1U];
}

void rs485_subdev_clear_write_ack(uint8_t addr)
{
    uint8_t i;
    if ((addr >= RS485_SLAVE_ADDR_MIN) && (addr <= RS485_SLAVE_ADDR_MAX))
    {
        g_subdev_write_ack[addr - 1U] = -1;
    }
    else if (addr == 0) /* addr=0 means clear all */
    {
        for (i = 0U; i < RS485_SUBDEV_MAX; i++)
        {
            g_subdev_write_ack[i] = -1;
        }
    }
}

void rs485_subdev_scan_reset(void)
{
    uint8_t i;

    for (i = 0U; i < RS485_SUBDEV_MAX; i++)
    {
        g_subdev_valid[i] = 0U;
        g_subdev_last_tick[i] = 0U;
        g_subdev_write_ack[i] = 0U;
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

                /* Print sub-device info */
                usb_printf("  [SubDev Info] Addr=%d\r\n", pkt.address);
                usb_printf("    - SerialNumber: %s\r\n", (char *)info->SerialNumber);
                usb_printf("    - DeviceName: %s\r\n", (char *)info->DeviceName);
                usb_printf("    - DeviceType: %d\r\n", info->DeviceType);
                usb_printf("    - SlotId: %d\r\n", info->SlotId);
                usb_printf("    - Version: %s\r\n", (char *)info->Version);
                usb_printf("    - Sensitivity: %d\r\n", info->Sensitivity);
                usb_printf("  [SubDev Info End]\r\n");
            }
        }
        break;
    case DEVINFO_WRITE_REQ_ACK:
        usb_printf("rs485_parse_frame DEVINFO_WRITE_REQ_ACK %d \n", pkt.address);
        if ((pkt.address >= RS485_SLAVE_ADDR_MIN) && (pkt.address <= RS485_SLAVE_ADDR_MAX))
        {
            /* data_len=0: legacy mode, no status; data_len=1: with status byte (0=success) */
            if (pkt.data_len == 1U)
            {
                g_subdev_write_ack[pkt.address - 1U] = (int8_t)(pkt.data[0]);
            }
            else
            {
                g_subdev_write_ack[pkt.address - 1U] = 0; /* legacy: treat as success */
            }
        }
        break;
    case BRIDGE_SET_ACK:
        usb_printf("rs485_parse_frame BRIDGE_SET_ACK %d, data_len=%d\n", pkt.address, pkt.data_len);
        if ((pkt.address >= RS485_SLAVE_ADDR_MIN) && (pkt.address <= RS485_SLAVE_ADDR_MAX))
        {
            if (pkt.data_len == 1U)
            {
                g_subdev_write_ack[pkt.address - 1U] = (int8_t)(pkt.data[0]);
            }
            else
            {
                g_subdev_write_ack[pkt.address - 1U] = 0;
            }
        }
        break;
    case GAIN_SET_ACK:
        usb_printf("rs485_parse_frame GAIN_SET_ACK %d, data_len=%d\n", pkt.address, pkt.data_len);
        if ((pkt.address >= RS485_SLAVE_ADDR_MIN) && (pkt.address <= RS485_SLAVE_ADDR_MAX))
        {
            if (pkt.data_len == 1U)
            {
                g_subdev_write_ack[pkt.address - 1U] = (int8_t)(pkt.data[0]);
            }
            else
            {
                g_subdev_write_ack[pkt.address - 1U] = 0;
            }
        }
        break;
    case DAC_SET_ACK:
        usb_printf("rs485_parse_frame DAC_SET_ACK %d, data_len=%d\n", pkt.address, pkt.data_len);
        if ((pkt.address >= RS485_SLAVE_ADDR_MIN) && (pkt.address <= RS485_SLAVE_ADDR_MAX))
        {
            if (pkt.data_len == 1U)
            {
                g_subdev_write_ack[pkt.address - 1U] = (int8_t)(pkt.data[0]);
            }
            else
            {
                g_subdev_write_ack[pkt.address - 1U] = 0;
            }
        }
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

/* 子板配置命令发送接口实现 */

/**
 * @brief 发送DAC校准设置命令到子板
 * @param addr 子板地址 (1-8)
 * @param dac_cfg DAC配置参数
 * @return 0=成功, -1=失败
 */
int8_t rs485_subdev_set_dac(uint8_t addr, const dac_set_payload_t *dac_cfg)
{
    if ((addr < RS485_SLAVE_ADDR_MIN) || (addr > RS485_SLAVE_ADDR_MAX) || (dac_cfg == NULL))
    {
        return -1;
    }
    return rs485_send_frame(addr, DAC_SET, (const uint8_t *)dac_cfg, sizeof(dac_set_payload_t));
}

/**
 * @brief 发送桥路设置命令到子板
 * @param addr 子板地址 (1-8)
 * @param bridge_cfg 桥路配置参数
 * @return 0=成功, -1=失败
 */
int8_t rs485_subdev_set_bridge(uint8_t addr, const bridge_set_payload_t *bridge_cfg)
{
    if ((addr < RS485_SLAVE_ADDR_MIN) || (addr > RS485_SLAVE_ADDR_MAX) || (bridge_cfg == NULL))
    {
        return -1;
    }
    return rs485_send_frame(addr, BRIDGE_SET, (const uint8_t *)bridge_cfg, sizeof(bridge_set_payload_t));
}

/**
 * @brief 发送增益设置命令到子板
 * @param addr 子板地址 (1-8)
 * @param gain_cfg 增益配置参数
 * @return 0=成功, -1=失败
 */
int8_t rs485_subdev_set_gain(uint8_t addr, const gain_set_payload_t *gain_cfg)
{
    if ((addr < RS485_SLAVE_ADDR_MIN) || (addr > RS485_SLAVE_ADDR_MAX) || (gain_cfg == NULL))
    {
        return -1;
    }
    return rs485_send_frame(addr, GAIN_SET, (const uint8_t *)gain_cfg, sizeof(gain_set_payload_t));
}

/* 测试函数实现 */

/**
 * @brief 测试子板配置命令
 * @note 调用此函数测试DAC、桥路、增益设置命令
 */
void rs485_subdev_config_test(void)
{
    uint8_t addr;
    int8_t ret;
    int8_t ack_status;

    usb_printf("\r\n===== RS485 Subdev Config Test Start =====\r\n");

    /* 测试DAC设置 - 统一设置3个通道为2.5V */
    for (addr = RS485_SLAVE_ADDR_MIN; addr <= RS485_SLAVE_ADDR_MAX; addr++)
    {
        if (rs485_subdev_is_valid(addr))
        {
            dac_set_payload_t dac_cfg = {
                .bitflag = 0x07, // 全1, 统一设置3个通道
                .voltage0 = 2.5f,
                .voltage1 = 2.5f,
                .voltage2 = 2.5f};
            ret = rs485_subdev_set_dac(addr, &dac_cfg);
            usb_printf("DAC_SET to addr=%d, bitflag=0x%02X, voltages={%.2f,%.2f,%.2f}, ret=%d\r\n",
                       addr, dac_cfg.bitflag, dac_cfg.voltage0, dac_cfg.voltage1, dac_cfg.voltage2, ret);

            /* 等待ACK */
            uint32_t start_tick = HAL_GetTick();
            while ((HAL_GetTick() - start_tick) < 100)
            {
                rs485_processor_poll();
                ack_status = rs485_subdev_get_write_ack(addr);
                if (ack_status >= 0) /* -1=no response, >=0=received */
                {
                    if (ack_status == 0)
                    {
                        usb_printf("DAC_SET_ACK: addr=%d SUCCESS\r\n", addr);
                    }
                    else
                    {
                        usb_printf("DAC_SET_ACK: addr=%d FAILED, err=0x%02X\r\n", addr, ack_status);
                    }
                    rs485_subdev_clear_write_ack(addr);
                    break;
                }
            }
            if (rs485_subdev_get_write_ack(addr) < 0)
            {
                usb_printf("DAC_SET: addr=%d TIMEOUT (no ACK)\r\n", addr);
            }
        }
    }

    /* 测试桥路设置 - 使能激励，全桥，接入分流电阻 */
    for (addr = RS485_SLAVE_ADDR_MIN; addr <= RS485_SLAVE_ADDR_MAX; addr++)
    {
        if (rs485_subdev_is_valid(addr))
        {
            bridge_set_payload_t bridge_cfg = {
                .exc_en = 1,        // 使能激励
                .bridge = 0x00,     // bit0-2=0, 全桥
                .bridgeShunt = 0x07 // bit0-2=1, 3个通道都接入分流电阻
            };
            ret = rs485_subdev_set_bridge(addr, &bridge_cfg);
            usb_printf("BRIDGE_SET to addr=%d, exc_en=%d, bridge=0x%02X, bridgeShunt=0x%02X, ret=%d\r\n",
                       addr, bridge_cfg.exc_en, bridge_cfg.bridge, bridge_cfg.bridgeShunt, ret);

            /* 等待ACK */
            uint32_t start_tick = HAL_GetTick();
            while ((HAL_GetTick() - start_tick) < 100)
            {
                rs485_processor_poll();
                ack_status = rs485_subdev_get_write_ack(addr);
                if (ack_status >= 0)
                {
                    if (ack_status == 0)
                    {
                        usb_printf("BRIDGE_SET_ACK: addr=%d SUCCESS\r\n", addr);
                    }
                    else
                    {
                        usb_printf("BRIDGE_SET_ACK: addr=%d FAILED, err=0x%02X\r\n", addr, ack_status);
                    }
                    rs485_subdev_clear_write_ack(addr);
                    break;
                }
            }
            if (rs485_subdev_get_write_ack(addr) < 0)
            {
                usb_printf("BRIDGE_SET: addr=%d TIMEOUT (no ACK)\r\n", addr);
            }
        }
    }

    /* 测试增益设置 - 10倍增益，PGA放大128倍 */
    for (addr = RS485_SLAVE_ADDR_MIN; addr <= RS485_SLAVE_ADDR_MAX; addr++)
    {
        if (rs485_subdev_is_valid(addr))
        {
            gain_set_payload_t gain_cfg = {
                .gain = 0x07, // bit0-2=1, 3个通道都是10倍
                .pga = 0x01C7 // bit6-8=1, bit3-5=1, bit0-2=1 (全部128倍)
            };
            ret = rs485_subdev_set_gain(addr, &gain_cfg);
            usb_printf("GAIN_SET to addr=%d, gain=0x%02X, pga=0x%04X, ret=%d\r\n",
                       addr, gain_cfg.gain, gain_cfg.pga, ret);

            /* 等待ACK */
            uint32_t start_tick = HAL_GetTick();
            while ((HAL_GetTick() - start_tick) < 100)
            {
                rs485_processor_poll();
                ack_status = rs485_subdev_get_write_ack(addr);
                if (ack_status >= 0)
                {
                    if (ack_status == 0)
                    {
                        usb_printf("GAIN_SET_ACK: addr=%d SUCCESS\r\n", addr);
                    }
                    else
                    {
                        usb_printf("GAIN_SET_ACK: addr=%d FAILED, err=0x%02X\r\n", addr, ack_status);
                    }
                    rs485_subdev_clear_write_ack(addr);
                    break;
                }
            }
            if (rs485_subdev_get_write_ack(addr) < 0)
            {
                usb_printf("GAIN_SET: addr=%d TIMEOUT (no ACK)\r\n", addr);
            }
        }
    }

    usb_printf("===== RS485 Subdev Config Test End =====\r\n");
}
