# 板卡ID和采集通道对应关系分析

## 一、硬件架构

```
┌──────────────────────────────────────────────────────────────┐
│                     STM32H7 主控制器                         │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐         │
│  │    SPI0      │  │    SPI1      │  │    SPI2      │         │
│  │  (SPI_NUM=3) │  │              │  │              │         │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘         │
│         │                 │                 │                │
│       菊花链            菊花链            菊花链              │
│      8个ADC            8个ADC            8个ADC               │
│  (SPI_CH_NUM=8)  (SPI_CH_NUM=8)  (SPI_CH_NUM=8)             │
└──────────────────────────────────────────────────────────────┘
```

### 关键宏定义（collector_processor.h）

| 宏定义 | 值 | 说明 |
|--------|-----|------|
| `SPI_NUM` | 3 | SPI总线数量 |
| `SPI_CH_NUM` | 8 | 单条SPI总线上ADC通道数（菊花链） |
| `ADC_CH_TOTAL` | 24 | 总逻辑通道数 = 3 × 8 = 24 |

---

## 二、通道映射关系

### 2.1 逻辑通道ID计算公式

```c
// 物理到逻辑通道 (collector_processor.c:52)
逻辑通道ID = spi_idx × SPI_CH_NUM + adc_pos

// 逻辑到物理通道 (usb_processor.c:888-889)
spi_idx = ch_id % SPI_NUM;    // SPI索引 (0-2)
adc_pos = ch_id / SPI_NUM;    // ADC位置 (0-7)
```

### 2.2 完整映射表

| 逻辑通道ID | SPI索引 | ADC位置 | 子设备索引 | 子设备内局部通道 |
|------------|---------|---------|------------|------------------|
| 0 | 0 | 0 | 0 | 0 |
| 1 | 1 | 0 | 0 | 1 |
| 2 | 2 | 0 | 0 | 2 |
| 3 | 0 | 1 | 1 | 0 |
| 4 | 1 | 1 | 1 | 1 |
| 5 | 2 | 1 | 1 | 2 |
| 6 | 0 | 2 | 2 | 0 |
| 7 | 1 | 2 | 2 | 1 |
| 8 | 2 | 2 | 2 | 2 |
| 9 | 0 | 3 | 3 | 0 |
| 10 | 1 | 3 | 3 | 1 |
| 11 | 2 | 3 | 3 | 2 |
| 12 | 0 | 4 | 4 | 0 |
| 13 | 1 | 4 | 4 | 1 |
| 14 | 2 | 4 | 4 | 2 |
| 15 | 0 | 5 | 5 | 0 |
| 16 | 1 | 5 | 5 | 1 |
| 17 | 2 | 5 | 5 | 2 |
| 18 | 0 | 6 | 6 | 0 |
| 19 | 1 | 6 | 6 | 1 |
| 20 | 2 | 6 | 6 | 2 |
| 21 | 0 | 7 | 7 | 0 |
| 22 | 1 | 7 | 7 | 1 |
| 23 | 2 | 7 | 7 | 2 |

### 2.3 子设备与通道对应关系（bridge_config.c）

```c
// 子设备1对应通道0-2，子设备2对应通道3-5，依此类推
// 共8个子设备，每个子设备管理3个通道
```

| 子设备索引 | 管理的逻辑通道 | 子设备内局部通道 |
|------------|----------------|------------------|
| 0 | 0, 1, 2 | 0, 1, 2 |
| 1 | 3, 4, 5 | 0, 1, 2 |
| 2 | 6, 7, 8 | 0, 1, 2 |
| 3 | 9, 10, 11 | 0, 1, 2 |
| 4 | 12, 13, 14 | 0, 1, 2 |
| 5 | 15, 16, 17 | 0, 1, 2 |
| 6 | 18, 19, 20 | 0, 1, 2 |
| 7 | 21, 22, 23 | 0, 1, 2 |

---

## 三、USB_CollectChCfg_Reply 配置流程

**函数位置**：`usb_processor.c:815`

### 3.1 数据包结构

#### 通道表头（ChannelTableHeader）

```c
typedef struct {
    uint32_t nTotalChannelNum;      // 通道总数
    float    fHardwareSampleRate;     // 硬件采样率
    uint32_t nFlagIntDiff;           // 标志位
} ChannelTableHeader;
```

