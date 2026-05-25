#include "bridge_config.h"
#include "rs485_processor.h"
#include <string.h>
#include "./BSP/RS485/rs485.h"

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
        case 1: /* 1/4桥 */
        case 2: /* 1/4桥 */
            return 2; /* 1/4桥 */

        case 3: /* 1/2桥 */
        case 4: /* 1/2桥 */
        case 9: /* 1/2桥 */
            return 1; /* 半桥 */

        case 5: /* 全桥 */
        case 6: /* 全桥 */
        case 7: /* 全桥 */
            return 0; /* 全桥 */

        case 8: /* 配置错误 */
        default:
            return 2; /* 默认1/4桥 */
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
    uint8_t bit_mask;

    if (subdev_addr < RS485_SLAVE_ADDR_MIN || subdev_addr > RS485_SLAVE_ADDR_MAX || cfg == NULL)
    {
        return -1;
    }

    /* 检查子设备是否有效 */
    if (!rs485_subdev_is_valid(subdev_addr))
    {
        return -1;
    }

    /* ---------- 1. 配置DAC (DAC参数暂时没有，预留) ---------- */
    dac_set_payload_t dac_cfg = {0};
    dac_cfg.bitflag = 0x07; /* 设置三个通道 */
    for (i = 0; i < BRIDGE_CHANNELS_PER_SUBDEV; i++)
    {
        switch (i)
        {
            case 0: dac_cfg.voltage0 = cfg->voltage[i]; break;
            case 1: dac_cfg.voltage1 = cfg->voltage[i]; break;
            case 2: dac_cfg.voltage2 = cfg->voltage[i]; break;
            default: break;
        }
    }

    rs485_subdev_clear_write_ack(subdev_addr);
    ret = rs485_subdev_set_dac(subdev_addr, &dac_cfg);
    if (ret != 0)
    {
        return -1; /* 发送失败 */
    }

    ret = bridge_wait_ack(subdev_addr, timeout_ms);
    if (ret != 0)
    {
        return -2; /* 超时或设备错误 */
    }

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

    /* 构建bridge字段 (每个通道2bits，共6bits用于3个通道)
     * bit0-1: 通道0桥路类型
     * bit2-3: 通道1桥路类型
     * bit4-5: 通道2桥路类型
     * 0=全桥, 1=半桥, 2=1/4桥
     */
    bridge_cfg.bridge = 0;
    for (i = 0; i < BRIDGE_CHANNELS_PER_SUBDEV; i++)
    {
        local_ch = bridge_get_local_ch_idx(i);
        if (local_ch >= 0)
        {
            /* 根据通道配置设置对应的bits */
            bit_mask = (cfg->bridge_type[i] & 0x03) << (local_ch * 2);
            bridge_cfg.bridge |= bit_mask;
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

    rs485_subdev_clear_write_ack(subdev_addr);
    ret = rs485_subdev_set_bridge(subdev_addr, &bridge_cfg);
    if (ret != 0)
    {
        return -1;
    }

    ret = bridge_wait_ack(subdev_addr, timeout_ms);
    if (ret != 0)
    {
        return -2;
    }

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
                case 1: pga_code = 0; break;   /* 1倍 */
                case 2: pga_code = 1; break;   /* 2倍 */
                case 10: pga_code = 2; break;  /* 10倍 */
                case 20: pga_code = 4; break;  /* 20倍 */
                case 128: pga_code = 7; break; /* 128倍 */
                case 1280: pga_code = 7; break; /* 1280倍(实际是128*10) */
                default: pga_code = 0; break;
            }
            gain_cfg.pga |= ((uint16_t)pga_code << (local_ch * 3));
        }
    }

    rs485_subdev_clear_write_ack(subdev_addr);
    ret = rs485_subdev_set_gain(subdev_addr, &gain_cfg);
    if (ret != 0)
    {
        return -1;
    }

    ret = bridge_wait_ack(subdev_addr, timeout_ms);
    if (ret != 0)
    {
        return -2;
    }

    /* ---------- 4. 配置PWM频率 ---------- */
    pwm_set_payload_t pwm_cfg = {0};
    pwm_cfg.pwm_freq = cfg->pwm_freq; /* 来自USB_CollectChCfg_Reply的sample_rate */

    rs485_subdev_clear_write_ack(subdev_addr);
    ret = rs485_subdev_set_pwm(subdev_addr, &pwm_cfg);
    if (ret != 0)
    {
        return -1;
    }

    ret = bridge_wait_ack(subdev_addr, timeout_ms);
    if (ret != 0)
    {
        return -2;
    }

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
        return -1;
    }

    /* 检查该子设备是否为桥路类型 */
    if (!bridge_is_bridge_subdev(subdev_idx))
    {
        return -1; /* 不是桥路子设备，无需配置 */
    }

    /* 初始化输出配置为默认值 */
    memset(out_cfg, 0, sizeof(bridge_subdev_cfg_t));

    /* 遍历所有通道，找到属于该子设备的通道 */
    for (i = 0; i < total_channel_num; i++)
    {
        ch_subdev_idx = bridge_get_subdev_idx_from_channel(channel_table_elem[i].nChannelID);
        if (ch_subdev_idx != subdev_idx)
        {
            continue; /* 不是该子设备的通道 */
        }

        local_ch_idx = bridge_get_local_ch_idx(channel_table_elem[i].nChannelID);
        if (local_ch_idx < 0 || local_ch_idx >= BRIDGE_CHANNELS_PER_SUBDEV)
        {
            continue;
        }

        /* 提取分组类型 */
        out_cfg->nGroupID[local_ch_idx] = channel_table_elem[i].nGroupID;

        /* 提取nVar1中的参数 */
        nVar1 = (uint32_t)channel_table_elem[i].nVar1;
        bridge_type = (uint16_t)(nVar1 & 0xFFFF);       /* bit15-0 */
        shunt_type = (uint16_t)((nVar1 >> 16) & 0xFFFF); /* bit31-16 */

        /* 映射桥路类型 */
        out_cfg->bridge_type[local_ch_idx] = bridge_type_map((uint8_t)bridge_type);

        /* 映射分流校准类型 (只允许R2) */
        out_cfg->shunt_calib[local_ch_idx] = bridge_shunt_type_map(shunt_type);

        /* 验证分流器电阻 (只允许40K) */
        if (channel_table_elem[i].fVar7 > 0)
        {
            if (bridge_validate_shunt_r(channel_table_elem[i].fVar7) != 0)
            {
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

        /* 设置DAC电压 (默认2.5V，根据PGA调整) */
        out_cfg->voltage[local_ch_idx] = 2.5f;

        found_bridge_ch = 1;
    }

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
        return -1;
    }

    /* 依次配置每个子设备 */
    for (subdev_idx = 0; subdev_idx < SUBDEV_NUM_MAX; subdev_idx++)
    {
        /* 检查是否为桥路子设备 */
        if (!bridge_is_bridge_subdev(subdev_idx))
        {
            continue; /* 不是桥路子设备，跳过 */
        }

        /* 提取该子设备的配置 */
        ret = bridge_extract_subdev_cfg(channel_table_elem, total_channel_num, subdev_idx, &cfg);
        if (ret != 0)
        {
            /* 该子设备没有桥路通道配置，跳过 */
            continue;
        }

        /* 设置PWM频率 (来自USB_CollectChCfg_Reply中的sample_rate) */
        cfg.pwm_freq = pwm_freq;

        /* 配置该子设备 */
        ret = bridge_config_subdev(subdev_idx + 1, &cfg, 100); /* 地址从1开始，超时100ms */
        if (ret != 0)
        {
            /* 配置失败，返回错误 */
            return -1;
        }
    }

    return 0; /* 全部配置成功 */
}
