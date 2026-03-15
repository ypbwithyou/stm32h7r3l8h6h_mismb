# USB配置到数据显示完整流程分析

## 流程概述

整个数据流包含以下四个主要阶段：
1. **USB_CollectChCfg_Reply** - 通道配置处理
2. **定时器中断** - 触发DMA采集
3. **DMA传输完成回调** - 处理采集数据
4. **USB_Display_Reply** - 发送数据到PC

## 详细流程分析

### 阶段1: USB_CollectChCfg_Reply (通道配置)

**位置**: `User/usb_processor.c` 第375-461行

**处理流程**:
```
PC发送通道配置 → USB_CollectChCfg_Reply → 解析配置 → 更新g_spi_adc_cnt → 设置DMA传输大小
```

**关键代码**:
```c
// 1. 解析通道配置
for (size_t i = 0; i < channelTableHeader->nTotalChannelNum; i++)
{
    int32_t ch_id = channelTableElem[i].nChannelID;
    if (ch_id >= 0 && ch_id < ADC_CH_TOTAL)
    {
        g_ch_enable_mask |= (1u << ch_id);
        g_enabled_chs[g_enabled_ch_cnt++] = (uint8_t)ch_id;
        
        // 计算每个SPI总线的ADC数量
        uint8_t spi_idx = ch_id % SPI_NUM;
        uint8_t adc_pos = ch_id / SPI_NUM;
        if ((adc_pos + 1) > g_spi_adc_cnt[spi_idx])
            g_spi_adc_cnt[spi_idx] = adc_pos + 1;
    }
}

// 2. 根据新的g_spi_adc_cnt设置DMA传输大小
for (uint8_t spi_idx = 0; spi_idx < SPI_NUM; spi_idx++)
{
    uint8_t adc_count = g_spi_adc_cnt[spi_idx];
    if (adc_count == 0) adc_count = 8;  // 默认值
    uint32_t xfer_size = adc_count * 2;  // 2字节/ADC
    dma_set_spi_xfer_size(spi_idx, xfer_size);
}
```

**输出**:
- `g_spi_adc_cnt[3]` - 每个SPI总线的ADC数量
- `g_ch_enable_mask` - 通道使能掩码
- `g_enabled_chs[]` - 使能通道列表
- DMA传输大小已更新

### 阶段2: 定时器中断 (触发DMA采集)

**位置**: `Drivers/BSP/TIMER/gtim.c` 第55-106行

**处理流程**:
```
定时器中断 → HAL_TIM_PeriodElapsedCallback → 启动转换和DMA采集
```

**关键代码**:
```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    static uint8_t first_run = 1;
    
    if (first_run)
    {
        // 第一次运行，仅启动转换和DMA，不读取数据
        first_run = 0;
        ads8319_start_convst();
        dma_adc_start_collect();
        return;
    }
    
    // 读取上一帧DMA数据
    uint16_t adc_buf[SPI_NUM][SPI_CH_NUM];
    
    for (uint8_t spi = 0; spi < SPI_NUM; spi++)
    {
        if (g_spi_adc_cnt[spi] == 0) continue;
        
        // 从DMA缓冲区读取数据
        if (!dma_adc_get_data(spi, adc_buf[spi]))
        {
            continue;  // 没有就绪数据，跳过
        }
    }
    
    // 交错映射写回 g_cb_ch
    for (uint8_t spi = 0; spi < SPI_NUM; spi++)
    {
        for (uint8_t pos = 0; pos < g_spi_adc_cnt[spi]; pos++)
        {
            uint8_t ch = pos * SPI_NUM + spi;
            if (ch >= ADC_CH_TOTAL) continue;
            if (!(g_ch_enable_mask & (1u << ch))) continue;
            cb_write(g_cb_ch[ch], (const char *)&adc_buf[spi][pos], ADC_DATA_LEN);
        }
    }
    
    // 启动新一轮转换和DMA
    ads8319_start_convst();
    dma_adc_start_collect();
}
```

**关键点**:
1. 使用 `g_spi_adc_cnt[spi]` 控制循环次数
2. 使用 `g_ch_enable_mask` 过滤未使能的通道
3. 数据写入对应的环形缓冲区 `g_cb_ch[ch]`

### 阶段3: DMA传输完成回调 (处理采集数据)

**位置**: `Drivers/BSP/DMA_LIST/dma_spi_adc.c` 第132-159行

**处理流程**:
```
DMA传输完成 → dma_adc_transfer_complete → 转换数据 → 标记缓冲区就绪
```

**关键代码**:
```c
void dma_adc_transfer_complete(uint8_t spi_idx)
{
    if (spi_idx >= SPI_USED_MAX) return;
    
    uint32_t node_idx = g_buffer_mgr[spi_idx].current_rx_node;
    uint8_t *dma_buf = &spi_rx_dma_buf[spi_idx][node_idx][0];
    
    // 根据g_spi_adc_cnt处理实际采集的ADC数量
    uint8_t adc_count = g_spi_adc_cnt[spi_idx];
    for (uint8_t i = 0; i < adc_count; i++)
    {
        adc_dma_buffer[spi_idx][node_idx][i] = (dma_buf[2*i] << 8) | dma_buf[2*i + 1];
    }
    
    buffer_ready[spi_idx][node_idx] = 1;
    current_buffer[spi_idx] = node_idx;
}
```

**关键点**:
1. 根据 `g_spi_adc_cnt[spi_idx]` 处理实际ADC数量
2. 转换字节数据为16位ADC数据
3. 标记缓冲区就绪，供定时器中断读取