#### 通道配置元素（ChannelTableElem）

```c
typedef struct {
    int32_t  nChannelID;      // 通道ID (0-23)
    int32_t  nVar1;           // 桥路类型(bit15-0) + 分流校准(bit31-16)
    int32_t  nInputRange;     // 输入量程(0-5)
    int32_t  nGroupID;        // 组ID
    float    fVar7;           // 分流电阻值
    // ... 其他字段
} ChannelTableElem;
```

### 3.2 配置步骤详解

#### 步骤1：解析通道表头

```c
const ChannelTableHeader *channelTableHeader = 
    (const ChannelTableHeader *)(data_in + userDataLoc);

// 获取关键参数:
// - nTotalChannelNum: 通道总数
// - fHardwareSampleRate: 硬件采样率
// - nFlagIntDiff: 标志位
```

#### 步骤2：解析通道配置数组

```c
const ChannelTableElem *channelTableElem = 
    (const ChannelTableElem *)(data_in + userDataLoc);

// 遍历获取每个通道的配置:
// - nChannelID: 通道ID
// - nVar1: 桥路类型和分流校准
// - nInputRange: 输入量程
```

#### 步骤3：通道使能映射（核心代码）

```c
g_ch_enable_mask = 0;           // 通道使能掩码（24位，每位对应一个通道）
g_enabled_ch_cnt = 0;           // 使能通道计数
memset(g_spi_adc_cnt, 0, sizeof(g_spi_adc_cnt));  // 每SPI的ADC数量

for (size_t i = 0; i < channelTableHeader->nTotalChannelNum; i++)
{
    int32_t ch_id = channelTableElem[i].nChannelID;
    
    // 如果上位机通道ID从1开始，转换为从0开始
    if (channelTableElem[0].nChannelID == 1)
        ch_id -= 1;
    
    if (ch_id >= 0 && ch_id < ADC_CH_TOTAL)
    {
        // 设置通道使能掩码（bit-i 表示通道i使能）
        g_ch_enable_mask |= (1u << ch_id);
        g_enabled_chs[g_enabled_ch_cnt++] = (uint8_t)ch_id;
        
        // 计算SPI索引和ADC位置
        uint8_t spi_idx = ch_id % SPI_NUM;    // 0, 1, 2
        uint8_t adc_pos = ch_id / SPI_NUM;    // 0-7
        
        // 更新该SPI需要读取的ADC数量
        if ((adc_pos + 1) > g_spi_adc_cnt[spi_idx])
            g_spi_adc_cnt[spi_idx] = adc_pos + 1;
    }
}
```

#### 步骤4：采样率配置

```c
// 从支持的采样率列表中查找匹配的采样率
for (uint8_t i = 0; i < g_ida_ch_rate_count; i++)
{
    if (channelTableHeader->fHardwareSampleRate == g_ida_ch_rate[i].ch_cfg_value)
    {
        sample_rate = g_ida_ch_rate[i].ch_cfg_value;
        break;
    }
}

// 验证采样率是否支持
sample_rate = AdcCollectorMatchSampleRate(sample_rate, g_enabled_ch_cnt);

// 选择采集模式：DMA模式或Polling模式
mode = AdcCollectorSelectMode(sample_rate);
// - ADC_COLLECT_MODE_DMA: 采样率 ≤ 25600Hz
// - ADC_COLLECT_MODE_POLLING: 采样率 > 25600Hz

// 配置ADC采样率
CfgAdcSampleRate(sample_rate);
```

#### 步骤5：桥路子设备配置

```c
int8_t bridge_ret = bridge_config_all_subdevs(
    channelTableElem,              // 通道配置表
    channelTableHeader->nTotalChannelNum,  // 通道总数
    sample_rate                    // 采样率（用于PWM配置）
);
```

配置内容包括：
1. **DAC配置**：设置激励电压
2. **桥路配置**：设置桥路类型（全桥/半桥/1/4桥）和分流校准
3. **增益配置**：设置PGA增益
4. **PWM配置**：设置激励频率

---

## 四、通道配置参数映射

### 4.1 桥路类型（nVar1 bit15-0）

