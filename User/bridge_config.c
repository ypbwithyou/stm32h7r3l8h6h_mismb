#include "bridge_config.h"
#include "rs485_processor.h"
#include <string.h>
#include "./BSP/RS485/rs485.h"
#include "usbd_cdc_if.h"

/**
 * @brief 获取通道所属的子设备索引
 * @note 子设备1对应通道0-2，子设备2对应通道3-5，依此类推
 *       共8个子设备，24个通道 (0-23)
 */
int8_t bridge_get_subdev_idx_from_channel(int32_t channel_id)
{
    if (channel_id < 0 || channel_id >= (SUBDEV_NUM_MAX * BRIDGE_CHANNELS_PER_SUBDEV))
    {
        return -1;
    }
    return (int8_t)(channel_id / BRIDGE_CHANNELS_PER_SUBDEV);
}

/**
 * @brief 获取通道在子设备内的局部通道号
 */
int8_t bridge_get_local_ch_idx(int32_t channel_id)
{
    if (channel_id < 0 || channel_id >= (SUBDEV_NUM_MAX * BRIDGE_CHANNELS_PER_SUBDEV))
    {
        return -1;
    }
    return (int8_t)(channel_id % BRIDGE_CHANNELS_PER_SUBDEV);
}

/**
 * @brief 判断子设备是否为桥路类型
 */
uint8_t bridge_is_bridge_subdev(uint8_t subdev_idx)
{
    if (subdev_idx >= SUBDEV_NUM_MAX)
    {
        return 0;
    }

    /* 检查子设备信息是否有效且为桥路类型 */
    if (g_SubDevicelnfo[subdev_idx].DeviceType == MINI_SLICE_BRIDGE_TYPE)
    {
        return 1;
    }
    return 0;
}

/**
 * @brief 判断通道是否属于桥路子设备
 */
uint8_t bridge_is_bridge_channel(int32_t channel_id)
{
    int8_t subdev_idx = bridge_get_subdev_idx_from_channel(channel_id);
    if (subdev_idx < 0)
    {
        return 0;
    }
    return bridge_is_bridge_subdev((uint8_t)subdev_idx);
}

/**
 * @brief 桥路类型映射: nVar1(1-9) -> bridge_set_payload_t.bridge值
 * @note 1-2: 1/4桥, 3-4,9: 1/2桥, 5-7: 全桥, 8: 配置错误
 *       bridge_set_payload_t.bridge: 0=全桥, 1=半桥, 2=1/4桥
 */
uint8_t bridge_type_map(uint8_t nvar1_bridge_type)
{
    switch (nvar1_bridge_type)
    {
    case 1:       /* 1/4桥 */
    case 2:       /* 1/4桥 */
        return 1; /* 非全桥 */

    case 3:       /* 1/2桥 */
    case 4:       /* 1/2桥 */
    case 9:       /* 1/2桥 */
        return 1; /* 非全桥 */

    case 5:       /* 全桥 */
    case 6:       /* 全桥 */
    case 7:       /* 全桥 */
        return 0; /* 全桥 */

    case 8: /* 配置错误 */
    default:
        return 1; /* 默认非全桥 */
    }
}

/**
 * @brief 分流校准类型映射: nVar1(bit31-16, 0-10) -> bridgeShunt
 * @note 只允许设置R2(值为2)，其他值返回0
 */
uint8_t bridge_shunt_type_map(uint16_t nvar1_shunt_type)
{
    /* 只允许设置R2 */
    if (nvar1_shunt_type == SHUNT_CALIB_R2)
    {
        return 2; /* 接入R2 */
    }
    /* 其他情况(包括R1)都视为不接入 */
    return 0; /* 不接分流电阻 */
}

/**
 * @brief 验证分流器电阻值
 * @note 只允许设置40K
 */
int8_t bridge_validate_shunt_r(float fShuntR)
{
    /* 允许40K电阻，允许一定误差 */
    const float target_r = 40000.0f;
    const float tolerance = 1000.0f; /* 1K误差范围 */

    if (fShuntR >= (target_r - tolerance) && fShuntR <= (target_r + tolerance))
    {
        return 0; /* 有效 */
    }
    return -1; /* 无效 */
}

