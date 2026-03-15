# DMA传输大小动态设置修改说明

## 修改概述

根据您的要求，已修改代码，使得DMA传输大小根据 `g_spi_adc_cnt` 每个通道的采集数动态设置，而不是使用固定的 `RX_BUFFER_SIZE`（16字节）。

## 修改的文件和位置

### 1. `Drivers/BSP/DMA_LIST/dma_spi_adc.c`

#### 1.1 添加头文件包含
```c
#include "../../User/collector_processor.h"  // 用于访问 g_spi_adc_cnt
```

#### 1.2 修改 `dma_adc_init()` 函数
将固定的传输大小改为根据 `g_spi_adc_cnt` 动态计算：
```c
/* 设置SPI传输大小（根据g_spi_adc_cnt每个通道的采集数动态设置） */
for (uint8_t spi_idx = 0; spi_idx < SPI_USED_MAX; spi_idx++)
{
    /* 每个ADC转换结果为2字节，计算总传输大小 */
    /* 如果g_spi_adc_cnt未初始化或为0，使用默认值ADS8319_CHAIN_LENGTH */
    uint8_t adc_count = g_spi_adc_cnt[spi_idx];
    if (adc_count == 0)
    {
        adc_count = ADS8319_CHAIN_LENGTH;  // 默认使用最大ADC数量
    }
    uint32_t xfer_size = adc_count * 2;  // 2字节/ADC
    dma_set_spi_xfer_size(spi_idx, xfer_size);
}
```

#### 1.3 修改 `dma_adc_transfer_complete()` 函数
将数据处理从固定的 `ADS8319_CHAIN_LENGTH` 改为根据 `g_spi_adc_cnt` 动态处理：
```c
/* 根据g_spi_adc_cnt处理实际采集的ADC数量 */
uint8_t adc_count = g_spi_adc_cnt[spi_idx];
for (uint8_t i = 0; i < adc_count; i++)
{
    adc_dma_buffer[spi_idx][node_idx][i] = (dma_buf[2*i] << 8) | dma_buf[2*i + 1];
}
```

#### 1.4 修改 `dma_adc_get_data()` 函数
将数据复制从固定的 `ADS8319_CHAIN_LENGTH` 改为根据 `g_spi_adc_cnt` 动态复制：
```c
/* 根据g_spi_adc_cnt复制实际采集的ADC数据 */
uint8_t adc_count = g_spi_adc_cnt[spi_idx];
for (uint8_t i = 0; i < adc_count; i++)
{
    adc_data[i] = adc_dma_buffer[spi_idx][node_idx][i];
}
```

#### 1.5 删除重复函数
删除了重复的 `dma_adc_transfer_complete()` 函数定义。

### 2. `Drivers/BSP/DMA_LIST/dma_list.c`

#### 2.1 修改 `dma_get_xfer_size()` 函数
移除了传输大小的最大值限制，允许根据实际需要动态设置：
```c
static uint32_t dma_get_xfer_size(uint8_t spi_idx)
{
    uint32_t size = g_dma_spi_xfer_size[spi_idx];
    if (size == 0)
    {
        size = RX_BUFFER_SIZE;
    }
    // 不再限制最大值，允许根据g_spi_adc_cnt动态设置传输大小
    return size;
}
```

## 工作原理

1. **初始化时**：根据每个 SPI 总线上启用的 ADC 数量 (`g_spi_adc_cnt[spi_idx]`) 计算 DMA 传输大小（每个 ADC 2 字节）
   - 如果 `g_spi_adc_cnt[spi_idx]` 为 0，则使用默认值 `ADS8319_CHAIN_LENGTH`（8个ADC）
   - 传输大小 = `g_spi_adc_cnt[spi_idx] * 2` 字节

2. **传输时**：DMA 按照计算出的大小进行数据传输

3. **处理时**：根据实际的 ADC 数量处理接收到的数据，避免处理未启用的 ADC 数据

## 注意事项

1. **缓冲区大小**：`adc_dma_buffer` 的定义是 `[SPI_USED_MAX][DMA_SPI_RX_NODE_USED][ADS8319_CHAIN_LENGTH]`，这意味着每个 SPI 总线有 8 个 ADC 的空间，与 `g_spi_adc_cnt` 的最大值一致，是安全的。

2. **默认值处理**：在 `dma_adc_init()` 中添加了默认值处理，如果 `g_spi_adc_cnt` 未初始化或为 0，则使用 `ADS8319_CHAIN_LENGTH` 作为默认值。

3. **兼容性**：修改保持了与现有代码的兼容性，现有的 `gtim.c`、`usb_processor.c` 和 `offline_processor.c` 中的逻辑不需要修改。

## 测试建议

1. 验证不同 `g_spi_adc_cnt` 值下的 DMA 传输是否正常
2. 检查数据是否正确处理，没有错位或丢失
3. 验证缓冲区大小是否足够
4. 检查默认值处理是否正常工作

## 文件列表

- `Drivers/BSP/DMA_LIST/dma_spi_adc.c` - 主要修改文件
- `Drivers/BSP/DMA_LIST/dma_list.c` - 辅助修改文件
- `User/collector_processor.h` - 包含 `g_spi_adc_cnt` 定义
