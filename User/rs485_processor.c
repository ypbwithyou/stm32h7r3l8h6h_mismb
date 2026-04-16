#include "rs485_processor.h"
#include "./BSP/RS485/rs485.h"
#include "usb_processor.h"
#include <string.h>
#include <stdio.h>
#include "usbd_cdc_if.h"
#include "./MALLOC/malloc.h"

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
    uint8_t devices_found_mask = 0; /* Bitmask of found devices: bit0=addr1, bit1=addr2, ... */
    uint8_t devices_new_proto[RS485_SUBDEV_MAX] = {0}; /* Track which devices use new protocol */

    rs485_subdev_scan_reset();

    usb_printf("\r\n===== RS485 Subdev Scan Start (Dual Protocol) =====\r\n");

    /* 
     * Phase 1: Scan all addresses with OLD protocol
     * Old devices only understand old format
     * New devices can auto-detect old format and respond correctly
     */
    // usb_printf("[Phase 1] Sending 512 bytes 0x00 to flush old protocol buffer...\r\n");
    // {
    //     uint8_t flush_buf[512];
    //     memset(flush_buf, 0x00, sizeof(flush_buf));
    //     (void)rs485_send_raw(flush_buf, sizeof(flush_buf));
    // }
    usb_printf("[Phase 1] Scanning with OLD protocol...\r\n");
    for (addr = RS485_SLAVE_ADDR_MIN; addr <= RS485_SLAVE_ADDR_MAX; addr++)
    {
        usb_printf("  Scanning addr=%d...\r\n", addr);
        (void)rs485_send_frame_ex(RS485_MASTER_ADDR, addr, DEVINFO_READ_REQ, NULL, 0U, RS485_PROTO_FMT_OLD);

        start_tick = HAL_GetTick();
        while ((HAL_GetTick() - start_tick) < RS485_STARTUP_SCAN_INTERVAL_MS)
        {
            if (rs485_read_raw_frame(frame, sizeof(frame), &frame_len) == 0)
            {
                rs485_parse_frame(frame, frame_len);
                /* Check if any device responded */
                for (uint8_t check_addr = RS485_SLAVE_ADDR_MIN; check_addr <= RS485_SLAVE_ADDR_MAX; check_addr++)
                {
                    if (g_subdev_valid[check_addr - 1U] && !(devices_found_mask & (1U << (check_addr - 1U))))
                    {
                        devices_found_mask |= (1U << (check_addr - 1U));
                        devices_new_proto[check_addr - 1U] = 0; /* Using OLD protocol */
                        usb_printf("    -> addr=%d responded [OLD protocol]\r\n", check_addr);
                    }
                }
            }
        }
    }


    /*
     * Phase 2: Scan remaining addresses with NEW protocol
     * Only scan addresses that didn't respond in Phase 1
     */
    // usb_printf("[Phase 2] Sending 512 bytes 0x00 to flush old protocol buffer...\r\n");
    // {
    //     uint8_t flush_buf[512];
    //     memset(flush_buf, 0x00, sizeof(flush_buf));
    //     (void)rs485_send_raw(flush_buf, sizeof(flush_buf));
    // }
    usb_printf("[Phase 2] Scanning remaining addresses with NEW protocol...\r\n");
    for (addr = RS485_SLAVE_ADDR_MIN; addr <= RS485_SLAVE_ADDR_MAX; addr++)
    {
        /* Skip if already found in Phase 1 */
        if (devices_found_mask & (1U << (addr - 1U)))
        {
            continue;
        }

        usb_printf("  Scanning addr=%d...\r\n", addr);
        (void)rs485_send_frame_ex(RS485_MASTER_ADDR, addr, DEVINFO_READ_REQ, NULL, 0U, RS485_PROTO_FMT_NEW);

        start_tick = HAL_GetTick();
        while ((HAL_GetTick() - start_tick) < RS485_STARTUP_SCAN_INTERVAL_MS)
        {
            if (rs485_read_raw_frame(frame, sizeof(frame), &frame_len) == 0)
            {
                rs485_parse_frame(frame, frame_len);
                /* Check if this specific device responded */
                if (g_subdev_valid[addr - 1U])
                {
                    devices_found_mask |= (1U << (addr - 1U));
                    devices_new_proto[addr - 1U] = 1; /* Using NEW protocol */
                    usb_printf("    -> addr=%d responded [NEW protocol]\r\n", addr);
                    break;
                }
            }
        }
    }

    /* Print scan summary with protocol info */
    usb_printf("\r\n===== RS485 Subdev Scan Summary =====\r\n");
    for (addr = RS485_SLAVE_ADDR_MIN; addr <= RS485_SLAVE_ADDR_MAX; addr++)
    {
        if (g_subdev_valid[addr - 1U])
        {
            const char *proto_str = devices_new_proto[addr - 1U] ? "NEW" : "OLD";
            usb_printf("  [OK] addr=%d is online (protocol: %s)\r\n", addr, proto_str);
        }
        else
        {
            usb_printf("  [--] addr=%d offline\r\n", addr);
        }
    }
    usb_printf("=====================================\r\n");
}