/**
 * @brief 量程增益映射: nInputRange -> gain/pga
 * @note nInputRange映射:
 *       0 -> gain=0(1倍), pga=1 (2.5V)
 *       1 -> gain=0(1倍), pga=2 (1.25V)
 *       2 -> gain=1(10倍), pga=10 (0.25V)
 *       3 -> gain=1(10倍), pga=20 (0.125V)
 *       4 -> gain=1(10倍), pga=128 (0.01953125V)
 *       5 -> gain=1(10倍), pga=1280 (0.001953125V)
 */
int8_t bridge_gain_pga_map(int32_t nInputRange, uint8_t *out_gain, uint16_t *out_pga)
{
    if (out_gain == NULL || out_pga == NULL)
    {
        return -1;
    }

    switch (nInputRange)
    {
    case 0:
        *out_gain = 0; /* 1倍 */
        *out_pga = 1;  /* 2.5V */
        break;
    case 1:
        *out_gain = 0; /* 1倍 */
        *out_pga = 2;  /* 1.25V */
        break;
    case 2:
        *out_gain = 1; /* 10倍 */
        *out_pga = 10; /* 0.25V */
        break;
    case 3:
        *out_gain = 1; /* 10倍 */
        *out_pga = 20; /* 0.125V */
        break;
    case 4:
        *out_gain = 0;  /* 10倍 */
        *out_pga = 128; /* 0.01953125V */
        break;
    case 5:
        *out_gain = 1;   /* 10倍 */
        *out_pga = 1280; /* 0.001953125V */
        break;
    default:
        /* 无效量程 */
        return -1;
    }
    return 0;
}

/**
 * @brief 等待子设备配置ACK
 */
int8_t bridge_wait_ack(uint8_t subdev_addr, uint32_t timeout_ms)
{
    uint32_t start_tick;
    int8_t ack_status;

    if (subdev_addr < RS485_SLAVE_ADDR_MIN || subdev_addr > RS485_SLAVE_ADDR_MAX)
    {
        return -1;
    }

    start_tick = HAL_GetTick();
    while ((HAL_GetTick() - start_tick) < timeout_ms)
    {
        rs485_processor_poll();
        ack_status = rs485_subdev_get_write_ack(subdev_addr);
        if (ack_status >= 0) /* -1=no response, >=0=received */
        {
            return (ack_status == 0) ? 0 : ack_status; /* 0=success, >0=error code */
        }
    }
    return -1; /* 超时 */
}

/**
 * @brief 配置单个子设备的桥路参数
 */
