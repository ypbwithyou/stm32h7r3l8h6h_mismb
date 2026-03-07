/**
 ****************************************************************************************************
 * @file        hyperram.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       HyperRAM驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 * 
 * 实验平台:正点原子 H7R3开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 * 
 ****************************************************************************************************
 */

#include "./BSP/HYPERRAM/hyperram.h"

#define HYPERRAM_IDENTIFICATION_REGISTER_0  ((0x00000000UL) << 1)
#define HYPERRAM_IDENTIFICATION_REGISTER_1  ((0x00000001UL) << 1)
#define HYPERRAM_CONFIGURATION_REGISTER_0   ((0x00000800UL) << 1)
#define HYPERRAM_CONFIGURATION_REGISTER_1   ((0x00000801UL) << 1)

#define HYPERRAM_IDENTIFICATION_0_POS_MANUFACTURER  (0)
#define HYPERRAM_IDENTIFICATION_0_POS_COL_COUNT     (8)
#define HYPERRAM_IDENTIFICATION_0_POS_ROW_COUNT     (4)
#define HYPERRAM_IDENTIFICATION_1_POS_DEVICE_TYPE   (0)

#define HYPERRAM_IDENTIFICATION_0_MASK_MANUFACTURER ((uint16_t)0x000F << HYPERRAM_IDENTIFICATION_0_POS_MANUFACTURER)
#define HYPERRAM_IDENTIFICATION_0_MASK_COL_COUNT    ((uint16_t)0x000F << HYPERRAM_IDENTIFICATION_0_POS_COL_COUNT)
#define HYPERRAM_IDENTIFICATION_0_MASK_ROW_COUNT    ((uint16_t)0x001F << HYPERRAM_IDENTIFICATION_0_POS_ROW_COUNT)
#define HYPERRAM_IDENTIFICATION_1_MASK_DEVICE_TYPE  ((uint16_t)0x000F << HYPERRAM_IDENTIFICATION_1_POS_DEVICE_TYPE)

#define HYPERRAM_IDENTIFICATION_0_MANUFACTURER      ((uint16_t)0x0006)
#define HYPERRAM_IDENTIFICATION_1_DEVICE_TYPE       ((uint16_t)0x0001)

static XSPI_HandleTypeDef g_xspi_handle = {0};
static uint32_t hyperram_size = 0;

/**
 * @brief   硬件复位HyperRAM
 * @param   无
 * @retval  无
 */