int8_t rs485_build_frame_ex(const rs485_packet_t *packet, uint8_t *out, uint16_t out_size, uint16_t *out_len, rs485_proto_format_t format)
{
    uint16_t frame_len;
    uint8_t xor_v;
    uint16_t header_len;    /* Header + address(es) + function */
    uint16_t data_offset;   /* Where data starts */

    if ((packet == NULL) || (out == NULL) || (out_len == NULL))
    {
        return -1;
    }

    if (packet->data_len > RS485_PROTO_MAX_PAYLOAD)
    {
        return -1;
    }

    /* Calculate frame structure based on format */
    if (format == RS485_PROTO_FMT_OLD)
    {
        /* Old format: Header(1) + Addr(1) + Func(1) + Len(2) + Data(n) + XOR(1) + Tail(1) = 7 + n */
        header_len = 3;     /* Header + Addr + Func */
        data_offset = 5;    /* After Len(2) */
        frame_len = (uint16_t)(7U + packet->data_len);
    }
    else
    {
        /* New format: Header(1) + SrcAddr(1) + DstAddr(1) + Func(1) + Len(2) + Data(n) + XOR(1) + Tail(1) = 8 + n */
        header_len = 4;     /* Header + SrcAddr + DstAddr + Func */
        data_offset = 6;    /* After Len(2) */
        frame_len = (uint16_t)(8U + packet->data_len);
    }

    if ((frame_len > out_size) || (frame_len > RS485_RX_BUF_LEN))
    {
        return -1;
    }

    /* Build frame header */
    out[0] = RS485_FRAME_HEAD;
    
    if (format == RS485_PROTO_FMT_OLD)
    {
        /* Old format: single address field */
        out[1] = packet->dst_addr;  /* Use dst_addr for compatibility */
        out[2] = packet->function;
        /* little-endian length: low byte first, then high byte */
        out[3] = (uint8_t)(packet->data_len & 0xFFU);
        out[4] = (uint8_t)(packet->data_len >> 8);
    }
    else
    {
        /* New format: separate src and dst addresses */
        out[1] = packet->src_addr;
        out[2] = packet->dst_addr;
        out[3] = packet->function;
        /* little-endian length: low byte first, then high byte */
        out[4] = (uint8_t)(packet->data_len & 0xFFU);
        out[5] = (uint8_t)(packet->data_len >> 8);
    }

    /* Copy payload data */
    if ((packet->data_len > 0U) && (packet->data != NULL))
    {
        memcpy(&out[data_offset], packet->data, packet->data_len);
    }

    /* Calculate XOR (from Header to end of Data) */
    xor_v = rs485_xor_u8(out, (uint16_t)(data_offset + packet->data_len));
    
    /* Set XOR and Tail */
    if (format == RS485_PROTO_FMT_OLD)
    {
        out[5U + packet->data_len] = xor_v;
        out[6U + packet->data_len] = RS485_FRAME_END;
    }
    else
    {
        out[6U + packet->data_len] = xor_v;
        out[7U + packet->data_len] = RS485_FRAME_END;
    }

    *out_len = frame_len;
    return 0;
}