int8_t bridge_config_subdev(uint8_t subdev_addr,
                            const bridge_subdev_cfg_t *cfg,
                            uint32_t timeout_ms)
{
    int8_t ret;
    uint8_t i;
    uint8_t local_ch;

    if (subdev_addr < RS485_SLAVE_ADDR_MIN || subdev_addr > RS485_SLAVE_ADDR_MAX || cfg == NULL)
    {
        usb_printf("[Bridge]  config_subdev(addr=%u): invalid param\r\n", subdev_addr);
        return -1;
    }

    /* 检查子设备是否有效 */
    if (!rs485_subdev_is_valid(subdev_addr))
    {
        usb_printf("[Bridge]  config_subdev(addr=%u): subdev not valid\r\n", subdev_addr);
        return -1;
    }

    usb_printf("[Bridge]  config_subdev(addr=%u): ch0={bridge=%u,shunt=%u,gain=%u,pga=%u,voltage=%.2f,nGroup=%ld}, "
               "ch1={bridge=%u,shunt=%u,gain=%u,pga=%u,voltage=%.2f,nGroup=%ld}, "
               "ch2={bridge=%u,shunt=%u,gain=%u,pga=%u,voltage=%.2f,nGroup=%ld}, pwm_freq=%lu\r\n",
               subdev_addr,
               cfg->bridge_type[0], cfg->shunt_calib[0], cfg->gain[0], cfg->pga[0], cfg->voltage[0], (long)cfg->nGroupID[0],
               cfg->bridge_type[1], cfg->shunt_calib[1], cfg->gain[1], cfg->pga[1], cfg->voltage[1], (long)cfg->nGroupID[1],
               cfg->bridge_type[2], cfg->shunt_calib[2], cfg->gain[2], cfg->pga[2], cfg->voltage[2], (long)cfg->nGroupID[2],
               (unsigned long)cfg->pwm_freq);

    /* ---------- 1. 配置DAC (DAC参数暂时没有，预留) ---------- */
    dac_set_payload_t dac_cfg = {0};
    dac_cfg.bitflag = 0x07; /* 设置三个通道 */
    for (i = 0; i < BRIDGE_CHANNELS_PER_SUBDEV; i++)
    {
        switch (i)
        {
        case 0:
            dac_cfg.voltage0 = cfg->voltage[i];
            break;
        case 1:
            dac_cfg.voltage1 = cfg->voltage[i];
            break;
        case 2:
            dac_cfg.voltage2 = cfg->voltage[i];
            break;
        default:
            break;
        }
    }

    usb_printf("[Bridge]  config_subdev(addr=%u): setting DAC (v0=%.2f, v1=%.2f, v2=%.2f)\r\n",
               subdev_addr, dac_cfg.voltage0, dac_cfg.voltage1, dac_cfg.voltage2);
    rs485_subdev_clear_write_ack(subdev_addr);
    ret = rs485_subdev_set_dac(subdev_addr, &dac_cfg);
    if (ret != 0)
    {
        usb_printf("[Bridge]  config_subdev(addr=%u): DAC send failed\r\n", subdev_addr);
        return -1; /* 发送失败 */
    }

    ret = bridge_wait_ack(subdev_addr, timeout_ms);
    if (ret != 0)
    {
        usb_printf("[Bridge]  config_subdev(addr=%u): DAC ack timeout\r\n", subdev_addr);
        return -2; /* 超时或设备错误 */
    }
    usb_printf("[Bridge]  config_subdev(addr=%u): DAC OK\r\n", subdev_addr);

    /* ---------- 2. 配置桥路 ---------- */
    bridge_set_payload_t bridge_cfg = {0};
    bridge_cfg.exc_en = 0;
    for (i = 0; i < BRIDGE_CHANNELS_PER_SUBDEV; i++)
    {
        if (cfg->nGroupID[i] == GROUP_STRESS_STRAIN)
        {
            bridge_cfg.exc_en = 1;
            break;
        }
    }

    /* 构建bridge字段 (每个通道1bit，共3bits用于3个通道)
     * bit0: 通道0电压信号来源
     * bit1: 通道1电压信号来源
     * bit2: 通道2电压信号来源
     * 0=全桥, 1=半桥/1/4桥
     */
    bridge_cfg.bridge = 0;
    for (i = 0; i < BRIDGE_CHANNELS_PER_SUBDEV; i++)
    {
        local_ch = bridge_get_local_ch_idx(i);
        if (local_ch >= 0 && cfg->bridge_type[i] != 0)
        {
            /* 非全桥（半桥或1/4桥）时置1 */
            bridge_cfg.bridge |= (1 << local_ch);
        }
    }

    /* 构建bridgeShunt字段 (每个通道1bit，共3bits)
     * bit0: 通道0分流电阻
     * bit1: 通道1分流电阻
     * bit2: 通道2分流电阻
     * 0=不接, 1=接入
     */
    bridge_cfg.bridgeShunt = 0;
    for (i = 0; i < BRIDGE_CHANNELS_PER_SUBDEV; i++)
    {
        local_ch = bridge_get_local_ch_idx(i);
        if (local_ch >= 0 && cfg->shunt_calib[i] == 2) /* R2接入 */
        {
            bridge_cfg.bridgeShunt |= (1 << local_ch);
        }
    }

    usb_printf("[Bridge]  config_subdev(addr=%u): setting bridge (exc_en=%u, bridge=0x%02x, shunt=0x%02x)\r\n",
               subdev_addr, bridge_cfg.exc_en, bridge_cfg.bridge, bridge_cfg.bridgeShunt);
    rs485_subdev_clear_write_ack(subdev_addr);
    ret = rs485_subdev_set_bridge(subdev_addr, &bridge_cfg);
    if (ret != 0)
    {
        usb_printf("[Bridge]  config_subdev(addr=%u): bridge send failed\r\n", subdev_addr);
        return -1;
    }

    ret = bridge_wait_ack(subdev_addr, timeout_ms);
    if (ret != 0)
    {
        usb_printf("[Bridge]  config_subdev(addr=%u): bridge ack timeout\r\n", subdev_addr);
        return -2;
    }
    usb_printf("[Bridge]  config_subdev(addr=%u): bridge OK\r\n", subdev_addr);

    /* ---------- 3. 配置增益 ---------- */
    gain_set_payload_t gain_cfg = {0};

    /* 构建gain字段 (每个通道1bit，共3bits)
     * bit0: 通道0增益 (0=1倍, 1=10倍)
     * bit1: 通道1增益
     * bit2: 通道2增益
     */
    gain_cfg.gain = 0;
    for (i = 0; i < BRIDGE_CHANNELS_PER_SUBDEV; i++)
    {
        local_ch = bridge_get_local_ch_idx(i);
        if (local_ch >= 0 && cfg->gain[i] == 1)
        {
            gain_cfg.gain |= (1 << local_ch);
        }
    }

    /* 构建pga字段 (每个通道3bits，共9bits)
     * bit0-2: 通道0 PGA
     * bit3-5: 通道1 PGA
     * bit6-8: 通道2 PGA
     * 0=1倍, 1=2倍, 2=4倍, 4=8倍, 8=16倍, 16=32倍, 32=64倍, 64=128倍
     * 但实际使用十进制值: 1, 2, 10, 20, 128
     */
    gain_cfg.pga = 0;
    for (i = 0; i < BRIDGE_CHANNELS_PER_SUBDEV; i++)
    {
        local_ch = bridge_get_local_ch_idx(i);
        if (local_ch >= 0)
        {
            uint16_t pga_val = cfg->pga[i];
            /* 映射PGA值到对应的编码 */
            uint8_t pga_code = 0;
            switch (pga_val)
            {
            case 1:
                pga_code = 0;
                break; /* 1倍 */
            case 2:
                pga_code = 1;
                break; /* 2倍 */
            case 10:
                pga_code = 2;
                break; /* 10倍 */
            case 20:
                pga_code = 4;
                break; /* 20倍 */
            case 128:
                pga_code = 7;
                break; /* 128倍 */
            case 1280:
                pga_code = 7;
                break; /* 1280倍(实际是128*10) */
            default:
                pga_code = 0;
                break;
            }
            gain_cfg.pga |= ((uint16_t)pga_code << (local_ch * 3));
        }
    }

    usb_printf("[Bridge]  config_subdev(addr=%u): setting gain (gain=0x%02x, pga=0x%04x)\r\n",
               subdev_addr, gain_cfg.gain, gain_cfg.pga);
    rs485_subdev_clear_write_ack(subdev_addr);
    ret = rs485_subdev_set_gain(subdev_addr, &gain_cfg);
    if (ret != 0)
    {
        usb_printf("[Bridge]  config_subdev(addr=%u): gain send failed\r\n", subdev_addr);
        return -1;
    }

    ret = bridge_wait_ack(subdev_addr, timeout_ms);
    if (ret != 0)
    {
        usb_printf("[Bridge]  config_subdev(addr=%u): gain ack timeout\r\n", subdev_addr);
        return -2;
    }
    usb_printf("[Bridge]  config_subdev(addr=%u): gain OK\r\n", subdev_addr);

    /* ---------- 4. 配置PWM频率 ---------- */
    pwm_set_payload_t pwm_cfg = {0};
    pwm_cfg.pwm_freq = cfg->pwm_freq; /* 来自USB_CollectChCfg_Reply的sample_rate */

    usb_printf("[Bridge]  config_subdev(addr=%u): setting PWM (freq=%lu Hz)\r\n",
               subdev_addr, (unsigned long)pwm_cfg.pwm_freq);
    rs485_subdev_clear_write_ack(subdev_addr);
    ret = rs485_subdev_set_pwm(subdev_addr, &pwm_cfg);
    if (ret != 0)
    {
        usb_printf("[Bridge]  config_subdev(addr=%u): PWM send failed\r\n", subdev_addr);
        return -1;
    }

    ret = bridge_wait_ack(subdev_addr, timeout_ms);
    if (ret != 0)
    {
        usb_printf("[Bridge]  config_subdev(addr=%u): PWM ack timeout\r\n", subdev_addr);
        return -2;
    }
    usb_printf("[Bridge]  config_subdev(addr=%u): PWM OK\r\n", subdev_addr);

    usb_printf("[Bridge]  config_subdev(addr=%u): all done\r\n", subdev_addr);
    return 0; /* 全部配置成功 */
}

