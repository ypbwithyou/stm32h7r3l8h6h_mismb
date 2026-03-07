/**
 ****************************************************************************************************
 * @file        norflash.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       NOR Flash驱动代码
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

#include "./BSP/NORFLASH/norflash.h"
#include "./BSP/NORFLASH/norflash_mx25um25645g.h"
#include "./BSP/NORFLASH/norflash_w25q128_dual.h"

/* 使用的NOR Flash设备指针定义 */
static const norflash_t *norflash = NULL;

/* 支持的NOR Flash设备指针列表定义 */
static const norflash_t *norflashs[] = {
#ifdef NORFLASH_SUPPORT_MX25UM25645G
    &norflash_mx25um25645g,
#endif /* NORFLASH_SUPPORT_MX25UM25645G */
#ifdef NORFLASH_SUPPORT_W25Q128_DUAL
    &norflash_w25q128_dual,
#endif /* NORFLASH_SUPPORT_W25Q128_DUAL */
};

/* XSPI句柄定义 */
static XSPI_HandleTypeDef xspi1_handle = {0};

/* NOR Flash扇区缓冲区定义 */
static uint8_t norflash_sector_buffer[NORFLASH_SECTOR_BUFFER_SIZE / sizeof(uint8_t)];

/**
 * @brief   初始化XSPI1
 * @param   hxspi: XSPI句柄指针
 * @param   type: NOR Flash设备类型（参考norflash.h文件中的norflash_type_t定义）
 * @retval  初始化结果
 * @arg     0: 初始化成功
 * @arg     1: 初始化失败
 */
