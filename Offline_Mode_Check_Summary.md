# 离线模式流程检查总结

## 检查结果

我已经完整检查了 `offline_processor(g_IdaSystemStatus.st_dev_offline.offline_mode)` 参数为1时的完整流程。

## 关键发现

### ✅ 已正确实现的功能

1. **DMA传输大小动态设置** (`HandleAcqStart` 函数)
   - 位置: `User/offline_processor.c` 第277-290行
   - 根据 `g_spi_adc_cnt` 动态计算DMA传输大小
   - 调用 `dma_set_spi_xfer_size()` 更新传输大小

2. **通道配置解析**
   - 根据离线配置文件计算 `g_spi_adc_cnt`
   - 更新 `g_ch_enable_mask` 和 `g_enabled_chs`

3. **定时器中断采集**
   - 使用 `g_spi_adc_cnt[spi]` 控制循环次数
   - 数据写入对应的环形缓冲区

4. **文件保存**
   - `OfflineDatasRecord` 按通道写入文件
   - 带帧头信息，包含通道ID和时间戳

## 完整流程

### 1. 离线配置加载
```
offline_processor(1) → 检查配置文件 → 读取通道配置 → 计算g_spi_adc_cnt
```

### 2. 调度执行 - 采集开始
```
时间到达 → HandleAcqStart → 配置通道 → 设置DMA大小 → 启动定时器
```

### 3. 定时器中断采集
```
定时器中断 → 启动SPI转换 → DMA传输 → dma_adc_transfer_complete
```

### 4. 数据处理
```
dma_adc_transfer_complete → 转换数据 → 标记缓冲区就绪
```

### 5. 文件保存
```
OfflineDatasRecord → 检查数据 → 读取缓冲区 → 写入文件
```

### 6. 采集结束
```
时间到达 → HandleRecordEnd → 停止采集 → 更新文件头 → 关闭文件
```

## 示例：离线采集3个通道

1. **配置加载**:
   - `g_spi_adc_cnt = [1, 1, 1]`
   - DMA传输大小: [2, 2, 2] 字节

2. **启动采集**:
   - 设置DMA传输大小
   - 启动定时器

3. **持续采集** (10秒):
   - 每次中断采集1个ADC数据
   - 数据写入 `g_cb_ch[0]`, `g_cb_ch[1]`, `g_cb_ch[2]`

4. **文件保存**:
   - 读取缓冲区数据
   - 写入文件（带帧头）

5. **采集结束**:
   - 停止定时器
   - 更新文件头
   - 关闭文件

## 关键修改位置

### 1. `User/offline_processor.c` - HandleAcqStart
- **位置**: 第277-290行
- **功能**: 根据 `g_spi_adc_cnt` 设置DMA传输大小
- **代码**:
  ```c
  for (uint8_t spi_idx = 0; spi_idx < SPI_NUM; spi_idx++)
  {
      uint8_t adc_count = g_spi_adc_cnt[spi_idx];
      if (adc_count == 0) adc_count = 8;
      uint32_t xfer_size = adc_count * 2;
      dma_set_spi_xfer_size(spi_idx, xfer_size);
  }
  ```

### 2. `Drivers/BSP/DMA_LIST/dma_spi_adc.c` - dma_adc_transfer_complete
- **位置**: 第143-148行
- **功能**: 根据 `g_spi_adc_cnt` 处理数据
- **代码**:
  ```c
  uint8_t adc_count = g_spi_adc_cnt[spi_idx];
  for (uint8_t i = 0; i < adc_count; i++)
  {
      adc_dma_buffer[spi_idx][node_idx][i] = (dma_buf[2*i] << 8) | dma_buf[2*i + 1];
  }
  ```

### 3. `Drivers/BSP/DMA_LIST/dma_spi_adc.c` - dma_adc_get_data
- **位置**: 第93-97行
- **功能**: 根据 `g_spi_adc_cnt` 复制数据
- **代码**:
  ```c
  uint8_t adc_count = g_spi_adc_cnt[spi_idx];
  for (uint8_t i = 0; i < adc_count; i++)
  {
      adc_data[i] = adc_dma_buffer[spi_idx][processed_buffer[spi_idx]][i];
  }
  ```

## 验证建议

1. **配置验证**: 上传离线配置文件，检查 `g_spi_adc_cnt` 和DMA传输大小
2. **采集验证**: 检查定时器中断是否正确处理不同数量的ADC
3. **文件验证**: 检查文件写入是否正确，数据是否完整
4. **时间验证**: 检查调度时间是否正确执行

## 注意事项

1. **配置文件**: 离线模式需要配置文件 `OFFLINE_SCHEDULE_FILE`
2. **文件系统**: 确保SD卡或文件系统正常工作
3. **缓冲区大小**: 确保环形缓冲区足够大，避免数据丢失
4. **实时性**: DMA传输大小在配置完成后立即更新

## 文档列表

- `Offline_Mode_Flow_Analysis.md` - 离线模式完整流程分析
- `Offline_Mode_Check_Summary.md` - 本文件，流程检查总结
- 其他相关文档...