/**
 * @brief 从通道配置提取一个子设备的桥路配置参数
 */
int8_t bridge_extract_subdev_cfg(const ChannelTableElem *channel_table_elem,
                                 uint32_t total_channel_num,
                                 uint8_t subdev_idx,
                                 bridge_subdev_cfg_t *out_cfg)
{
    uint32_t i;
    int8_t ch_subdev_idx;
    int8_t local_ch_idx;
    int8_t found_bridge_ch = 0;
    uint32_t nVar1;
    uint16_t bridge_type;
    uint16_t shunt_type;

    if (channel_table_elem == NULL || out_cfg == NULL || subdev_idx >= SUBDEV_NUM_MAX)
    {
        usb_printf("[Bridge]  extract_subdev_cfg(subdev=%u): invalid param\r\n", subdev_idx);
        return -1;
    }

    /* 检查该子设备是否为桥路类型 */
    if (!bridge_is_bridge_subdev(subdev_idx))
    {
        usb_printf("[Bridge]  extract_subdev_cfg(subdev=%u): not bridge subdev\r\n", subdev_idx);
        return -1; /* 不是桥路子设备，无需配置 */
    }

    /* 初始化输出配置为默认值 */
    memset(out_cfg, 0, sizeof(bridge_subdev_cfg_t));

    usb_printf("[Bridge]  extract_subdev_cfg(subdev=%u): total_ch=%lu\r\n",
               subdev_idx, (unsigned long)total_channel_num);

    /* 遍历所有通道，找到属于该子设备的通道 */
    for (i = 0; i < total_channel_num; i++)
    {

        int32_t nChannelID = channel_table_elem[i].nChannelID - 1;
        usb_printf("[Bridge]  extract_subdev_cfg(subdev=%u):   ch loop i=%lu, nChannelID=%ld\r\n",
                   subdev_idx, (unsigned long)i, (long)nChannelID);
        ch_subdev_idx = bridge_get_subdev_idx_from_channel(nChannelID);
        if (ch_subdev_idx != subdev_idx)
        {
            continue; /* 不是该子设备的通道 */
        }

        local_ch_idx = bridge_get_local_ch_idx(nChannelID);
        if (local_ch_idx < 0 || local_ch_idx >= BRIDGE_CHANNELS_PER_SUBDEV)
        {
            continue;
        }

        usb_printf("[Bridge]  extract_subdev_cfg(subdev=%u): ch%u found, nChannelID=%ld, nGroupID=%ld, "
                   "nVar1=0x%08lx, nInputRange=%ld, fVar7=%.2f\r\n",
                   subdev_idx, local_ch_idx,
                   (long)nChannelID,
                   (long)channel_table_elem[i].nGroupID,
                   (unsigned long)channel_table_elem[i].nVar1,
                   (long)channel_table_elem[i].nInputRange,
                   channel_table_elem[i].fVar7);

        /* 提取分组类型 */
        out_cfg->nGroupID[local_ch_idx] = channel_table_elem[i].nGroupID;

        /* 提取nVar1中的参数 */
        nVar1 = (uint32_t)channel_table_elem[i].nVar1;
        bridge_type = (uint16_t)(nVar1 & 0xFFFF);        /* bit15-0 */
        shunt_type = (uint16_t)((nVar1 >> 16) & 0xFFFF); /* bit31-16 */

        /* 映射桥路类型 */
        out_cfg->bridge_type[local_ch_idx] = bridge_type_map((uint8_t)bridge_type);
        usb_printf("[Bridge]  extract_subdev_cfg(subdev=%u):   bridge_type_raw=%u -> %u\r\n",
                   subdev_idx, bridge_type, out_cfg->bridge_type[local_ch_idx]);

        /* 映射分流校准类型 (只允许R2) */
        out_cfg->shunt_calib[local_ch_idx] = bridge_shunt_type_map(shunt_type);
        usb_printf("[Bridge]  extract_subdev_cfg(subdev=%u):   shunt_raw=%u -> %u\r\n",
                   subdev_idx, shunt_type, out_cfg->shunt_calib[local_ch_idx]);

        /* 验证分流器电阻 (只允许40K) */
        if (channel_table_elem[i].fVar7 > 0)
        {
            if (bridge_validate_shunt_r(channel_table_elem[i].fVar7) != 0)
            {
                usb_printf("[Bridge]  extract_subdev_cfg(subdev=%u):   shunt R invalid (%.2f), disable\r\n",
                           subdev_idx, channel_table_elem[i].fVar7);
                /* 电阻值无效，设为不接入 */
                out_cfg->shunt_calib[local_ch_idx] = 0;
            }
        }

        /* 映射量程增益 */
        uint8_t gain;
        uint16_t pga;
        if (bridge_gain_pga_map(channel_table_elem[i].nInputRange, &gain, &pga) == 0)
        {
            out_cfg->gain[local_ch_idx] = gain;
            out_cfg->pga[local_ch_idx] = pga;
        }
        usb_printf("[Bridge]  extract_subdev_cfg(subdev=%u):   gain=%u, pga=%u\r\n",
                   subdev_idx, out_cfg->gain[local_ch_idx], out_cfg->pga[local_ch_idx]);

        /* 设置DAC电压 (默认2.5V，根据PGA调整) */
        out_cfg->voltage[local_ch_idx] = 2.5f;
        usb_printf("[Bridge]  extract_subdev_cfg(subdev=%u):   voltage[%u]=%.2f\r\n",
                   subdev_idx, local_ch_idx, out_cfg->voltage[local_ch_idx]);

        found_bridge_ch = 1;
    }

    usb_printf("[Bridge]  extract_subdev_cfg(subdev=%u): %s\r\n",
               subdev_idx, found_bridge_ch ? "found bridge ch" : "no bridge ch found");
    return found_bridge_ch ? 0 : -1;
}

