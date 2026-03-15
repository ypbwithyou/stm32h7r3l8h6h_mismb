# USB接口配置动态修改DMA传输长度实现说明

## 概述

当通过USB接口配置通道使能时，`g_spi_adc_cnt` 会动态改变。为了正确传输数据，需要在配置更新后重新设置DMA传输大小。

## 修改的文件

### 1. `User/usb_processor.c`

#### 1.1 添加头文件包含
```c
#include "./BSP/DMA_LIST/dma_list.h"  // 用于 dma_set_spi_xfer_size
```

#### 1.2 修改 `USB_CollectChCfg_Reply` 函数

**原代码（有问题）**：
```c
g_spi_adc_cnt[0] = 1;
g_spi_adc_cnt[1] = 1;
g_spi_adc_cnt[2] = 1;
```

**新代码**：
```c
// 注意：移除了硬编码的 g_spi_adc_cnt 设置，使用动态计算的值
// g_spi_adc_cnt[0] = 1;
// g_spi_adc_cnt[1] = 1;
// g_spi_adc_cnt[2] = 1;

usb_printf("spi_adc_cnt: [%d, %d, %d]  mask=0x%06lX\n",
           g_spi_adc_cnt[0], g_spi_adc_cnt[1], g_spi_adc_cnt[2],
           g_ch_enable_mask);

/* 根据新的 g_spi_adc_cnt 设置 DMA 传输大小 */
for (uint8_t spi_idx = 0; spi_idx < SPI_NUM; spi_idx++)
{
    /* 每个ADC转换结果为2字节，计算总传输大小 */
    /* 如果g_spi_adc_cnt为0，使用默认值ADS8319_CHAIN_LENGTH */
    uint8_t adc_count = g_spi_adc_cnt[spi_idx];
    if (adc_count == 0)
    {
        adc_count = 8;  // ADS8319_CHAIN_LENGTH 默认值
    }
    uint32_t xfer_size = adc_count * 2;  // 2字节/ADC
    dma_set_spi_xfer_size(spi_idx, xfer_size);
    usb_printf("SPI%d: adc_count=%d, xfer_size=%d bytes\n", spi_idx, adc_count, xfer_size);
}
```

### 2. `User/offline_processor.c`

#### 2.1 添加头文件包含
```c
#include "./BSP/DMA_LIST/dma_list.h"  // 用于 dma_set_spi_xfer_size
```

#### 2.2 修改配置函数

在 `g_spi_adc_cnt` 更新后添加相同的DMA传输大小设置代码：

```c
usb_printf("spi_adc_cnt: [%d, %d, %d]  mask=0x%06lX\n",
           g_spi_adc_cnt[0], g_spi_adc_cnt[1], g_spi_adc_cnt[2],
           g_ch_enable_mask);

/* 根据新的 g_spi_adc_cnt 设置 DMA 传输大小 */
for (uint8_t spi_idx = 0; spi_idx < SPI_NUM; spi_idx++)
{
    /* 每个ADC转换结果为2字节，计算总传输大小 */
    /* 如果g_spi_adc_cnt为0，使用默认值ADS8319_CHAIN_LENGTH */
    uint8_t adc_count = g_spi_adc_cnt[spi_idx];
    if (adc_count == 0)
    {
        adc_count = 8;  // ADS8319_CHAIN_LENGTH 默认值
    }
    uint32_t xfer_size = adc_count * 2;  // 2字节/ADC
    dma_set_spi_xfer_size(spi_idx, xfer_size);
    usb_printf("SPI%d: adc_count=%d, xfer_size=%d bytes\n", spi_idx, adc_count, xfer_size);
}
```

## 工作流程

### 1. USB接口配置通道使能