int8_t rs485_parse_packet(const uint8_t *frame, uint16_t frame_len, rs485_packet_t *packet_out)
{
    uint16_t data_len;
    uint8_t xor_expect;
    uint8_t xor_recv;
    uint8_t format_detected = RS485_PROTO_FMT_NEW;  /* Default to new format */
    uint8_t xor_pos;        /* XOR byte position */
    uint8_t tail_pos;       /* Tail byte position */
    uint8_t data_offset;    /* Data start position */

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

    /* 
     * Auto-detect protocol format
     * Old format: Header(0) + Addr(1) + Func(2) + Len(3-4) + Data(5...) + XOR(5+len) + Tail(6+len)
     * New format: Header(0) + SrcAddr(1) + DstAddr(2) + Func(3) + Len(4-5) + Data(6...) + XOR(6+len) + Tail(7+len)
     * 
     * Try new format first (more strict validation)
     */
    if (frame_len >= 8U)
    {
        /* Check if it could be new format: addresses should be valid */
        uint8_t possible_src = frame[1];
        uint8_t possible_dst = frame[2];
        
        if (rs485_is_valid_address(possible_src) && rs485_is_valid_address(possible_dst))
        {
            /* New format: get length from bytes 4-5 */
            data_len = (uint16_t)(((uint16_t)frame[5] << 8) | frame[4]);
            if (data_len <= RS485_PROTO_MAX_PAYLOAD && frame_len == (uint16_t)(8U + data_len))
            {
                /* Validate XOR for new format */
                xor_pos = 6U + data_len;
                xor_expect = rs485_xor_u8(frame, (uint16_t)(6U + data_len));
                xor_recv = frame[xor_pos];
                
                if (xor_expect == xor_recv && frame[xor_pos + 1U] == RS485_FRAME_END)
                {
                    format_detected = RS485_PROTO_FMT_NEW;
                    data_offset = 6;    /* Data starts after Len(2) */
                    goto parse_success;
                }
            }
        }
    }

    /* Try old format */
    if (frame_len >= 7U)
    {
        uint8_t possible_addr = frame[1];
        
        /* Old format: get length from bytes 3-4 */
        data_len = (uint16_t)(((uint16_t)frame[4] << 8) | frame[3]);
        if (data_len <= RS485_PROTO_MAX_PAYLOAD && frame_len == (uint16_t)(7U + data_len))
        {
            /* Validate XOR for old format */
            xor_pos = 5U + data_len;
            xor_expect = rs485_xor_u8(frame, (uint16_t)(5U + data_len));
            xor_recv = frame[xor_pos];
            
            if (xor_expect == xor_recv && frame[xor_pos + 1U] == RS485_FRAME_END)
            {
                format_detected = RS485_PROTO_FMT_OLD;
                data_offset = 5;    /* Data starts after Len(2) */
                goto parse_success;
            }
        }
    }

    /* Neither format matched */
    return -1;

parse_success:
    /* Extract packet fields based on detected format */
    if (format_detected == RS485_PROTO_FMT_NEW)
    {
        packet_out->src_addr = frame[1];
        packet_out->dst_addr = frame[2];
        packet_out->function = frame[3];
    }
    else
    {
        /* Old format: single address used as both src and dst */
        packet_out->src_addr = frame[1];  /* Source is the device that sent this */
        packet_out->dst_addr = RS485_MASTER_ADDR;  /* Old format always targets master */
        packet_out->function = frame[2];
    }
    
    packet_out->data_len = data_len;
    packet_out->data = &frame[data_offset];
    packet_out->proto_format = format_detected;
    
    return 0;
}