static uint8_t norflash_xspi1_init(XSPI_HandleTypeDef *hxspi, norflash_type_t type)
{
    __IO uint8_t *ptr;
    uint32_t i;
    XSPIM_CfgTypeDef xspim_cfg_struct = {0};
    
    if (hxspi == NULL)
    {
        return 1;
    }
    
    if ((type == NORFlash_Unknow) || (type >= NORFlash_Dummy))
    {
        return 1;
    }
    
    ptr = (__IO uint8_t *)hxspi;
    for (i = 0; i < sizeof(XSPI_HandleTypeDef); i++)
    {
        *ptr++ = 0x00;
    }
    
    /* 反初始化XSPI */
    hxspi->Instance = XSPI1;
    if (HAL_XSPI_DeInit(hxspi) != HAL_OK)
    {
        return 1;
    }
    
    /* 初始化XSPI */
    hxspi->Instance = XSPI1;
    hxspi->Init.FifoThresholdByte = 4;
    if (0)
    {
        
    }
#ifdef NORFLASH_SUPPORT_MX25UM25645G
    else if (type == NORFlash_MX25UM25645G)
    {
        hxspi->Init.MemoryMode = HAL_XSPI_SINGLE_MEM;
        hxspi->Init.MemoryType = HAL_XSPI_MEMTYPE_MACRONIX;
    }
#endif /* NORFLASH_SUPPORT_MX25UM25645G */
#ifdef NORFLASH_SUPPORT_W25Q128_DUAL
    else if (type == NORFlash_W25Q128_Dual)
    {
        hxspi->Init.MemoryMode = HAL_XSPI_DUAL_MEM;
        hxspi->Init.MemoryType = HAL_XSPI_MEMTYPE_APMEM;
    }
#endif /* NORFLASH_SUPPORT_W25Q128_DUAL */
    hxspi->Init.MemorySize = HAL_XSPI_SIZE_256MB;
    hxspi->Init.ChipSelectHighTimeCycle = 1;
    hxspi->Init.FreeRunningClock = HAL_XSPI_FREERUNCLK_DISABLE;
    hxspi->Init.ClockMode = HAL_XSPI_CLOCK_MODE_0;
    hxspi->Init.WrapSize = HAL_XSPI_WRAP_NOT_SUPPORTED;
    hxspi->Init.ClockPrescaler = 4 - 1;
    hxspi->Init.SampleShifting = HAL_XSPI_SAMPLE_SHIFT_NONE;
    hxspi->Init.DelayHoldQuarterCycle = HAL_XSPI_DHQC_DISABLE;
    hxspi->Init.ChipSelectBoundary = HAL_XSPI_BONDARYOF_NONE;
    hxspi->Init.MaxTran = 0;
    hxspi->Init.Refresh = 0;
    hxspi->Init.MemorySelect = HAL_XSPI_CSSEL_NCS1;
    if (HAL_XSPI_Init(hxspi) != HAL_OK)
    {
        return 1;
    }
    
    /* 配置XSPIM */
    xspim_cfg_struct.nCSOverride = HAL_XSPI_CSSEL_OVR_NCS1;
    xspim_cfg_struct.IOPort = HAL_XSPIM_IOPORT_1;
    if (HAL_XSPIM_Config(hxspi, &xspim_cfg_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    return 0;
}

/**
 * @brief   反初始化XSPI1
 * @param   hxspi: XSPI句柄指针
 * @retval  反初始化结果
 * @arg     0: 反初始化成功
 * @arg     1: 反初始化失败
 */
static uint8_t norflash_xspi1_deinit(XSPI_HandleTypeDef *hxspi)
{
    if (HAL_XSPI_DeInit(hxspi) != HAL_OK)
    {
        return 1;
    }
    
    return 0;
}

#ifndef __SYS_H
/**
 * @brief   HAL库XSPI初始化回调函数
 * @param   hxspi: XSPI句柄指针
 * @retval  无
 */
void HAL_XSPI_MspInit(XSPI_HandleTypeDef* hxspi)
{
    RCC_PeriphCLKInitTypeDef rcc_periph_clk_init_struct = {0};
    GPIO_InitTypeDef gpio_init_struct = {0};
    
    if (hxspi->Instance == XSPI1)
    {
        /* 配置时钟源 */
        rcc_periph_clk_init_struct.PeriphClockSelection = RCC_PERIPHCLK_XSPI1;
        rcc_periph_clk_init_struct.Xspi1ClockSelection = RCC_XSPI1CLKSOURCE_PLL2S;
        HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init_struct);
        
        /* 使能时钟 */
        __HAL_RCC_XSPIM_CLK_ENABLE();
        __HAL_RCC_XSPI1_CLK_ENABLE();
        __HAL_RCC_GPIOP_CLK_ENABLE();
        __HAL_RCC_GPIOO_CLK_ENABLE();
        
        /* 初始化通讯引脚 */
        gpio_init_struct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_NOPULL;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init_struct.Alternate = GPIO_AF9_XSPIM_P1;
        HAL_GPIO_Init(GPIOP, &gpio_init_struct);
        gpio_init_struct.Pin = GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_4;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_NOPULL;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init_struct.Alternate = GPIO_AF9_XSPIM_P1;
        HAL_GPIO_Init(GPIOO, &gpio_init_struct);
    }
}

/**
 * @brief   HAL库XSPI反初始化回调函数
 * @param   hxspi: XSPI句柄指针
 * @retval  无
 */
void HAL_XSPI_MspDeInit(XSPI_HandleTypeDef* hxspi)
{
    if (hxspi->Instance == XSPI1)
    {
        /* 禁用时钟 */
        __HAL_RCC_XSPIM_CLK_DISABLE();
        __HAL_RCC_XSPI1_CLK_DISABLE();
        
        /* 反初始化通讯引脚 */
        HAL_GPIO_DeInit(GPIOP, GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
        HAL_GPIO_DeInit(GPIOO, GPIO_PIN_0 | GPIO_PIN_2 | GPIO_PIN_4);
    }
}
#endif /* __SYS_H */

/**
 * @brief   初始化NOR Flash
 * @param   无
 * @retval  NOR Flash设备类型（参考norflash.h文件中的norflash_type_t定义）
 */
norflash_type_t norflash_init(void)
{
    uint8_t norflash_index;
    
    if (norflash != NULL)
    {
        if (norflash_deinit() != 0)
        {
            return 1;
        }
    }
    
    for (norflash_index = 0; norflash_index < (sizeof(norflashs) / sizeof(norflashs[0])); norflash_index++)
    {
        /* 初始化XSPI1 */
        if (norflash_xspi1_init(&xspi1_handle, norflashs[norflash_index]->type) != 0)
        {
            break;
        }
        
        if (norflashs[norflash_index]->ops.init == NULL)
        {
            continue;
        }
        
        /* 初始化NOR Flash设备 */
        if (norflashs[norflash_index]->ops.init(&xspi1_handle) == 0)
        {
            if (norflashs[norflash_index]->type == NORFlash_MX25UM25645G)
            {
                /* MX25UM25645G最高支持200MHz时钟 */
                HAL_XSPI_SetClockPrescaler(&xspi1_handle, 2 - 1);
            }
            norflash = norflashs[norflash_index];
            return norflash->type;
        }
    }
    
    return NORFlash_Unknow;
}

/**
 * @brief   反初始化NOR Flash
 * @param   无
 * @retval  反初始化结果
 * @arg     0: 反初始化成功
 * @arg     1: 反初始化失败
 */
uint8_t norflash_deinit(void)
{
    if (norflash == NULL)
    {
        return 1;
    }
    
    /* 反初始化NOR Flash设备 */
    if (norflash->ops.deinit != NULL)
    {
        if (norflash->ops.deinit(&xspi1_handle) != 0)
        {
            return 1;
        }
    }
    
    /* 反初始化XSPI1 */
    if (norflash_xspi1_deinit(&xspi1_handle) != 0)
    {
        return 1;
    }
    
    norflash = NULL;
    
    return 0;
}

/**
 * @brief   全片擦除NOR Flash
 * @param   无
 * @retval  全片擦除结果
 * @arg     0: 全片擦除成功
 * @arg     1: 全片擦除失败
 */
uint8_t norflash_erase_chip(void)
{
    if (norflash == NULL)
    {
        return 1;
    }
    
    if (xspi1_handle.State == HAL_XSPI_STATE_BUSY_MEM_MAPPED)
    {
        return 1;
    }
    
    /* 全片擦除NOR Flash设备 */
    if (norflash->ops.erase_chip != NULL)
    {
        if (norflash->ops.erase_chip(&xspi1_handle) == 0)
        {
            return 0;
        }
    }
    
    return 1;
}

/**
 * @brief   块擦除NOR Flash
 * @param   address: 块地址
 * @retval  块擦除结果
 * @arg     0: 块擦除成功
 * @arg     1: 块擦除失败
 */
uint8_t norflash_erase_block(uint32_t address)
{
    if (norflash == NULL)
    {
        return 1;
    }
    
    if (xspi1_handle.State == HAL_XSPI_STATE_BUSY_MEM_MAPPED)
    {
        return 1;
    }
    
    /* 块擦除NOR Flash设备 */
    if (norflash->ops.erase_block != NULL)
    {
        if (norflash->ops.erase_block(&xspi1_handle, address) == 0)
        {
            return 0;
        }
    }
    
    return 1;
}

/**
 * @brief   扇区擦除NOR Flash
 * @param   address: 扇区地址
 * @retval  扇区擦除结果
 * @arg     0: 扇区擦除成功
 * @arg     1: 扇区擦除失败
 */
uint8_t norflash_erase_sector(uint32_t address)
{
    if (norflash == NULL)
    {
        return 1;
    }
    
    if (xspi1_handle.State == HAL_XSPI_STATE_BUSY_MEM_MAPPED)
    {
        return 1;
    }
    
    /* 扇区擦除NOR Flash设备 */
    if (norflash->ops.erase_sector != NULL)
    {
        if (norflash->ops.erase_sector(&xspi1_handle, address) == 0)
        {
            return 0;
        }
    }
    
    return 1;
}

/**
 * @brief   页编程NOR Flash
 * @param   address: 页地址
 * @param   data: 数据缓冲区指针
 * @param   length: 数据长度
 * @retval  页编程结果
 * @arg     0: 页编程成功
 * @arg     1: 页编程失败
 */
uint8_t norflash_program_page(uint32_t address, uint8_t *data, uint32_t length)
{
    if (norflash == NULL)
    {
        return 1;
    }
    
    if (xspi1_handle.State == HAL_XSPI_STATE_BUSY_MEM_MAPPED)
    {
        return 1;
    }
    
    if (length > norflash->parameter.page_size)
    {
        return 1;
    }
    
    /* 页编程NOR Flash设备 */
    if (norflash->ops.program_page != NULL)
    {
        if (norflash->ops.program_page(&xspi1_handle, address, data, length) == 0)
        {
            return 0;
        }
    }
    
    return 1;
}

/**
 * @brief   读NOR Flash
 * @param   address: 地址
 * @param   data: 数据缓冲区指针
 * @param   length: 数据长度
 * @retval  读结果
 * @arg     0: 读成功
 * @arg     1: 读失败
 */
uint8_t norflash_read(uint32_t address, uint8_t *data, uint32_t length)
{
    uint32_t index;
    uint32_t length_16b;
    
    if (norflash == NULL)
    {
        return 1;
    }
    
    /* 内存映射状态下直接从映射地址读取 */
    if (xspi1_handle.State == HAL_XSPI_STATE_BUSY_MEM_MAPPED)
    {
        length_16b = length >> 1;
        for (index = 0; index < length_16b; index++)
        {
            ((uint16_t *)data)[index] = ((volatile uint16_t *)(NORFLASH_MEMORY_MAPPED_BASE + address))[index];
        }
        
        if (length & 1UL)
        {
            data[length - 1] = *(volatile uint8_t *)(NORFLASH_MEMORY_MAPPED_BASE + address + length - 1);
        }
        
        return 0;
    }
    
    /* 读NOR Flash设备 */
    if (norflash->ops.read != NULL)
    {
        if (norflash->ops.read(&xspi1_handle, address, data, length) == 0)
        {
            return 0;
        }
    }
    
    return 1;
}

/**
 * @brief   开启NOR Flash内存映射
 * @param   无
 * @retval  开启内存映射结果
 * @arg     0: 开启内存映射成功
 * @arg     1: 开启内存映射失败
 */
uint8_t norflash_memory_mapped(void)
{
    if (norflash == NULL)
    {
        return 1;
    }
    
    if (xspi1_handle.State == HAL_XSPI_STATE_BUSY_MEM_MAPPED)
    {
        return 0;
    }
    
    /* 开启NOR Flash设备内存映射 */
    if (norflash->ops.memory_mapped != NULL)
    {
        if (norflash->ops.memory_mapped(&xspi1_handle) == 0)
        {
            return 0;
        }
    }
    
    return 1;
}

/**
 * @brief   获取NOR Flash擦后数据值
 * @param   无
 * @retval  NOR Flash擦后数据值
 */
uint8_t norflash_get_empty_value(void)
{
    if (norflash == NULL)
    {
        return 0;
    }
    
    return norflash->parameter.empty_value;
}

/**
 * @brief   获取NOR Flash片大小
 * @param   无
 * @retval  NOR Flash片大小
 */
uint32_t norflash_get_chip_size(void)
{
    if (norflash == NULL)
    {
        return 0;
    }
    
    return norflash->parameter.chip_size;
}

/**
 * @brief   获取NOR Flash块大小
 * @param   无
 * @retval  NOR Flash块大小
 */
uint32_t norflash_get_block_size(void)
{
    if (norflash == NULL)
    {
        return 0;
    }
    
    return norflash->parameter.block_size;
}

/**
 * @brief   获取NOR Flash扇区大小
 * @param   无
 * @retval  NOR Flash扇区大小
 */
uint32_t norflash_get_sector_size(void)
{
    if (norflash == NULL)
    {
        return 0;
    }
    
    return norflash->parameter.sector_size;
}

/**
 * @brief   获取NOR Flash页大小
 * @param   无
 * @retval  NOR Flash页大小
 */
uint32_t norflash_get_page_size(void)
{
    if (norflash == NULL)
    {
        return 0;
    }
    
    return norflash->parameter.page_size;
}

/**
 * @brief   写NOR Flash
 * @param   address: 地址
 * @param   data: 数据缓冲区指针
 * @param   length: 数据长度
 * @retval  写结果
 * @arg     0: 写成功
 * @arg     1: 写失败
 */
uint8_t norflash_write(uint32_t address, uint8_t *data, uint32_t length)
{
    uint32_t sector_size;
    uint32_t sectors_index;
    uint32_t sector_offset;
    uint32_t sector_remain;
    uint32_t sector_write_length;
    uint32_t data_index;
    uint32_t page_size;
    uint32_t pages_index;
    uint32_t page_offset;
    uint32_t page_remain;
    uint32_t page_write_length;
    
    if (norflash == NULL)
    {
        return 1;
    }
    
    if (xspi1_handle.State == HAL_XSPI_STATE_BUSY_MEM_MAPPED)
    {
        return 1;
    }
    
    sector_size = norflash_get_sector_size();
    page_size = norflash_get_page_size();
    
    /* NOR Flash扇区缓冲区大小校验 */
    if (sector_size > (sizeof(norflash_sector_buffer) / sizeof(norflash_sector_buffer[0])))
    {
        return 1;
    }
    
    sectors_index = address / sector_size;
    sector_offset = address % sector_size;
    while (length != 0)
    {
        sector_remain = sector_size - sector_offset;
        sector_write_length = (length < sector_remain) ? length : sector_remain;
        length -= sector_write_length;
        
        /* 读取NOR Flash扇区数据 */
        if(norflash_read(sectors_index * sector_size, norflash_sector_buffer, sector_size) != 0)
        {
            return 1;
        }
        
        /* NOR Flash扇区待写入位置数据空校验 */
        for (data_index = 0; data_index < sector_write_length; data_index++)
        {
            if (norflash_sector_buffer[sector_offset + data_index] != norflash_get_empty_value())
            {
                break;
            }
        }
        
        /* NOR Flash扇区待写入位置有非空数据 */
        if (data_index < sector_write_length)
        {
            /* 擦除NOR Flash扇区 */
            if (norflash_erase_sector(sectors_index * sector_size) != 0)
            {
                return 1;
            }
            
            /* 待写入数据写入NOR Flash扇区缓冲区 */
            for (data_index = 0; data_index < sector_write_length; data_index++)
            {
                norflash_sector_buffer[sector_offset + data_index] = *data;
                data++;
            }
            
            /* NOR Flash扇区缓冲区数据写入NOR Flash */
            for (pages_index = 0; pages_index < (sector_size / page_size); pages_index++)
            {
                if (norflash_program_page(sectors_index * sector_size + pages_index * page_size, &norflash_sector_buffer[pages_index * page_size], page_size) != 0)
                {
                    return 1;
                }
            }
        }
        /* NOR Flash扇区待写入位置无非空数据 */
        else
        {
            pages_index = sector_offset / page_size;
            page_offset = sector_offset % page_size;
            while (sector_write_length != 0)
            {
                page_remain = page_size - page_offset;
                page_write_length = (sector_write_length < page_remain) ? sector_write_length : page_remain;
                sector_write_length -= page_write_length;
                
                /* 待写入数据写入NOR Flash */
                if (norflash_program_page(sectors_index * sector_size + pages_index * page_size + page_offset, data, page_write_length) != 0)
                {
                    return 1;
                }
                data += page_write_length;
                
                pages_index++;
                page_offset = 0;
            }
        }
        
        sectors_index++;
        sector_offset = 0;
    }
    
    return 0;
}
