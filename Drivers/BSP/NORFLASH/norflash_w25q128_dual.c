/**
 ****************************************************************************************************
 * @file        norflash_w25q128_dual.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       NOR Flash 双W25Q128驱动代码
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

#include "./BSP/NORFLASH/norflash_w25q128_dual.h"
#include "./SYSTEM/delay/delay.h"

/* W25Q128命令定义 */
#define W25Q128_COMMAND_ENABLE_RESET            (0x66UL)
#define W25Q128_COMMAND_RESET_DEVICE            (0x99UL)
#define W25Q128_COMMAND_MANUFACTURER_DEVICE_ID  (0x90UL)
#define W25Q128_COMMAND_READ_STATUS_REGISTER_2  (0x35UL)
#define W25Q128_COMMAND_WRITE_ENABLE            (0x06UL)
#define W25Q128_COMMAND_READ_STATUS_REGISTER_1  (0x05UL)
#define W25Q128_COMMAND_WRITE_STATUS_REGISTER_2 (0x31UL)
#define W25Q128_COMMAND_CHIP_ERASE              (0xC7UL)
#define W25Q128_COMMAND_BLOCK_ERASE             (0xD8UL)
#define W25Q128_COMMAND_SECTOR_ERASE            (0x20UL)
#define W25Q128_COMMAND_QUAD_INPUT_PAGE_PROGRAM (0x32UL)
#define W25Q128_COMMAND_FAST_READ_QUAD_IO       (0xEBUL)

/* W25Q128 ID定义 */
#define W25Q128_MANUFACTURER_DEVICE_ID          (0x17EFUL)
#define BY25FQ128_MANUFACTURER_DEVICE_ID        (0x1768UL)

/* W25Q128大小参数定义 */
#define W25Q128_CHIP_SIZE                       (0x01000000UL)
#define W25Q128_BLOCK_SIZE                      (0x00010000UL)
#define W25Q128_SECTOR_SIZE                     (0x00001000UL)
#define W25Q128_PAGE_SIZE                       (0x00000100UL)

/* W25Q128擦后数据值定义 */
#define W25Q128_EMPTY_VALUE                     ((uint8_t)0xFF)

/**
 * @brief   软复位
 * @param   hxspi: XSPI句柄指针
 * @retval  复位结果
 * @arg     0: 复位成功
 * @arg     1: 复位失败
 */
static uint8_t w25q128_dual_software_reset(XSPI_HandleTypeDef *hxspi)
{
    XSPI_RegularCmdTypeDef xspi_regular_cmd_struct = {0};
    
    if (hxspi == NULL)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_ENABLE_RESET;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_NONE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_RESET_DEVICE;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_NONE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    delay_ms(1);
    
    return 0;
}

/**
 * @brief   校验ID
 * @param   hxspi: XSPI句柄指针
 * @retval  校验结果
 * @arg     0: 校验成功
 * @arg     1: 校验失败
 */
