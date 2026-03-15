# SPI DMA采集模块使用说明

## 概述

本模块将原有的CPU轮询方式SPI采集改为DMA方式，大幅降低CPU占用率。

## 文件说明

### 新增文件

1. **`Drivers/BSP/DMA_LIST/dma_spi_adc.h`** - DMA采集模块头文件
2. **`Drivers/BSP/DMA_LIST/dma_spi_adc.c`** - DMA采集模块实现

### 修改文件

1. **`Drivers/BSP/DMA_LIST/dma_list.c`**
   - 在 `dma_transfer_complete_cb()` 中添加 `dma_adc_transfer_complete(spi_idx)` 调用
   - 添加 `#include "./BSP/DMA_LIST/dma_spi_adc.h"`

2. **`Drivers/BSP/TIMER/gtim.c`**
   - 添加 `#include "./BSP/DMA_LIST/dma_spi_adc.h"`
   - 修改 `HAL_TIM_PeriodElapsedCallback()` 使用DMA采集

3. **`User/ida_config.c`**
   - 添加 `#include "./BSP/DMA_LIST/dma_spi_adc.h"`
   - 在 `IdaDeviceInit()` 中初始化DMA ADC模块

## API接口

### 初始化

```c
/**
 * @brief  DMA采集模块初始化
 * 在系统初始化时调用
 */
void dma_adc_init(void);
```

### 启动采集

```c
/**
 * @brief  启动DMA采集（在定时器中调用）
 */
void dma_adc_start_collect(void);
```

### 获取数据

```c
/**
 * @brief  获取最新就绪数据
 * @param  spi_idx: SPI索引 (0-2)
 * @param  adc_data: 输出ADC数据缓冲区 (16位数组)
 * @return 1: 成功, 0: 无数据
 */
uint8_t dma_adc_get_data(uint8_t spi_idx, uint16_t *adc_data);

/**
 * @brief  获取批量就绪数据（用于定时器处理）
 * @param  adc_buf: 输出缓冲区 [SPI_NUM][ADS8319_CHAIN_LENGTH]
 * @return 成功获取的SPI数量
 */
uint8_t dma_adc_get_batch_data(uint16_t adc_buf[SPI_NUM][ADS8319_CHAIN_LENGTH]);
```

### 状态查询

```c
/**
 * @brief  检查是否有就绪数据
 * @param  spi_idx: SPI索引
 * @return 1: 有数据, 0: 无数据
 */
uint8_t dma_adc_has_data(uint8_t spi_idx);
```

## 使用示例

### 定时器中断处理

```c
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    static uint8_t first_run = 1;
    
    if (htim->Instance != GTIM_TIMX)
        return;
    
    g_gtim_it_counts++;
    
    if (first_run)
    {
        // 第一次运行，仅启动转换和DMA
        first_run = 0;
        ads8319_start_convst();
        dma_adc_start_collect();
        return;
    }
    
    // 读取上一帧DMA数据
    uint16_t adc_buf[SPI_NUM][ADS8319_CHAIN_LENGTH];
    
    for (uint8_t spi = 0; spi < SPI_NUM; spi++)
    {
        if (g_spi_adc_cnt[spi] == 0)
            continue;
        
        if (dma_adc_get_data(spi, adc_buf[spi]))
        {
            // 处理数据...
        }
    }
    
    // 启动新一轮转换和DMA
    ads8319_start_convst();
    dma_adc_start_collect();
}
```

## 工作流程

```
定时器中断触发
    ↓
启动CONVST转换 (ads8319_start_convst)
    ↓
启动DMA传输 (dma_adc_start_collect)
    ↓
DMA异步传输数据到缓冲区
    ↓
DMA完成回调 (dma_transfer_complete_cb)
    ↓
转换数据格式并标记就绪 (dma_adc_transfer_complete)
    ↓
下一次定时器中断读取数据
```

## 缓冲区管理

- **DMA环形缓冲区**: 10个节点 × 3个SPI
- **ADC数据缓冲区**: 10个节点 × 3个SPI × 8个ADC通道
- **双缓冲机制**: 当前处理缓冲区和就绪缓冲区分离

## 性能对比

| 方式 | CPU占用 | 延迟 | 实现复杂度 |
|------|---------|------|-----------|
| CPU轮询 | 高 | 低 | 简单 |
| DMA | 低 | 略高 | 中等 |

## 配置参数

在 `dma_spi_adc.h` 中配置：

```c
#define SPI_USED_MAX                3       // SPI总线数量
#define DMA_SPI_RX_NODE_USED        10      // DMA接收节点数量
#define RX_BUFFER_SIZE              16      // 每个ADC的缓冲区大小（字节）
#define ADS8319_CHAIN_LENGTH        8       // 菊花链中ADC数量
#define SPI_NUM                     3       // SPI总线数量（与SPI_USED_MAX一致）
```

## 注意事项

1. **缓存一致性**: STM32H7需要处理数据缓存，已在代码中处理
2. **时序要求**: DMA传输需要在CONVST之后启动
3. **缓冲区大小**: 根据实际ADC数量调整
4. **中断优先级**: DMA中断优先级应高于定时器中断

## 编译配置

确保在Keil MDK-ARM项目中添加以下文件：

```
Drivers/BSP/DMA_LIST/dma_spi_adc.h
Drivers/BSP/DMA_LIST/dma_spi_adc.c
```

并在包含路径中添加：
```
Drivers/BSP/DMA_LIST
```
