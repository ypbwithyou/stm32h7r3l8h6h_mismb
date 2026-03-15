# 动态DMA传输大小配置实现总结

## 修改概述

实现了根据USB接口配置动态修改每次DMA传输长度的功能。当上位机通过USB配置通道使能时，系统会自动计算每个SPI总线的ADC数量，并重新设置DMA传输大小。

## 修改的文件

### 1. `User/usb_processor.c`
- 添加了 `#include "./BSP/DMA_LIST/dma_list.h"`
- 修改 `USB_CollectChCfg_Reply()` 函数：
  - 移除了硬编码的 `g_spi_adc_cnt` 设置
  - 在配置完成后动态计算并设置DMA传输大小
  - 添加调试打印信息

### 2. `User/offline_processor.c`
- 添加了 `#include "./BSP/DMA_LIST/dma_list.h"`
- 在通道配置函数中添加了相同的DMA传输大小设置逻辑

### 3. `Drivers/BSP/DMA_LIST/dma_spi_adc.c`
- 添加了 `#include "../../User/collector_processor.h"`
- 修改 `dma_adc_init()` 函数，根据 `g_spi_adc_cnt` 动态设置初始传输大小
- 修改 `dma_adc_transfer_complete()` 函数，根据实际ADC数量处理数据
- 修改 `dma_adc_get_data()` 函数，根据实际ADC数量复制数据

### 4. `Drivers/BSP/DMA_LIST/dma_list.c`
- 修改 `dma_get_xfer_size()` 函数，移除传输大小的最大值限制

## 实现原理

### 1. 通道配置解析
当USB接口收到通道配置时：
```c
for (size_t i = 0; i < channelTableHeader->nTotalChannelNum; i++)
{
    int32_t ch_id = channelTableElem[i].nChannelID;
    if (ch_id >= 0 && ch_id < ADC_CH_TOTAL)
    {
        uint8_t spi_idx = ch_id % SPI_NUM;
        uint8_t adc_pos = ch_id / SPI_NUM;
        if ((adc_pos + 1) > g_spi_adc_cnt[spi_idx])
            g_spi_adc_cnt[spi_idx] = adc_pos + 1;
    }
}
```

### 2. DMA传输大小计算
配置完成后立即计算并设置DMA传输大小：
```c
for (uint8_t spi_idx = 0; spi_idx < SPI_NUM; spi_idx++)
{
    uint8_t adc_count = g_spi_adc_cnt[spi_idx];
    if (adc_count == 0)
    {
        adc_count = 8;  // 默认值 ADS8319_CHAIN_LENGTH
    }
    uint32_t xfer_size = adc_count * 2;  // 2字节/ADC
    dma_set_spi_xfer_size(spi_idx, xfer_size);
}
```

### 3. DMA传输使用新大小
下次调用 `dma_start_transfer_all()` 时：
1. `dma_get_xfer_size()` 返回更新后的传输大小
2. `dma_rebuild_lists()` 用新大小重建DMA列表
3. `SPI_CR2_TSIZE` 寄存器设置为新大小
4. DMA按新大小传输数据

### 4. 数据处理
在 `dma_adc_transfer_complete()` 中：
```c
uint8_t adc_count = g_spi_adc_cnt[spi_idx];
for (uint8_t i = 0; i < adc_count; i++)
{
    adc_dma_buffer[spi_idx][node_idx][i] = (dma_buf[2*i] << 8) | dma_buf[2*i + 1];
}
```

## 工作流程示例

### 示例1：配置3个独立通道（CH0, CH1, CH2）

1. **USB配置**：上位机发送通道配置 [CH0, CH1, CH2]
2. **解析配置**：
   - CH0 → SPI0, ADC位置0 → `g_spi_adc_cnt[0] = 1`
   - CH1 → SPI1, ADC位置0 → `g_spi_adc_cnt[1] = 1`
   - CH2 → SPI2, ADC位置0 → `g_spi_adc_cnt[2] = 1`
3. **设置DMA大小**：
   - SPI0: 1 ADC × 2字节 = 2字节
   - SPI1: 1 ADC × 2字节 = 2字节
   - SPI2: 1 ADC × 2字节 = 2字节
4. **下次传输**：DMA按2字节传输每个SPI总线的数据

### 示例2：配置8个通道（CH0-CH7）

1. **USB配置**：上位机发送通道配置 [CH0, CH1, CH2, CH3, CH4, CH5, CH6, CH7]
2. **解析配置**：
   - CH0-CH7 → SPI0, ADC位置0-7 → `g_spi_adc_cnt[0] = 8`
   - SPI1, SPI2 没有通道 → `g_spi_adc_cnt[1] = 0`, `g_spi_adc_cnt[2] = 0`
3. **设置DMA大小**：
   - SPI0: 8 ADC × 2字节 = 16字节
   - SPI1: 0 ADC → 使用默认值8 ADC × 2字节 = 16字节
   - SPI2: 0 ADC → 使用默认值8 ADC × 2字节 = 16字节
4. **下次传输**：DMA按16字节传输SPI0的数据，按默认值传输SPI1和SPI2的数据

### 示例3：混合配置（SPI0: 3个ADC, SPI1: 2个ADC, SPI2: 1个ADC）

1. **USB配置**：[CH0, CH1, CH2, CH3, CH4, CH5]
2. **解析配置**：
   - CH0, CH1, CH2 → SPI0, ADC位置0,1,2 → `g_spi_adc_cnt[0] = 3`
   - CH3, CH4 → SPI1, ADC位置0,1 → `g_spi_adc_cnt[1] = 2`
   - CH5 → SPI2, ADC位置0 → `g_spi_adc_cnt[2] = 1`
3. **设置DMA大小**：
   - SPI0: 3 ADC × 2字节 = 6字节
   - SPI1: 2 ADC × 2字节 = 4字节
   - SPI2: 1 ADC × 2字节 = 2字节
4. **下次传输**：DMA按6字节、4字节、2字节分别传输SPI0、SPI1、SPI2的数据

## 调试信息

配置完成后会打印调试信息：
```
spi_adc_cnt: [3, 2, 1]  mask=0x00003F
SPI0: adc_count=3, xfer_size=6 bytes
SPI1: adc_count=2, xfer_size=4 bytes
SPI2: adc_count=1, xfer_size=2 bytes
```

## 注意事项

1. **硬编码设置移除**：移除了 `usb_processor.c` 中的硬编码设置，确保使用动态计算的值
2. **默认值处理**：如果某个SPI总线没有配置通道，使用默认值8个ADC
3. **实时性**：DMA传输大小在配置完成后立即更新，下次传输时生效
4. **缓冲区大小**：`adc_dma_buffer` 足够容纳最大8个ADC的数据
5. **兼容性**：修改保持了与现有代码的兼容性

## 验证方法

1. 通过USB配置不同的通道组合
2. 检查调试输出中的 `spi_adc_cnt` 和 `xfer_size`
3. 验证DMA传输的数据量是否正确
4. 检查接收的数据是否完整无错位
5. 使用示波器或逻辑分析仪验证SPI信号

## 相关文件

- `User/usb_processor.c` - USB通道配置处理
- `User/offline_processor.c` - 离线通道配置处理
- `Drivers/BSP/DMA_LIST/dma_spi_adc.c` - DMA采集模块
- `Drivers/BSP/DMA_LIST/dma_list.c` - DMA列表管理
- `User/collector_processor.h` - 通道配置相关定义