static uint8_t w25q128_dual_verify_identification(XSPI_HandleTypeDef *hxspi)
{
    XSPI_RegularCmdTypeDef xspi_regular_cmd_struct = {0};
    uint8_t data[4];
    
    if (hxspi == NULL)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_MANUFACTURER_DEVICE_ID;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.Address = 0x000000UL;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_1_LINE;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_24_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_DISABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_1_LINE;
    xspi_regular_cmd_struct.DataLength = 2 * 2;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    if (HAL_XSPI_Receive(hxspi, data, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    if (((((uint16_t)data[0] | (data[2] << 8)) != W25Q128_MANUFACTURER_DEVICE_ID) || (((uint16_t)data[1] | (data[3] << 8)) != W25Q128_MANUFACTURER_DEVICE_ID)) && ((((uint16_t)data[0] | (data[2] << 8)) != BY25FQ128_MANUFACTURER_DEVICE_ID) || (((uint16_t)data[1] | (data[3] << 8)) != BY25FQ128_MANUFACTURER_DEVICE_ID)))
    {
        return 1;
    }
    
    return 0;
}

/**
 * @brief   配置Quad SPI模式
 * @param   hxspi: XSPI句柄指针
 * @retval  配置结果
 * @arg     0: 配置成功
 * @arg     1: 配置失败
 */
static uint8_t w25q128_dual_configure_quad(XSPI_HandleTypeDef *hxspi)
{
    XSPI_RegularCmdTypeDef xspi_regular_cmd_struct = {0};
    XSPI_AutoPollingTypeDef xspi_auto_polling_struct = {0};
    uint8_t data[2];
    
    if (hxspi == NULL)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_READ_STATUS_REGISTER_2;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_1_LINE;
    xspi_regular_cmd_struct.DataLength = 1 * 2;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    if (HAL_XSPI_Receive(hxspi, data, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_WRITE_ENABLE;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_NONE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_READ_STATUS_REGISTER_1;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_1_LINE;
    xspi_regular_cmd_struct.DataLength = 1 * 2;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_auto_polling_struct.MatchValue = (1UL << 1) | ((1UL << 1) << 8);
    xspi_auto_polling_struct.MatchMask = (1UL << 1) | ((1UL << 1) << 8);
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    *(uint16_t *)data &= ~((1UL << 1) | ((1UL << 1) << 8));
    *(uint16_t *)data |= (1UL << 1) | ((1UL << 1) << 8);
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_WRITE_STATUS_REGISTER_2;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_1_LINE;
    xspi_regular_cmd_struct.DataLength = 1 * 2;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    if (HAL_XSPI_Transmit(hxspi, data, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_READ_STATUS_REGISTER_1;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_1_LINE;
    xspi_regular_cmd_struct.DataLength = 1 * 2;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_auto_polling_struct.MatchValue = (0UL << 0) | ((0UL << 0) << 8);
    xspi_auto_polling_struct.MatchMask = (1UL << 0) | ((1UL << 0) << 8);
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_READ_STATUS_REGISTER_2;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_1_LINE;
    xspi_regular_cmd_struct.DataLength = 1 * 2;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    if (HAL_XSPI_Receive(hxspi, data, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    if ((*(uint16_t *)data & ((1UL << 1) | ((1UL << 1) << 8))) != ((1UL << 1) | ((1UL << 1) << 8)))
    {
        return 1;
    }
    
    return 0;
}
/**
 * @brief   配置DTR-OPI模式
 * @param   hxspi: XSPI句柄指针
 * @retval  配置结果
 * @arg     0: 配置成功
 * @arg     1: 配置失败
 */

/**
 * @brief   初始化
 * @param   hxspi: XSPI句柄指针
 * @retval  初始化结果
 * @arg     0: 初始化成功
 * @arg     1: 初始化失败
 */
static uint8_t w25q128_dual_init(XSPI_HandleTypeDef *hxspi)
{
    if (hxspi == NULL)
    {
        return 1;
    }
    
    /* 软复位 */
    if (w25q128_dual_software_reset(hxspi) != 0)
    {
        return 1;
    }
    
    /* 校验ID */
    if (w25q128_dual_verify_identification(hxspi) != 0)
    {
        return 1;
    }
    
    /* 配置Quad SPI模式 */
    if (w25q128_dual_configure_quad(hxspi) != 0)
    {
        return 1;
    }
    
    return 0;
}

/**
 * @brief   全片擦除
 * @param   hxspi: XSPI句柄指针
 * @retval  全片擦除结果
 * @arg     0: 全片擦除成功
 * @arg     1: 全片擦除失败
 */
static uint8_t w25q128_dual_erase_chip(XSPI_HandleTypeDef *hxspi)
{
    XSPI_RegularCmdTypeDef xspi_regular_cmd_struct = {0};
    XSPI_AutoPollingTypeDef xspi_auto_polling_struct = {0};
    
    if (hxspi == NULL)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_WRITE_ENABLE;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_NONE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_READ_STATUS_REGISTER_1;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_1_LINE;
    xspi_regular_cmd_struct.DataLength = 1 * 2;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_auto_polling_struct.MatchValue = (1UL << 1) | ((1UL << 1) << 8);
    xspi_auto_polling_struct.MatchMask = (1UL << 1) | ((1UL << 1) << 8);
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_CHIP_ERASE;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_NONE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_READ_STATUS_REGISTER_1;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_1_LINE;
    xspi_regular_cmd_struct.DataLength = 1 * 2;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_auto_polling_struct.MatchValue = (0UL << 0) | ((0UL << 0) << 8);
    xspi_auto_polling_struct.MatchMask = (1UL << 0) | ((1UL << 0) << 8);
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, 200000UL) != HAL_OK)
    {
        return 1;
    }
    
    return 0;
}

/**
 * @brief   块擦除
 * @param   hxspi: XSPI句柄指针
 * @param   address: 块地址
 * @retval  块擦除结果
 * @arg     0: 块擦除成功
 * @arg     1: 块擦除失败
 */
static uint8_t w25q128_dual_erase_block(XSPI_HandleTypeDef *hxspi, uint32_t address)
{
    XSPI_RegularCmdTypeDef xspi_regular_cmd_struct = {0};
    XSPI_AutoPollingTypeDef xspi_auto_polling_struct = {0};
    
    if (hxspi == NULL)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_WRITE_ENABLE;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_NONE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_READ_STATUS_REGISTER_1;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_1_LINE;
    xspi_regular_cmd_struct.DataLength = 1 * 2;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_auto_polling_struct.MatchValue = (1UL << 1) | ((1UL << 1) << 8);
    xspi_auto_polling_struct.MatchMask = (1UL << 1) | ((1UL << 1) << 8);
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_BLOCK_ERASE;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.Address = address;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_1_LINE;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_24_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_DISABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_NONE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_READ_STATUS_REGISTER_1;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_1_LINE;
    xspi_regular_cmd_struct.DataLength = 1 * 2;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_auto_polling_struct.MatchValue = (0UL << 0) | ((0UL << 0) << 8);
    xspi_auto_polling_struct.MatchMask = (1UL << 0) | ((1UL << 0) << 8);
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, 400UL) != HAL_OK)
    {
        return 1;
    }
    
    return 0;
}

/**
 * @brief   扇区擦除
 * @param   hxspi: XSPI句柄指针
 * @param   address: 扇区地址
 * @retval  扇区擦除结果
 * @arg     0: 扇区擦除成功
 * @arg     1: 扇区擦除失败
 */
static uint8_t w25q128_dual_erase_sector(XSPI_HandleTypeDef *hxspi, uint32_t address)
{
    XSPI_RegularCmdTypeDef xspi_regular_cmd_struct = {0};
    XSPI_AutoPollingTypeDef xspi_auto_polling_struct = {0};
    
    if (hxspi == NULL)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_WRITE_ENABLE;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_NONE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_READ_STATUS_REGISTER_1;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_1_LINE;
    xspi_regular_cmd_struct.DataLength = 1 * 2;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_auto_polling_struct.MatchValue = (1UL << 1) | ((1UL << 1) << 8);
    xspi_auto_polling_struct.MatchMask = (1UL << 1) | ((1UL << 1) << 8);
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_SECTOR_ERASE;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.Address = address;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_1_LINE;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_24_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_DISABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_NONE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_READ_STATUS_REGISTER_1;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_1_LINE;
    xspi_regular_cmd_struct.DataLength = 1 * 2;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_auto_polling_struct.MatchValue = (0UL << 0) | ((0UL << 0) << 8);
    xspi_auto_polling_struct.MatchMask = (1UL << 0) | ((1UL << 0) << 8);
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, 400UL) != HAL_OK)
    {
        return 1;
    }
    
    return 0;
}

/**
 * @brief   页编程
 * @param   hxspi: XSPI句柄指针
 * @param   address: 页地址
 * @param   data: 数据缓冲区指针
 * @param   length: 数据长度
 * @retval  页编程结果
 * @arg     0: 页编程成功
 * @arg     1: 页编程失败
 */
static uint8_t w25q128_dual_program_page(XSPI_HandleTypeDef *hxspi, uint32_t address, uint8_t *data, uint32_t length)
{
    XSPI_RegularCmdTypeDef xspi_regular_cmd_struct = {0};
    XSPI_AutoPollingTypeDef xspi_auto_polling_struct = {0};
    
    if (hxspi == NULL)
    {
        return 1;
    }
    
    if (length > (2 * W25Q128_PAGE_SIZE))
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_WRITE_ENABLE;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_NONE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_READ_STATUS_REGISTER_1;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_1_LINE;
    xspi_regular_cmd_struct.DataLength = 1 * 2;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_auto_polling_struct.MatchValue = (1UL << 1) | ((1UL << 1) << 8);
    xspi_auto_polling_struct.MatchMask = (1UL << 1) | ((1UL << 1) << 8);
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_QUAD_INPUT_PAGE_PROGRAM;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.Address = address;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_1_LINE;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_24_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_DISABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_4_LINES;
    xspi_regular_cmd_struct.DataLength = length;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    if (HAL_XSPI_Transmit(hxspi, data, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_READ_STATUS_REGISTER_1;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_1_LINE;
    xspi_regular_cmd_struct.DataLength = 1 * 2;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_auto_polling_struct.MatchValue = (0UL << 0) | ((0UL << 0) << 8);
    xspi_auto_polling_struct.MatchMask = (1UL << 0) | ((1UL << 0) << 8);
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, 3UL) != HAL_OK)
    {
        return 1;
    }
    
    return 0;
}

/**
 * @brief   读
 * @param   hxspi: XSPI句柄指针
 * @param   address: 地址
 * @param   data: 数据缓冲区指针
 * @param   length: 数据长度
 * @retval  读结果
 * @arg     0: 读成功
 * @arg     1: 读失败
 */
static uint8_t w25q128_dual_read(XSPI_HandleTypeDef *hxspi, uint32_t address, uint8_t *data, uint32_t length)
{
    XSPI_RegularCmdTypeDef xspi_regular_cmd_struct = {0};
    
    if (hxspi == NULL)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_FAST_READ_QUAD_IO;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.Address = address;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_4_LINES;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_24_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_DISABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_4_LINES;
    xspi_regular_cmd_struct.DataLength = length;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 6;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    if (HAL_XSPI_Receive(hxspi, data, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    return 0;
}

/**
 * @brief   内存映射
 * @param   hxspi: XSPI句柄指针
 * @retval  内存映射结果
 * @arg     0: 内存映射成功
 * @arg     1: 内存映射失败
 */
static uint8_t w25q128_dual_memory_mapped(XSPI_HandleTypeDef *hxspi)
{
    XSPI_RegularCmdTypeDef xspi_regular_cmd_struct = {0};
    XSPI_AutoPollingTypeDef xspi_auto_polling_struct = {0};
    XSPI_MemoryMappedTypeDef xspi_memory_mapped_struct = {0};
    
    if (hxspi == NULL)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_WRITE_ENABLE;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_NONE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_READ_STATUS_REGISTER_1;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_1_LINE;
    xspi_regular_cmd_struct.DataLength = 1 * 2;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_auto_polling_struct.MatchValue = (1UL << 1) | ((1UL << 1) << 8);
    xspi_auto_polling_struct.MatchMask = (1UL << 1) | ((1UL << 1) << 8);
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_READ_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_FAST_READ_QUAD_IO;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_4_LINES;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_24_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_DISABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_4_LINES;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 6;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_WRITE_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = W25Q128_COMMAND_QUAD_INPUT_PAGE_PROGRAM;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_1_LINE;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_24_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_DISABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_4_LINES;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_memory_mapped_struct.TimeOutActivation = HAL_XSPI_TIMEOUT_COUNTER_ENABLE;
    xspi_memory_mapped_struct.TimeoutPeriodClock = 0;
    if (HAL_XSPI_MemoryMapped(hxspi, &xspi_memory_mapped_struct) != HAL_OK)
    {
        return 1;
    }
    
    return 0;
}

/* 双W25Q128设备设备定义 */
const norflash_t norflash_w25q128_dual = {
    .type = NORFlash_W25Q128_Dual,
    .parameter = {
        .empty_value = W25Q128_EMPTY_VALUE,
        .chip_size = 2 * W25Q128_CHIP_SIZE,
        .block_size = 2 * W25Q128_BLOCK_SIZE,
        .sector_size = 2 * W25Q128_SECTOR_SIZE,
        .page_size = 2 * W25Q128_PAGE_SIZE,
    },
    .ops = {
        .init = w25q128_dual_init,
        .deinit = NULL,
        .erase_chip = w25q128_dual_erase_chip,
        .erase_block = w25q128_dual_erase_block,
        .erase_sector = w25q128_dual_erase_sector,
        .program_page = w25q128_dual_program_page,
        .read = w25q128_dual_read,
        .memory_mapped = w25q128_dual_memory_mapped,
    },
};
