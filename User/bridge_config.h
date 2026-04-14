#ifndef __BRIDGE_CONFIG_H
#define __BRIDGE_CONFIG_H

#include <stdint.h>
#include "rs485_processor.h"
#include "dataType.h"

/* 桥路类型定义 (对应 nVar1 bit15-0)
 * 映射规则:
 *   1-2  -> 1/4桥
 *   3-4,9-> 1/2桥
 *   5-7  -> 全桥
 *   8    -> 配置错误 (默认按1/4桥处理)
 */
#define BRIDGE_TYPE_1_4_CH1     1   /* 1/4桥 */
#define BRIDGE_TYPE_1_4_CH2     2   /* 1/4桥 */
#define BRIDGE_TYPE_1_2_CH1     3   /* 1/2桥 */
#define BRIDGE_TYPE_1_2_CH2     4   /* 1/2桥 */
#define BRIDGE_TYPE_FULL_1      5   /* 全桥 */
#define BRIDGE_TYPE_FULL_2      6   /* 全桥 */
#define BRIDGE_TYPE_FULL_3      7   /* 全桥 */
#define BRIDGE_TYPE_ERR         8   /* 配置错误 */
#define BRIDGE_TYPE_1_2_CH3     9   /* 1/2桥 */

/* 分流校准类型定义 (对应 nVar1 bit31-16) */
#define SHUNT_CALIB_NONE    0   /* 不接入分流电阻 */
#define SHUNT_CALIB_R1      1   /* 接入R1 */
#define SHUNT_CALIB_R2      2   /* 接入R2 */

/* 有效通道数 */
#define BRIDGE_CHANNELS_PER_SUBDEV  3   /* 每个子设备有3个通道 */

/* 桥路子设备类型 */
#define MINI_SLICE_BRIDGE_TYPE      Mini_SliceBridge

/**
 * @brief 桥路配置参数结构体
 * @note 用于暂存一个子设备三个通道的桥路配置参数
 */
typedef struct
{
    uint8_t  bridge_type[BRIDGE_CHANNELS_PER_SUBDEV];   /* 每个通道的桥路类型 */
    uint8_t  shunt_calib[BRIDGE_CHANNELS_PER_SUBDEV];   /* 每个通道的分流校准类型 */
    uint16_t pga[BRIDGE_CHANNELS_PER_SUBDEV];           /* 每个通道的PGA增益值 */
    uint8_t  gain[BRIDGE_CHANNELS_PER_SUBDEV];          /* 每个通道的增益值 (0=1倍, 1=10倍) */
    float    voltage[BRIDGE_CHANNELS_PER_SUBDEV];       /* 每个通道的DAC电压值 */
} bridge_subdev_cfg_t;

/**
 * @brief 从通道配置提取一个子设备的桥路配置参数
 * @param channel_table_elem 通道配置表数组指针
 * @param total_channel_num  总通道数
 * @param subdev_idx         子设备索引 (0-7)
 * @param out_cfg            输出配置参数结构体
 * @return 0=成功提取到配置, -1=该子设备无桥路通道
 * @note 子设备1对应通道0-2，子设备2对应通道3-5，依此类推
 */
int8_t bridge_extract_subdev_cfg(const ChannelTableElem *channel_table_elem,
                                  uint32_t total_channel_num,
                                  uint8_t subdev_idx,
                                  bridge_subdev_cfg_t *out_cfg);

/**
 * @brief 桥路类型映射: nVar1(1-8) -> bridge_set_payload_t.bridge值
 * @param nvar1_bridge_type nVar1中的桥路类型索引 (bit15-0, 1-8)
 * @return 对应通道的桥路类型值 (0=全桥, 1=半桥, 2=1/4桥)
 */
uint8_t bridge_type_map(uint8_t nvar1_bridge_type);

