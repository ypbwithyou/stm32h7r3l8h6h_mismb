# 离线模式完整流程分析

## 流程概述

当 `offline_processor(g_IdaSystemStatus.st_dev_offline.offline_mode)` 参数为1时，系统进入离线模式，完整流程如下：

1. **离线配置加载** - 从文件读取通道配置和调度计划
2. **定时器中断触发DMA采集** - 根据配置采集ADC数据
3. **DMA传输完成回调** - 处理采集数据
4. **离线任务调度** - 根据时间计划执行采集和记录
5. **文件保存** - 将采集数据写入文件

## 详细流程分析

### 阶段1: 离线配置加载

**位置**: `User/offline_processor.c` 第427-532行

**处理流程**:
```
offline_processor(1) → 检查配置文件 → 读取通道配置 → 计算g_spi_adc_cnt → 设置DMA传输大小
```

**关键代码**:
```c
// 1. 检查配置文件是否存在
FRESULT fres = f_stat(OFFLINE_SCHEDULE_FILE, &fno);
if (fres == FR_NO_FILE || fres == FR_NO_PATH)
{
    // 文件不存在，等待重试
    return;
}

// 2. 读取并校验配置
if (GetOfflineCfgParam(OFFLINE_SCHEDULE_FILE) < 0)
{
    // 配置读取失败
    return;
}

if (CheckOfflineCfgParam() < 0)
{
    // 配置校验失败
    return;
}

// 3. 配置加载成功
g_IdaSystemStatus.st_dev_run.collect_cfg_flag = COLLECT_CFG_OK;
g_IdaSystemStatus.st_dev_offline.start_flag = 1;
```

**输出**:
- `g_offline_chCfgHeader` - 通道配置头信息
- `g_offline_chCfgParam[]` - 通道配置参数
- `g_offline_GlobalParam` - 全局参数
- `g_offline_ScheduleParam[]` - 调度计划参数

### 阶段2: 调度执行 - 采集开始

**位置**: `User/offline_processor.c` 第243-336行 (`HandleAcqStart`)

**处理流程**:
```
时间到达 → ExecuteScheduleAction → HandleAcqStart → 配置通道 → 设置DMA大小 → 启动采集
```

**关键代码**:
```c
static void HandleAcqStart(uint8_t idx, uint32_t elapsed_seconds)
{
    // 1. 通道配置
    g_ch_enable_mask = 0;
    g_enabled_ch_cnt = 0;
    memset(g_spi_adc_cnt, 0, sizeof(g_spi_adc_cnt));
    
    for (size_t i = 0; i < g_offline_chCfgHeader.nTotalChannelNum; i++)
    {
        int32_t ch_id = g_offline_chCfgParam[i].nChannelID;
        if (ch_id >= 0 && ch_id < ADC_CH_TOTAL)
        {
            g_ch_enable_mask |= (1u << ch_id);
            g_enabled_chs[g_enabled_ch_cnt++] = (uint8_t)ch_id;
            
            uint8_t spi_idx = ch_id % SPI_NUM;
            uint8_t adc_pos = ch_id / SPI_NUM;
            if ((adc_pos + 1) > g_spi_adc_cnt[spi_idx])
                g_spi_adc_cnt[spi_idx] = adc_pos + 1;
        }
    }
    
    // 2. 设置DMA传输大小
    for (uint8_t spi_idx = 0; spi_idx < SPI_NUM; spi_idx++)
    {
        uint8_t adc_count = g_spi_adc_cnt[spi_idx];
        if (adc_count == 0) adc_count = 8;
        uint32_t xfer_size = adc_count * 2;
        dma_set_spi_xfer_size(spi_idx, xfer_size);
    }
    
    // 3. 启动采集
    if (sample_rate > 0)
    {
        CfgAdcSampleRate(sample_rate);
        g_IdaSystemStatus.st_dev_run.run_flag = 1;
        AdcCollectorContrl(1);  // 启动定时器
    }
}
```

**关键点**:
1. 根据离线配置计算 `g_spi_adc_cnt`
2. 动态设置DMA传输大小
3. 启动定时器中断触发DMA采集

