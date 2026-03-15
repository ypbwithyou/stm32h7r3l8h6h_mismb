# dma_adc_has_data 函数检查总结

## 检查结果

### 问题发现
`dma_adc_has_data` 函数在头文件中声明，但在 `.c` 文件中未实现。

### 已完成修复
✅ **已实现** `dma_adc_has_data` 函数

## 函数实现

### 位置
`Drivers/BSP/DMA_LIST/dma_spi_adc.c` 第83-89行

### 代码
```c
/**
 * @brief  检查是否有就绪数据
 * @param  spi_idx: SPI索引
 * @return 1: 有数据, 0: 无数据
 */
uint8_t dma_adc_has_data(uint8_t spi_idx)
{
    if (spi_idx >= SPI_USED_MAX)
        return 0;
    
    return buffer_ready[spi_idx][processed_buffer[spi_idx]];
}
```

## 功能分析

### 函数用途
- **功能**: 检查指定SPI总线是否有就绪数据
- **返回值**: 1=有数据, 0=无数据
- **不读取数据**: 仅检查状态，不修改缓冲区

### 与 `dma_adc_get_data` 的区别

| 功能 | `dma_adc_has_data` | `dma_adc_get_data` |
|------|-------------------|-------------------|
| 检查数据 | ✅ | ✅ |
| 读取数据 | ❌ | ✅ |
| 修改缓冲区 | ❌ | ✅ (标记为已处理) |
| 返回值 | 1/0 (是否有数据) | 1/0 (是否成功读取) |

### 使用场景
1. **预检查**: 在读取数据前检查是否有数据
2. **状态查询**: 查询DMA采集状态
3. **条件判断**: 根据数据存在性执行不同逻辑

## 当前使用情况

### 查找使用情况
```bash
find . -name "*.c" -o -name "*.h" | xargs grep -l "dma_adc_has_data"
# 结果: 只有声明和实现文件，无调用文件
```

### 当前使用模式
当前代码主要使用 `dma_adc_get_data`，该函数已包含数据检查逻辑：
```c
if (!dma_adc_get_data(spi, adc_buf[spi]))
{
    // 如果没有就绪数据，跳过此SPI
    continue;
}
```

## 建议使用方式

### 方式1: 预检查后读取
```c
// 检查是否有数据
if (dma_adc_has_data(spi_idx))
{
    // 有数据，读取数据
    dma_adc_get_data(spi_idx, adc_data);
}
```

### 方式2: 状态查询
```c
// 查询DMA采集状态
for (uint8_t spi = 0; spi < SPI_NUM; spi++)
{
    if (dma_adc_has_data(spi))
    {
        usb_printf("SPI%d has data ready\n", spi);
    }
}
```

## 总结

1. **问题**: `dma_adc_has_data` 函数未实现
2. **修复**: ✅ 已实现该函数
3. **功能**: 仅检查数据存在性，不读取数据
4. **使用**: 当前未使用，但保持API完整性
5. **建议**: 可用于预检查或状态查询场景

整个DMA操作流程完整，`dma_adc_has_data` 函数已实现并可供使用。