static void hyperram_hardware_reset(void)
{
    HAL_GPIO_WritePin(GPION, GPIO_PIN_12, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(GPION, GPIO_PIN_12, GPIO_PIN_SET);
    HAL_Delay(1);
}

/**
 * @brief   读HyperRAM寄存器
 * @param   address: 寄存器地址
 * @param   data: 数据缓冲区指针
 * @retval  读取结果
 * @arg     0: 读取成功
 * @arg     1: 读取失败
 */
static uint8_t hyperram_read_register(uint32_t address, uint16_t *data)
{
    XSPI_HyperbusCmdTypeDef xspi_hyperbus_cmd_struct = {0};
    
    xspi_hyperbus_cmd_struct.AddressSpace = HAL_XSPI_REGISTER_ADDRESS_SPACE;
    xspi_hyperbus_cmd_struct.Address = address;
    xspi_hyperbus_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    xspi_hyperbus_cmd_struct.DataLength = 2;
    xspi_hyperbus_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    xspi_hyperbus_cmd_struct.DataMode = HAL_XSPI_DATA_8_LINES;
    if (HAL_XSPI_HyperbusCmd(&g_xspi_handle, &xspi_hyperbus_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    if (HAL_XSPI_Receive(&g_xspi_handle, (uint8_t *)data, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    return 0;
}

/**
 * @brief   写HyperRAM寄存器
 * @param   address: 寄存器地址
 * @param   data: 数据
 * @retval  写入结果
 * @arg     0: 写入成功
 * @arg     1: 写入失败
 */
static uint8_t hyperram_write_register(uint32_t address, uint16_t data)
{
    XSPI_HyperbusCmdTypeDef xspi_hyperbus_cmd_struct = {0};
    
    xspi_hyperbus_cmd_struct.AddressSpace = HAL_XSPI_REGISTER_ADDRESS_SPACE;
    xspi_hyperbus_cmd_struct.Address = address;
    xspi_hyperbus_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    xspi_hyperbus_cmd_struct.DataLength = 2;
    xspi_hyperbus_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    xspi_hyperbus_cmd_struct.DataMode = HAL_XSPI_DATA_8_LINES;
    if (HAL_XSPI_HyperbusCmd(&g_xspi_handle, &xspi_hyperbus_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    if (HAL_XSPI_Transmit(&g_xspi_handle, (uint8_t *)&data, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    return 0;
}

/**
 * @brief   检查HyperRAM的ID
 * @param   无
 * @retval  检查结果
 * @arg     0: 检查成功
 * @arg     1: 检查失败
 */
static uint8_t hyperram_check_identification(void)
{
    uint16_t data;
    
    if (hyperram_read_register(HYPERRAM_IDENTIFICATION_REGISTER_1, &data) != 0)
    {
        return 1;
    }
    
    if (((data & HYPERRAM_IDENTIFICATION_1_MASK_DEVICE_TYPE) >> HYPERRAM_IDENTIFICATION_1_POS_DEVICE_TYPE) != HYPERRAM_IDENTIFICATION_1_DEVICE_TYPE)
    {
        return 1;
    }
    
    if (hyperram_read_register(HYPERRAM_IDENTIFICATION_REGISTER_0, &data) != 0)
    {
        return 1;
    }
    
    if (((data & HYPERRAM_IDENTIFICATION_0_MASK_MANUFACTURER) >> HYPERRAM_IDENTIFICATION_0_POS_MANUFACTURER) != HYPERRAM_IDENTIFICATION_0_MANUFACTURER)
    {
        return 1;
    }
    
    hyperram_size = 1UL << (((((data & HYPERRAM_IDENTIFICATION_0_MASK_COL_COUNT) >> HYPERRAM_IDENTIFICATION_0_POS_COL_COUNT) + 1) + (((data & HYPERRAM_IDENTIFICATION_0_MASK_ROW_COUNT) >> HYPERRAM_IDENTIFICATION_0_POS_ROW_COUNT) + 1)) + 1);
    
    return 0;
}

/**
 * @brief   配置HyperRAM差分时钟模式
 * @param   无
 * @retval  配置结果
 * @arg     0: 配置成功
 * @arg     1: 配置失败
 */
static uint8_t hyperram_config_differential_clock(void)
{
    uint16_t data;
    
    if (hyperram_read_register(HYPERRAM_CONFIGURATION_REGISTER_1, &data) != 0)
    {
        return 1;
    }
    
    data &= ~(1UL << 6);
    if (hyperram_write_register(HYPERRAM_CONFIGURATION_REGISTER_1, data) != 0)
    {
        return 1;
    }
    
    return 0;
}

/**
 * @brief   配置HyperRAM内存映射
 * @param   无
 * @retval  配置结果
 * @arg     0: 配置成功
 * @arg     1: 配置失败
 */
static uint8_t hyperram_memory_mapped(void)
{
    XSPI_HyperbusCmdTypeDef xspi_hyperbus_cmd_struct = {0};
    XSPI_MemoryMappedTypeDef xspi_memory_mapped_struct = {0};
    
    xspi_hyperbus_cmd_struct.AddressSpace = HAL_XSPI_MEMORY_ADDRESS_SPACE;
    xspi_hyperbus_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    xspi_hyperbus_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    xspi_hyperbus_cmd_struct.DataMode = HAL_XSPI_DATA_8_LINES;
    if (HAL_XSPI_HyperbusCmd(&g_xspi_handle, &xspi_hyperbus_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_memory_mapped_struct.TimeOutActivation = HAL_XSPI_TIMEOUT_COUNTER_ENABLE;
    xspi_memory_mapped_struct.TimeoutPeriodClock = 0;
    if (HAL_XSPI_MemoryMapped(&g_xspi_handle, &xspi_memory_mapped_struct) != HAL_OK)
    {
        return 1;
    }
    
    return 0;
}

/**
 * @brief   初始化HyperRAM
 * @param   无
 * @retval  初始化结果
 * @arg     0: 初始化成功
 * @arg     1: 初始化失败
 */
uint8_t hyperram_init(void)
{
    RCC_PeriphCLKInitTypeDef rcc_periph_clk_init_struct = {0};
    GPIO_InitTypeDef gpio_init_struct = {0};
    XSPIM_CfgTypeDef xspim_cfg_struct = {0};
    XSPI_HyperbusCfgTypeDef xspi_hyperbus_cfg_struct = {0};
    
    /* 配置XSPI时钟源 */
    rcc_periph_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_XSPI2;
    rcc_periph_clk_init_struct.Xspi2ClockSelection = RCC_XSPI2CLKSOURCE_PLL2S;
    if (HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init_struct) != HAL_OK)
    {
        return 1;
    }
    
    /* 使能时钟 */
      __HAL_RCC_XSPIM_CLK_ENABLE();
    __HAL_RCC_XSPI2_CLK_ENABLE();
    __HAL_RCC_GPION_CLK_ENABLE();
    
    /* 初始化XSPIM引脚 */
    gpio_init_struct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull = GPIO_NOPULL;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_struct.Alternate = GPIO_AF9_XSPIM_P2;
    HAL_GPIO_Init(GPION, &gpio_init_struct);
    
    /* 初始化复位引脚 */
    gpio_init_struct.Pin = GPIO_PIN_12;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPION, &gpio_init_struct);
    
    /* 初始化XSPI */
    g_xspi_handle.Instance = XSPI2;
    g_xspi_handle.Init.FifoThresholdByte = 4;
    g_xspi_handle.Init.MemoryMode = HAL_XSPI_SINGLE_MEM;
    g_xspi_handle.Init.MemoryType = HAL_XSPI_MEMTYPE_HYPERBUS;
    g_xspi_handle.Init.MemorySize = HAL_XSPI_SIZE_256MB;
    g_xspi_handle.Init.ChipSelectHighTimeCycle = 2;
    g_xspi_handle.Init.FreeRunningClock = HAL_XSPI_FREERUNCLK_DISABLE;
    g_xspi_handle.Init.ClockMode = HAL_XSPI_CLOCK_MODE_0;
    g_xspi_handle.Init.WrapSize = HAL_XSPI_WRAP_32_BYTES;
    g_xspi_handle.Init.ClockPrescaler = 2 - 1;
    g_xspi_handle.Init.SampleShifting = HAL_XSPI_SAMPLE_SHIFT_NONE;
    g_xspi_handle.Init.DelayHoldQuarterCycle = HAL_XSPI_DHQC_DISABLE;
    g_xspi_handle.Init.ChipSelectBoundary = HAL_XSPI_BONDARYOF_NONE;
    g_xspi_handle.Init.MaxTran = 0;
    g_xspi_handle.Init.Refresh = 0;
    g_xspi_handle.Init.MemorySelect = HAL_XSPI_CSSEL_NCS1;
    if (HAL_XSPI_Init(&g_xspi_handle) != HAL_OK)
    {
        return 1;
    }
    
    /* 配置XSPIM */
    xspim_cfg_struct.nCSOverride = HAL_XSPI_CSSEL_OVR_NCS1;
    xspim_cfg_struct.IOPort = HAL_XSPIM_IOPORT_2;
    if (HAL_XSPIM_Config(&g_xspi_handle, &xspim_cfg_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    /* 配置HyperBus参数 */
    xspi_hyperbus_cfg_struct.RWRecoveryTimeCycle = 7;
    xspi_hyperbus_cfg_struct.AccessTimeCycle = 7;
    xspi_hyperbus_cfg_struct.WriteZeroLatency = HAL_XSPI_LATENCY_ON_WRITE;
    xspi_hyperbus_cfg_struct.LatencyMode = HAL_XSPI_FIXED_LATENCY;
    if (HAL_XSPI_HyperbusCfg(&g_xspi_handle, &xspi_hyperbus_cfg_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    /* 硬件复位HyperRAM */
    hyperram_hardware_reset();
    
    /* 检查HyperRAM的ID */
    if (hyperram_check_identification() != 0)
    {
        return 1;
    }
    
    /* 配置HyperRAM差分时钟模式 */
    if (hyperram_config_differential_clock() != 0)
    {
        return 1;
    }
    
    /* 配置HyperRAM内存映射 */
    if (hyperram_memory_mapped() != 0)
    {
        return 1;
    }
    
    return 0;
}

/**
 * @brief   获取HyperRAM容量
 * @param   无
 * @retval  HyperRAM容量
 */
uint32_t hyperram_get_size(void)
{
    return hyperram_size;
}