### 阶段3: 定时器中断触发DMA采集

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
        
        if (!dma_adc_get_data(spi, adc_buf[spi]))
        {
            continue;
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

### 阶段4: DMA传输完成回调

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

### 阶段5: 离线任务调度

**位置**: `User/offline_processor.c` 第546-600行

**处理流程**:
```
检查时间计划 → ExecuteScheduleAction → 执行采集或记录操作
```

**关键代码**:
```c
for (uint8_t i = 0; i < g_offline_GlobalParam.nScheduleCount; i++)
{
    ScheduleStatus *status = &g_schedule_run_status[i];
    
    if (*status == STATUS_END) continue;
    
    uint32_t start_time = (uint32_t)g_offline_ScheduleParam[i].param0;
    
    if (elapsed_seconds < start_time) continue;
    
    // 未执行
    if (*status == STATUS_NO)
    {
        ExecuteScheduleAction(i, elapsed_seconds);
        continue;
    }
    
    // 运行中
    if (*status == STATUS_ING)
    {
        uint32_t duration = (uint32_t)g_offline_ScheduleParam[i].param1;
        
        if (duration == 0) continue;
        
        if (elapsed_seconds >= start_time + duration)
        {
            *status = STATUS_END;
            
            if (g_offline_ScheduleParam[i].nType == Record_Start)
            {
                HandleRecordEnd(i);
                g_IdaSystemStatus.st_dev_offline.offline_mode = 0;
            }
            
            if (g_offline_ScheduleParam[i].nType == ACQ_Start)
            {
                g_IdaSystemStatus.st_dev_offline.start_flag = 0;
                g_IdaSystemStatus.st_dev_run.run_flag = 0;
                AdcCollectorContrl(0);
                g_IdaSystemStatus.st_dev_offline.offline_mode = 0;
            }
        }
    }
}
```

### 阶段6: 文件保存

**位置**: `User/offline_processor.c` 第894-993行 (`OfflineDatasRecord`)

**处理流程**:
```
检查缓冲区数据 → 读取数据 → 写入文件 → 更新文件头
```

**关键代码**:
```c
static void OfflineDatasRecord(void)
{
    // 1. 检查是否有足够数据
    for (uint8_t i = 0; i < ADC_CH_TOTAL; i++)
    {
        if (!(g_ch_enable_mask & (1u << i))) continue;
        if (!g_cb_ch[i] || cb_size(g_cb_ch[i]) < (int)block_bytes)
            return;  // 数据不足，等待下次
    }
    
    // 2. 按通道逐个写入文件
    for (uint8_t ch = 0; ch < ADC_CH_TOTAL; ch++)
    {
        if (!(g_ch_enable_mask & (1u << ch))) continue;
        
        // 填写通道帧头
        RECORD_FRAMEHEADER rec_hdr;
        // ... 填充帧头信息 ...
        
        // 写帧头
        res = f_write(&g_offline_record_fil, &rec_hdr, sizeof(rec_hdr), &bw);
        
        // 从环形缓冲区读取数据并写入文件
        cb_read(g_cb_ch[ch], (char *)ch_buf, block_bytes);
        res = f_write(&g_offline_record_fil, ch_buf, block_bytes, &bw);
    }
    
    // 3. 记录停止时的最终处理
    if (g_IdaSystemStatus.st_dev_record.record_status == RECORD_STOP)
    {
        g_recorde_file_head.nFrameNum = record_frame_num;
        g_recorde_file_head.dRecValidEndTime = dwt_get_ns() / NANOSECONDS_PER_SECOND;
        
        // 回写文件头部（更新帧数）
        res = f_lseek(&g_offline_record_fil, 0);
        res = f_write(&g_offline_record_fil, &g_recorde_file_head, sizeof(g_recorde_file_head), &bw);
    }
}
```

## 完整数据流示例

### 示例：离线采集3个通道（CH0, CH1, CH2），持续10秒

1. **配置加载**:
   - 读取离线配置文件
   - 解析通道配置：CH0, CH1, CH2
   - `g_spi_adc_cnt = [1, 1, 1]`
   - `g_enabled_ch_cnt = 3`
   - `g_enabled_chs = [0, 1, 2]`

2. **调度执行** (时间到达0秒):
   - `HandleAcqStart` 被调用
   - 设置DMA传输大小：[2, 2, 2] 字节
   - 启动定时器，开始采集

3. **定时器中断采集** (持续10秒):
   - 每次中断采集1个ADC数据（每个SPI总线）
   - 数据写入 `g_cb_ch[0]`, `g_cb_ch[1]`, `g_cb_ch[2]`

4. **文件保存** (持续进行):
   - `OfflineDatasRecord` 检查缓冲区数据
   - 读取每个通道的数据
   - 写入文件（带帧头）

5. **采集结束** (时间到达10秒):
   - `HandleRecordEnd` 被调用
   - 停止采集
   - 更新文件头
   - 关闭文件

## 关键问题检查

### 1. DMA传输大小是否正确应用？
✅ **已检查**: `HandleAcqStart` 中调用 `dma_set_spi_xfer_size()`

### 2. 数据处理是否正确？
✅ **已检查**: `dma_adc_transfer_complete` 根据 `g_spi_adc_cnt` 处理数据

### 3. 文件保存是否正确？
✅ **已检查**: `OfflineDatasRecord` 按通道写入文件，带帧头信息

### 4. 调度逻辑是否正确？
✅ **已检查**: 根据时间计划执行采集和记录操作

## 注意事项

1. **配置文件**: 离线模式需要配置文件 `OFFLINE_SCHEDULE_FILE`
2. **实时性**: DMA传输大小在配置完成后立即更新
3. **缓冲区管理**: 确保环形缓冲区足够大，避免数据丢失
4. **文件系统**: 确保SD卡或文件系统正常工作

## 调试建议

1. 在 `HandleAcqStart` 中添加调试打印，显示 `g_spi_adc_cnt` 和DMA传输大小
2. 在 `OfflineDatasRecord` 中添加调试打印，显示写入的通道和数据量
3. 检查文件写入速度和数据完整性
4. 使用示波器验证SPI信号和DMA传输