void rs485_parse_frame(uint8_t *frame, uint16_t frame_len)
{
    rs485_packet_t pkt;
    uint8_t device_addr;  /* Device address to use for indexing (source for responses) */

    if (rs485_parse_packet(frame, frame_len, &pkt) != 0)
    {
        return;
    }

    /* 
     * For received frames, determine the device address:
     * - For responses from slaves: use src_addr (the slave that responded)
     * - For requests from master (unlikely): use dst_addr (the target slave)
     * - Broadcast messages: use src_addr to track who sent it
     */
    if (pkt.src_addr >= RS485_SLAVE_ADDR_MIN && pkt.src_addr <= RS485_SLAVE_ADDR_MAX)
    {
        /* Response from a slave - use source address */
        device_addr = pkt.src_addr;
    }
    else if (pkt.dst_addr >= RS485_SLAVE_ADDR_MIN && pkt.dst_addr <= RS485_SLAVE_ADDR_MAX)
    {
        /* Request to a slave - use destination address */
        device_addr = pkt.dst_addr;
    }
    else
    {
        /* Invalid or broadcast - skip processing */
        return;
    }

    switch (pkt.function)
    {
    case DEVINFO_READ_REQ_ACK:
        usb_printf("rs485_parse_frame DEVINFO_READ_REQ_ACK src=%d dst=%d\r\n", pkt.src_addr, pkt.dst_addr);
        if (pkt.data_len == sizeof(SubDevicelnfo))
        {
            const SubDevicelnfo *info = (const SubDevicelnfo *)pkt.data;
            if (rs485_subdev_payload_is_valid(device_addr, info))
            {
                memcpy(&g_SubDevicelnfo[device_addr - 1U], info, sizeof(SubDevicelnfo));
                g_subdev_valid[device_addr - 1U] = 1U;
                g_subdev_last_tick[device_addr - 1U] = HAL_GetTick();
                
                /* Print sub-device info */
                usb_printf("  [SubDev Info] Addr=%d\r\n", device_addr);
                usb_printf("    - SerialNumber: %s\r\n", (char *)info->SerialNumber);
                usb_printf("    - DeviceName: %s\r\n", (char *)info->DeviceName);
                usb_printf("    - DeviceType: %d\r\n", info->DeviceType);
                usb_printf("    - SlotId: %d\r\n", info->SlotId);
                usb_printf("    - Version: %s\r\n", (char *)info->Version);
                usb_printf("    - Sensitivity: %d\r\n", info->Sensitivity);
                usb_printf("  [SubDev Info End]\r\n");
            }
            else
            {
                usb_printf("  [SubDev Info] Invalid payload for addr=%d\r\n", device_addr);
            }
        }
        else
        {
            usb_printf("  [SubDev Info] Data length mismatch: got %d, expected %d\r\n", 
                       pkt.data_len, sizeof(SubDevicelnfo));
        }
        break;
    case DEVINFO_WRITE_REQ_ACK:
        usb_printf("rs485_parse_frame DEVINFO_WRITE_REQ_ACK src=%d dst=%d\n", pkt.src_addr, pkt.dst_addr);
        if (rs485_is_slave_address(device_addr))
        {
            /* data_len=0: legacy mode, no status; data_len=1: with status byte (0=success) */
            if (pkt.data_len == 1U)
            {
                g_subdev_write_ack[device_addr - 1U] = (int8_t)(pkt.data[0]);
            }
            else
            {
                g_subdev_write_ack[device_addr - 1U] = 0; /* legacy: treat as success */
            }
        }
        break;
    case BRIDGE_SET_ACK:
        usb_printf("rs485_parse_frame BRIDGE_SET_ACK src=%d dst=%d, data_len=%d\n", pkt.src_addr, pkt.dst_addr, pkt.data_len);
        if (rs485_is_slave_address(device_addr))
        {
            if (pkt.data_len == 1U)
            {
                g_subdev_write_ack[device_addr - 1U] = (int8_t)(pkt.data[0]);
            }
            else
            {
                g_subdev_write_ack[device_addr - 1U] = 0;
            }
        }
        break;
    case GAIN_SET_ACK:
        usb_printf("rs485_parse_frame GAIN_SET_ACK src=%d dst=%d, data_len=%d\n", pkt.src_addr, pkt.dst_addr, pkt.data_len);
        if (rs485_is_slave_address(device_addr))
        {
            if (pkt.data_len == 1U)
            {
                g_subdev_write_ack[device_addr - 1U] = (int8_t)(pkt.data[0]);
            }
            else
            {
                g_subdev_write_ack[device_addr - 1U] = 0;
            }
        }
        break;
    case DAC_SET_ACK:
        usb_printf("rs485_parse_frame DAC_SET_ACK src=%d dst=%d, data_len=%d\n", pkt.src_addr, pkt.dst_addr, pkt.data_len);
        if (rs485_is_slave_address(device_addr))
        {
            if (pkt.data_len == 1U)
            {
                g_subdev_write_ack[device_addr - 1U] = (int8_t)(pkt.data[0]);
            }
            else
            {
                g_subdev_write_ack[device_addr - 1U] = 0;
            }
        }
        break;
    default:
        break;
    }
}