### 阶段4: USB_Display_Reply (发送数据到PC)

**位置**: `User/usb_processor.c` 第693-766行

**处理流程**:
```
PC请求数据 → USB_Display_Reply → 检查缓冲区 → 读取数据 → 打包发送
```

**关键代码**:
```c
static uint32_t USB_Display_Reply(uint8_t *data_in, uint32_t data_len,
                                  FrameHeadInfo *frame_head,
                                  UserDataHeadInfo *user_head)
{
    static uint32_t frame_num = 0;
    const uint32_t cb_needed = BLOCK_LEN * ADC_DATA_LEN;
    
    // 没有配置通道则直接返回
    if (g_enabled_ch_cnt == 0) return 0;
    
    // 检查所有配置通道是否就绪
    for (uint8_t i = 0; i < g_enabled_ch_cnt; i++)
    {
        uint8_t ch = g_enabled_chs[i];
        if (!g_cb_ch[ch] || cb_size(g_cb_ch[ch]) < (int)cb_needed)
            return 0;  // 有通道未就绪，本次不发
    }
    
    /* 填充帧头 */
    struct UserData user_data;
    memset(&user_data, 0, sizeof(user_data));
    frame_num++;
    user_data.data_head.nFrameChCount = g_enabled_ch_cnt;
    user_data.data_head.nFrameLen = BLOCK_LEN;
    
    /* 按配置顺序读取，与上位机通道顺序一致 */
    for (uint8_t n = 0; n < g_enabled_ch_cnt; n++)
    {
        uint8_t ch = g_enabled_chs[n];
        cb_read(g_cb_ch[ch], (char *)user_data.send_frame[n], cb_needed);
    }
    
    // 打包并发送数据
    uint32_t send_len = sizeof(ArmBackFrameHeader) +
                        (uint32_t)g_enabled_ch_cnt * BLOCK_LEN * sizeof(short);
    
    FrameHeadInfo reply_frame_head = create_default_frame_head(frame_num);
    UserDataHeadInfo reply_user_head = create_user_data_head(
        DVSARM_DISPNEXT_OK, SOURCE_TYPE_WITH_DATAS, DESTINATION_ARM_TO_PC, send_len);
    
    uint32_t packet_len = 0;
    pack_data((uint8_t *)&user_data, send_len,
              &reply_user_head, &reply_frame_head, &packet_len);
    return packet_len;
}
```

**关键点**:
1. 使用 `g_enabled_ch_cnt` 和 `g_enabled_chs[]` 确定发送哪些通道
2. 从环形缓冲区 `g_cb_ch[ch]` 读取数据
3. 按配置顺序打包数据，与上位机通道顺序一致

## 完整数据流示例

### 示例：配置3个通道（CH0, CH1, CH2）

1. **USB配置**:
   - `g_spi_adc_cnt = [1, 1, 1]`
   - `g_enabled_ch_cnt = 3`
   - `g_enabled_chs = [0, 1, 2]`
   - `g_ch_enable_mask = 0x07`

2. **定时器中断采集**:
   - SPI0采集1个ADC → 数据写入 `g_cb_ch[0]`
   - SPI1采集1个ADC → 数据写入 `g_cb_ch[1]`
   - SPI2采集1个ADC → 数据写入 `g_cb_ch[2]`

3. **USB发送**:
   - 读取 `g_cb_ch[0]`, `g_cb_ch[1]`, `g_cb_ch[2]`
   - 打包发送3个通道的数据

## 关键问题检查

### 1. DMA传输大小是否正确应用？
- ✅ `USB_CollectChCfg_Reply` 中调用 `dma_set_spi_xfer_size()`
- ✅ `dma_get_xfer_size()` 在启动传输时返回正确大小
- ✅ `SPI_CR2_TSIZE` 寄存器设置为计算出的大小

### 2. 数据处理是否正确？
- ✅ `dma_adc_transfer_complete` 根据 `g_spi_adc_cnt` 处理数据
- ✅ `dma_adc_get_data` 根据 `g_spi_adc_cnt` 复制数据
- ✅ 定时器中断根据 `g_spi_adc_cnt` 写入环形缓冲区

### 3. 数据发送是否正确？
- ✅ `USB_Display_Reply` 根据 `g_enabled_chs` 读取数据
- ✅ 按配置顺序发送，与上位机通道顺序一致

### 4. 缓冲区管理是否正确？
- ✅ `adc_dma_buffer` 定义为 `[SPI_USED_MAX][DMA_SPI_RX_NODE_USED][ADS8319_CHAIN_LENGTH]`
- ✅ 足够容纳最大8个ADC的数据
- ✅ 环形缓冲区 `g_cb_ch` 管理每个通道的数据

## 调试建议

1. 在 `USB_CollectChCfg_Reply` 中添加调试打印，显示 `g_spi_adc_cnt` 和DMA传输大小
2. 在 `dma_adc_transfer_complete` 中添加调试打印，显示实际处理的ADC数量
3. 在 `USB_Display_Reply` 中添加调试打印，显示读取的通道数据
4. 使用示波器或逻辑分析仪验证SPI信号和DMA传输

## 注意事项

1. **硬编码设置移除**: 确保 `usb_processor.c` 中没有硬编码的 `g_spi_adc_cnt` 设置
2. **默认值处理**: 如果某个SPI总线没有配置通道，使用默认值8个ADC
3. **实时性**: DMA传输大小在配置完成后立即更新，下次传输时生效
4. **兼容性**: 修改保持了与现有代码的兼容性