当上位机发送通道配置时：
1. 解析通道配置数据
2. 更新 `g_ch_enable_mask` 和 `g_enabled_chs`
3. **动态计算每个SPI总线的ADC数量**：
   ```c
   uint8_t spi_idx = ch_id % SPI_NUM;
   uint8_t adc_pos = ch_id / SPI_NUM;
   if ((adc_pos + 1) > g_spi_adc_cnt[spi_idx])
       g_spi_adc_cnt[spi_idx] = adc_pos + 1;
   ```

### 2. 更新DMA传输大小

配置完成后，立即调用 `dma_set_spi_xfer_size()` 更新每个SPI总线的传输大小：

```c
for (uint8_t spi_idx = 0; spi_idx < SPI_NUM; spi_idx++)
{
    uint8_t adc_count = g_spi_adc_cnt[spi_idx];
    if (adc_count == 0) adc_count = 8;  // 默认值
    uint32_t xfer_size = adc_count * 2;  // 2字节/ADC
    dma_set_spi_xfer_size(spi_idx, xfer_size);
}
```

### 3. 下次DMA传输使用新大小

下次调用 `dma_start_transfer_all()` 时，会使用更新后的传输大小：
1. `dma_get_xfer_size()` 返回新的传输大小
2. `dma_rebuild_lists()` 用新的大小重建DMA列表
3. `SPI_CR2_TSIZE` 寄存器设置为新的传输大小
4. DMA按新大小传输数据

## 示例场景

### 场景1：配置3个通道（SPI0: CH0, SPI1: CH1, SPI2: CH2）

1. **配置前**：
   - `g_spi_adc_cnt = [0, 0, 0]`
   - DMA传输大小：[16, 16, 16] 字节（默认值）

2. **配置后**：
   - `g_spi_adc_cnt = [1, 1, 1]`
   - DMA传输大小：[2, 2, 2] 字节（1个ADC × 2字节）

### 场景2：配置8个通道（SPI0: CH0-CH7）

1. **配置前**：
   - `g_spi_adc_cnt = [0, 0, 0]`
   - DMA传输大小：[16, 16, 16] 字节

2. **配置后**：
   - `g_spi_adc_cnt = [8, 0, 0]`
   - DMA传输大小：[16, 16, 16] 字节（8个ADC × 2字节 = 16字节）

### 场景3：混合配置（SPI0: CH0-CH2, SPI1: CH3-CH4, SPI2: CH5）

1. **配置前**：
   - `g_spi_adc_cnt = [0, 0, 0]`
   - DMA传输大小：[16, 16, 16] 字节

2. **配置后**：
   - `g_spi_adc_cnt = [3, 2, 1]`
   - DMA传输大小：[6, 4, 2] 字节

## 注意事项

1. **硬编码设置移除**：移除了 `usb_processor.c` 中的硬编码设置（`g_spi_adc_cnt[0] = 1` 等），确保使用动态计算的值。

2. **默认值处理**：如果某个SPI总线没有配置任何通道（`g_spi_adc_cnt[spi_idx] == 0`），使用默认值8个ADC。

3. **实时性**：DMA传输大小在配置完成后立即更新，下次传输时生效。

4. **缓冲区大小**：`adc_dma_buffer` 定义为 `[SPI_USED_MAX][DMA_SPI_RX_NODE_USED][ADS8319_CHAIN_LENGTH]`，足够容纳最大8个ADC的数据。

5. **调试信息**：添加了调试打印，显示每个SPI总线的ADC数量和传输大小。

## 验证方法

1. 通过USB配置不同的通道组合
2. 检查调试输出中的 `spi_adc_cnt` 和 `xfer_size`
3. 验证DMA传输的数据量是否正确
4. 检查接收的数据是否完整无错位

## 相关函数

- `dma_set_spi_xfer_size()` - 设置DMA传输大小
- `dma_get_xfer_size()` - 获取DMA传输大小
- `dma_start_transfer_all()` - 启动DMA传输
- `USB_CollectChCfg_Reply()` - USB通道配置处理
- `offline_processor.c` 中的配置函数 - 离线配置处理