| nVar1值 | 桥路类型 | bridge字段值 | 说明 |
|---------|----------|--------------|------|
| 1, 2 | 1/4桥 | 2 | 四分之一桥 |
| 3, 4, 9 | 1/2桥 | 1 | 半桥 |
| 5, 6, 7 | 全桥 | 0 | 全桥 |
| 8 | 配置错误 | 2（默认） | 按1/4桥处理 |

**bridge字段编码**（每个通道2bits）：
- bit0-1: 通道0桥路类型
- bit2-3: 通道1桥路类型
- bit4-5: 通道2桥路类型

### 4.2 分流校准（nVar1 bit31-16）

| nVar1值 | 分流校准类型 | bridgeShunt值 | 说明 |
|---------|--------------|---------------|------|
| 0 | 不接入 | 0 | 不接分流电阻 |
| 2 | 接入R2 | 1 | 接入R2分流电阻 |
| 其他 | 无效 | 0 | 视为不接入 |

**bridgeShunt字段编码**（每个通道1bit）：
- bit0: 通道0分流电阻
- bit1: 通道1分流电阻
- bit2: 通道2分流电阻

### 4.3 量程增益（nInputRange）

| nInputRange | gain | pga | 量程电压 | 适用场景 |
|-------------|------|-----|----------|----------|
| 0 | 0 (1倍) | 1 | 2.5V | 大信号 |
| 1 | 0 (1倍) | 2 | 1.25V | 较大信号 |
| 2 | 1 (10倍) | 10 | 0.25V | 小信号 |
| 3 | 1 (10倍) | 20 | 0.125V | 微弱信号 |
| 4 | 1 (10倍) | 128 | 0.01953125V | 极微弱信号 |
| 5 | 1 (10倍) | 1280 | 0.001953125V | 超微弱信号 |

---

## 五、数据流向图

```
上位机配置数据
     │
     ▼
┌─────────────────────────────────────┐
│  USB_CollectChCfg_Reply()           │
│  - 解析ChannelTableHeader           │
│  - 解析ChannelTableElem[]           │
└─────────────────────────────────────┘
     │
     ▼
┌─────────────────────────────────────┐
│  通道使能映射                        │
│  - 计算g_ch_enable_mask               │
│  - 计算g_spi_adc_cnt[3]             │
└─────────────────────────────────────┘
     │
     ▼
┌─────────────────────────────────────┐
│  采样率配置                          │
│  - 匹配采样率                        │
│  - 选择DMA/Polling模式               │
│  - 配置定时器                       │
└─────────────────────────────────────┘
     │
     ▼
┌─────────────────────────────────────┐
│  bridge_config_all_subdevs()        │
│  - 配置DAC电压                      │
│  - 配置桥路类型                     │
│  - 配置增益/PGA                     │
│  - 配置PWM频率                      │
└─────────────────────────────────────┘
     │
     ▼
RS485命令发送到子设备
```

---

## 六、关键全局变量

| 变量名 | 类型 | 说明 |
|--------|------|------|
| `g_ch_enable_mask` | uint32_t | 通道使能掩码，bit-i表示通道i使能 |
| `g_enabled_chs[]` | uint8_t[] | 使能的通道号列表（按上位机顺序） |
| `g_enabled_ch_cnt` | uint8_t | 使能通道总数 |
| `g_spi_adc_cnt[]` | uint8_t[3] | 每SPI需要读取的ADC数量 |
| `g_cb_ch[]` | CircularBuffer*[24] | 24个通道的环形缓冲区指针 |

---

## 七、总结

### 核心映射关系

1. **物理层**：3条SPI × 每条8个ADC = 24个物理ADC位置
2. **逻辑层**：24个逻辑通道ID（0-23）
3. **子设备层**：8个子设备，每个子设备管理3个通道
4. **映射公式**：`ch_id = spi_idx × 8 + adc_pos`

### 配置流程

1. 上位机发送 `ChannelTableHeader` + `ChannelTableElem[]`
2. 主控解析通道ID和配置参数
3. 计算SPI/ADC位置映射
4. 设置通道使能掩码和SPI读取数量
5. 配置采样率和采集模式
6. 通过RS485向子设备发送桥路、增益、PWM等配置

### 注意事项

- 通道ID可能从0或1开始，代码中会自动转换
- 采样率超过25600Hz时自动切换到Polling模式
- 每个SPI总线独立计算需要读取的ADC数量（adc_pos + 1）
- 桥路配置失败不会阻塞采集启动，仅记录日志