/**
 * @brief 分流校准类型映射: nVar1(bit31-16, 0-10) -> bridgeShunt
 * @param nvar1_shunt_type nVar1中的分流校准类型索引 (bit31-16, 0-10)
 * @return 分流电阻接入状态 (0=不接, 1=接入R1, 2=接入R2)
 * @note 只允许设置R2(值为2)，其他值返回0
 */
uint8_t bridge_shunt_type_map(uint16_t nvar1_shunt_type);

/**
 * @brief 验证分流器电阻值
 * @param fShuntR 分流器电阻值
 * @return 0=有效(40K), -1=无效
 */
int8_t bridge_validate_shunt_r(float fShuntR);

/**
 * @brief 量程增益映射: nInputRange -> gain/pga
 * @param nInputRange 输入量程 (0-5)
 * @param out_gain    输出gain值 (0=1倍, 1=10倍)
 * @param out_pga     输出PGA值 (1, 2, 10, 20, 128, 1280)
 * @return 0=成功, -1=无效量程
 * @note nInputRange映射:
 *       0 -> gain=0(1倍), pga=1 (2.5V)
 *       1 -> gain=0(1倍), pga=2 (1.25V)
 *       2 -> gain=1(10倍), pga=10 (0.25V)
 *       3 -> gain=1(10倍), pga=20 (0.125V)
 *       4 -> gain=1(10倍), pga=128 (0.01953125V)
 *       5 -> gain=1(10倍), pga=1280 (0.001953125V)
 */
int8_t bridge_gain_pga_map(int32_t nInputRange, uint8_t *out_gain, uint16_t *out_pga);

/**
 * @brief 配置单个子设备的桥路参数
 * @param subdev_addr 子设备地址 (1-8)
 * @param cfg         桥路配置参数
 * @param timeout_ms  等待ACK超时时间(ms)
 * @return 0=成功, -1=发送失败, -2=超时无响应, >0=设备返回错误码
 */
int8_t bridge_config_subdev(uint8_t subdev_addr,
                            const bridge_subdev_cfg_t *cfg,
                            uint32_t timeout_ms);

/**
 * @brief 配置所有桥路子设备
 * @param channel_table_elem 通道配置表数组指针
 * @param total_channel_num  总通道数
 * @return 0=全部成功, -1=配置失败
 * @note 依次配置每个子设备，一个失败即返回错误
 */
int8_t bridge_config_all_subdevs(const ChannelTableElem *channel_table_elem,
                                  uint32_t total_channel_num);

/**
 * @brief 等待子设备配置ACK
 * @param subdev_addr 子设备地址 (1-8)
 * @param timeout_ms  超时时间(ms)
 * @return 0=成功收到ACK, -1=超时
 */
int8_t bridge_wait_ack(uint8_t subdev_addr, uint32_t timeout_ms);

/**
 * @brief 获取通道所属的子设备索引
 * @param channel_id 通道ID (从0开始)
 * @return 子设备索引 (0-7), 如果无效返回-1
 * @note 子设备1对应通道0-2，子设备2对应通道3-5，依此类推
 */
int8_t bridge_get_subdev_idx_from_channel(int32_t channel_id);

/**
 * @brief 获取通道在子设备内的局部通道号
 * @param channel_id 通道ID (从0开始)
 * @return 局部通道号 (0-2), 如果无效返回-1
 */
int8_t bridge_get_local_ch_idx(int32_t channel_id);

/**
 * @brief 判断通道是否属于桥路子设备
 * @param channel_id 通道ID (从0开始)
 * @return 1=是桥路通道, 0=不是
 */
uint8_t bridge_is_bridge_channel(int32_t channel_id);

/**
 * @brief 判断子设备是否为桥路类型
 * @param subdev_idx 子设备索引 (0-7)
 * @return 1=是桥路子设备, 0=不是
 */
uint8_t bridge_is_bridge_subdev(uint8_t subdev_idx);

#endif /* __BRIDGE_CONFIG_H */