/**
 * @brief 配置所有桥路子设备
 */
int8_t bridge_config_all_subdevs(const ChannelTableElem *channel_table_elem,
                                 uint32_t total_channel_num,
                                 uint32_t pwm_freq)
{
    uint8_t subdev_idx;
    int8_t ret;
    bridge_subdev_cfg_t cfg;

    if (channel_table_elem == NULL || total_channel_num == 0)
    {
        usb_printf("[Bridge] bridge_config_all_subdevs: invalid param (elem=%p, total=%lu)\r\n",
                   (void *)channel_table_elem, (unsigned long)total_channel_num);
        return -1;
    }

    usb_printf("[Bridge] bridge_config_all_subdevs: start, total_ch=%lu, pwm_freq=%lu Hz\r\n",
               (unsigned long)total_channel_num, (unsigned long)pwm_freq);

    /* 依次配置每个子设备 */
    for (subdev_idx = 0; subdev_idx < SUBDEV_NUM_MAX; subdev_idx++)
    {
        /* 检查是否为桥路子设备 */
        if (!bridge_is_bridge_subdev(subdev_idx))
        {
            usb_printf("[Bridge]  subdev[%u]: not bridge subdev, skip\r\n", subdev_idx);
            continue;
        }

        /* 提取该子设备的配置 */
        ret = bridge_extract_subdev_cfg(channel_table_elem, total_channel_num, subdev_idx, &cfg);
        if (ret != 0)
        {
            usb_printf("[Bridge]  subdev[%u]: no bridge channel cfg, skip\r\n", subdev_idx);
            continue;
        }

        /* 设置PWM频率 (来自USB_CollectChCfg_Reply中的sample_rate) */
        cfg.pwm_freq = pwm_freq;

        /* 配置该子设备 */
        usb_printf("[Bridge]  subdev[%u]: configuring, addr=%u, pwm_freq=%lu\r\n",
                   subdev_idx, subdev_idx + 1, (unsigned long)pwm_freq);
        ret = bridge_config_subdev(subdev_idx + 1, &cfg, 100); /* 地址从1开始，超时100ms */
        if (ret != 0)
        {
            usb_printf("[Bridge]  subdev[%u]: config FAILED\r\n", subdev_idx);
            return -1;
        }
        usb_printf("[Bridge]  subdev[%u]: config OK\r\n", subdev_idx);
    }

    usb_printf("[Bridge] bridge_config_all_subdevs: all done\r\n");
    return 0; /* 全部配置成功 */
}