int8_t rs485_send_frame_ex(uint8_t src_addr, uint8_t dst_addr, uint8_t cmd, const uint8_t *data, uint16_t len, rs485_proto_format_t format)
{
    rs485_packet_t pkt;
    uint16_t frame_len = 0U;
    uint8_t frame[RS485_RX_BUF_LEN];

    if ((len > 0U) && (data == NULL))
    {
        return -1;
    }

    /* Validate addresses (except for broadcast) */
    if (!rs485_is_valid_address(src_addr) || 
        (dst_addr != RS485_BROADCAST_ADDR && !rs485_is_valid_address(dst_addr)))
    {
        return -1;
    }

    pkt.src_addr = src_addr;
    pkt.dst_addr = dst_addr;
    pkt.function = cmd;
    pkt.data_len = len;
    pkt.data = data;
    pkt.proto_format = format;

    if (rs485_build_frame_ex(&pkt, frame, sizeof(frame), &frame_len, format) != 0)
    {
        return -1;
    }

    return rs485_send_raw(frame, frame_len);
}

int8_t rs485_send_frame(uint8_t dev_num, uint8_t cmd, const uint8_t *data, uint16_t len)
{
    /* Legacy interface: always use new format with master as source */
    return rs485_send_frame_ex(RS485_MASTER_ADDR, dev_num, cmd, data, len, RS485_PROTO_FMT_NEW);
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

    usb_printf("\r\n===== RS485 Subdev Config Test Start (New Protocol Format) =====\r\n");
    usb_printf("Protocol Format: [Header] [SrcAddr] [DstAddr] [Func] [Len] [Data] [XOR] [Tail]\r\n");

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
            /* Use new format: src=MASTER(0x00), dst=target device */
            ret = rs485_send_frame_ex(RS485_MASTER_ADDR, addr, DAC_SET, 
                                      (const uint8_t *)&dac_cfg, sizeof(dac_set_payload_t), RS485_PROTO_FMT_NEW);
            usb_printf("DAC_SET to addr=%d (src=0x%02X, dst=0x%02X), bitflag=0x%02X, voltages={%.2f,%.2f,%.2f}, ret=%d\r\n",
                       addr, RS485_MASTER_ADDR, addr, dac_cfg.bitflag, 
                       dac_cfg.voltage0, dac_cfg.voltage1, dac_cfg.voltage2, ret);

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
                .exc_en = 1,       // 使能激励
                .bridge = 0x00,    // bit0-2=0, 全桥
                .bridgeShunt = 0x07 // bit0-2=1, 3个通道都接入分流电阻
            };
            ret = rs485_send_frame_ex(RS485_MASTER_ADDR, addr, BRIDGE_SET, 
                                      (const uint8_t *)&bridge_cfg, sizeof(bridge_set_payload_t), RS485_PROTO_FMT_NEW);
            usb_printf("BRIDGE_SET to addr=%d (src=0x%02X, dst=0x%02X), exc_en=%d, bridge=0x%02X, bridgeShunt=0x%02X, ret=%d\r\n",
                       addr, RS485_MASTER_ADDR, addr, bridge_cfg.exc_en, bridge_cfg.bridge, bridge_cfg.bridgeShunt, ret);

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
                .gain = 0x07,   // bit0-2=1, 3个通道都是10倍
                .pga = 0x01C7 // bit6-8=1, bit3-5=1, bit0-2=1 (全部128倍)
            };
            ret = rs485_send_frame_ex(RS485_MASTER_ADDR, addr, GAIN_SET, 
                                      (const uint8_t *)&gain_cfg, sizeof(gain_set_payload_t), RS485_PROTO_FMT_NEW);
            usb_printf("GAIN_SET to addr=%d (src=0x%02X, dst=0x%02X), gain=0x%02X, pga=0x%04X, ret=%d\r\n",
                       addr, RS485_MASTER_ADDR, addr, gain_cfg.gain, gain_cfg.pga, ret);

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
