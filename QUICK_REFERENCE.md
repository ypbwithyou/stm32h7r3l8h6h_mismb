# DMA SPI采集模块 - 快速参考

## 文件变更清单

| 文件 | 操作 | 说明 |
|------|------|------|
| `dma_spi_adc.h` | 新增 | DMA采集模块头文件 |
| `dma_spi_adc.c` | 新增 | DMA采集模块实现 |
| `dma_list.c` | 修改 | 添加DMA回调调用 |
| `gtim.c` | 修改 | 定时器使用DMA采集 |
| `ida_config.c` | 修改 | 初始化DMA模块 |

## 核心代码片段

### 1. 启动DMA采集
```c
// 在定时器中启动
ads8319_start_convst();     // 启动ADC转换
dma_adc_start_collect();    // 启动DMA传输
```

### 2. 读取数据
```c
// 获取DMA数据
uint16_t adc_data[8];
if (dma_adc_get_data(spi_idx, adc_data)) {
    // 处理数据...
}
```

### 3. DMA回调处理
```c
// 在dma_list.c的dma_transfer_complete_cb中
dma_adc_transfer_complete(spi_idx);
```

## 常见问题

### Q1: 编译错误 - 找不到头文件
**解决**: 确保在Keil MDK中添加了文件并设置包含路径

### Q2: 数据格式错误
**解决**: 检查字节序转换代码：
```c
adc_data[i] = (dma_buf[2*i] << 8) | dma_buf[2*i + 1];
```

### Q3: DMA不触发
**解决**: 
1. 检查SPI DMA使能位
2. 检查DMA中断优先级
3. 检查CONVST时序

## 调试技巧

### 1. 添加调试输出
```c
usb_printf("DMA RX count: %lu\r\n", dma_rx_callback_count[spi_idx]);
```

### 2. 检查缓冲区状态
```c
for (int i = 0; i < DMA_SPI_RX_NODE_USED; i++) {
    usb_printf("Buffer %d ready: %d\r\n", i, buffer_ready[spi_idx][i]);
}
```

### 3. 监控CPU占用
使用DWT计数器监控定时器中断执行时间。

## 性能测试

### 测试条件
- 定时器周期: 100us
- SPI时钟: 15MHz
- ADC数量: 8个/链
- 数据量: 16字节/链

### 预期性能
- CPU占用: < 10%
- 数据吞吐量: ~100KB/s
- 延迟: < 5us
