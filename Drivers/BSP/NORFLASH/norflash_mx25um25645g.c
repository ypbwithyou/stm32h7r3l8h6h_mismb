/**
 ****************************************************************************************************
 * @file        norflash_mx25um25645g.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       NOR Flash MX25UM25645G驱动代码
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

#include "./BSP/NORFLASH/norflash_mx25um25645g.h"
#include "./SYSTEM/delay/delay.h"

/* MX25UM25645G SPI命令定义 */
#define MX25UM25645G_COMMAND_SPI_RSTEN  (0x66UL)
#define MX25UM25645G_COMMAND_SPI_RST    (0x99UL)
#define MX25UM25645G_COMMAND_SPI_RDSR   (0x05UL)
#define MX25UM25645G_COMMAND_SPI_RDID   (0x9FUL)
#define MX25UM25645G_COMMAND_SPI_RDCR2  (0x71UL)
#define MX25UM25645G_COMMAND_SPI_WREN   (0x06UL)
#define MX25UM25645G_COMMAND_SPI_WRCR2  (0x72UL)

/* MX25UM25645G OPI命令定义 */
#define MX25UM25645G_COMMAND_OPI_RSTEN  (0x6699UL)
#define MX25UM25645G_COMMAND_OPI_RST    (0x9966UL)
#define MX25UM25645G_COMMAND_OPI_RDCR2  (0x718EUL)
#define MX25UM25645G_COMMAND_OPI_WREN   (0x06F9UL)
#define MX25UM25645G_COMMAND_OPI_RDSR   (0x05FAUL)
#define MX25UM25645G_COMMAND_OPI_CE     (0x609FUL)
#define MX25UM25645G_COMMAND_OPI_BE4B   (0xDC23UL)
#define MX25UM25645G_COMMAND_OPI_SE4B   (0x21DEUL)
#define MX25UM25645G_COMMAND_OPI_PP4B   (0x12EDUL)
#define MX25UM25645G_COMMAND_OPI_8DTRD  (0xEE11UL)

/* MX25UM25645G ID定义 */
#define MX25UM25645G_IDENTIFICATION     (0x3980C2UL)

/* MX25UM25645G大小参数定义 */
#define MX25UM25645G_CHIP_SIZE          (0x02000000UL)
#define MX25UM25645G_BLOCK_SIZE         (0x00010000UL)
#define MX25UM25645G_SECTOR_SIZE        (0x00001000UL)
#define MX25UM25645G_PAGE_SIZE          (0x00000100UL)

/* MX25UM25645G擦后数据值定义 */
#define MX25UM25645G_EMPTY_VALUE        ((uint8_t)0xFF)

/**
 * @brief   软复位
 * @param   hxspi: XSPI句柄指针
 * @retval  复位结果
 * @arg     0: 复位成功
 * @arg     1: 复位失败
 */
static uint8_t mx25um25645g_software_reset(XSPI_HandleTypeDef *hxspi)
{
    XSPI_RegularCmdTypeDef xspi_regular_cmd_struct = {0};
    XSPI_AutoPollingTypeDef xspi_auto_polling_struct = {0};
    
    if (hxspi == NULL)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_SPI_RSTEN;
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
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_SPI_RST;
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
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_RSTEN;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
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
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_RST;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
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
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_RSTEN;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_NONE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_RST;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_NONE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    delay_ms(1);
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_SPI_RDSR;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_1_LINE;
    xspi_regular_cmd_struct.DataLength = 1;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_auto_polling_struct.MatchValue = 0UL << 0;
    xspi_auto_polling_struct.MatchMask = 1UL << 0;
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    return 0;
}

/**
 * @brief   校验ID
 * @param   hxspi: XSPI句柄指针
 * @retval  校验结果
 * @arg     0: 校验成功
 * @arg     1: 校验失败
 */
static uint8_t mx25um25645g_verify_identification(XSPI_HandleTypeDef *hxspi)
{
    XSPI_RegularCmdTypeDef xspi_regular_cmd_struct = {0};
    uint8_t data[4];
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_SPI_RDID;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_1_LINE;
    xspi_regular_cmd_struct.DataLength = 3;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    if (HAL_XSPI_Receive(hxspi, data, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    if (*(uint32_t *)data != MX25UM25645G_IDENTIFICATION)
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
static uint8_t mx25um25645g_configure_dtr_opi(XSPI_HandleTypeDef *hxspi)
{
    XSPI_RegularCmdTypeDef xspi_regular_cmd_struct = {0};
    XSPI_AutoPollingTypeDef xspi_auto_polling_struct = {0};
    uint8_t data[1];
    
    if (hxspi == NULL)
    {
        return HAL_ERROR;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_SPI_RDCR2;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.Address = 0x00000000;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_1_LINE;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_DISABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_1_LINE;
    xspi_regular_cmd_struct.DataLength = 1;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    if (HAL_XSPI_Receive(hxspi, data, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_SPI_WREN;
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
        return HAL_ERROR;
    }
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_SPI_RDSR;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_1_LINE;
    xspi_regular_cmd_struct.DataLength = 1;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    xspi_auto_polling_struct.MatchValue = 1UL << 1;
    xspi_auto_polling_struct.MatchMask = 1UL << 1;
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
  
    data[0] &= ~(3UL << 0);
    data[0] |= (2UL << 0);
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_SPI_WRCR2;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_1_LINE;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
    xspi_regular_cmd_struct.Address = 0x00000000;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_1_LINE;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_DISABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_1_LINE;
    xspi_regular_cmd_struct.DataLength = 1;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_DISABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    if (HAL_XSPI_Transmit(hxspi, data, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_RDCR2;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.Address = 0x00000000;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_ENABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_8_LINES;
    xspi_regular_cmd_struct.DataLength = 1;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 4;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    if (HAL_XSPI_Receive(hxspi, data, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return HAL_ERROR;
    }
    if ((data[0] & (3UL << 0)) != (2UL << 0))
    {
        return HAL_ERROR;
    }
    
    return HAL_OK;
}

/**
 * @brief   初始化
 * @param   hxspi: XSPI句柄指针
 * @retval  初始化结果
 * @arg     0: 初始化成功
 * @arg     1: 初始化失败
 */
static uint8_t mx25um25645g_init(XSPI_HandleTypeDef *hxspi)
{
    if (hxspi == NULL)
    {
        return 1;
    }
    
    /* 软复位 */
    if (mx25um25645g_software_reset(hxspi) != 0)
    {
        return 1;
    }
    
    /* 校验ID */
    if (mx25um25645g_verify_identification(hxspi) != 0)
    {
        return 1;
    }
    
    /* 配置DTR-OPI模式 */
    if (mx25um25645g_configure_dtr_opi(hxspi) != 0)
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
static uint8_t mx25um25645g_erase_chip(XSPI_HandleTypeDef *hxspi)
{
    XSPI_RegularCmdTypeDef xspi_regular_cmd_struct = {0};
    XSPI_AutoPollingTypeDef xspi_auto_polling_struct = {0};
    
    if (hxspi == NULL)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_WREN;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_NONE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_RDSR;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.Address = 0x00000000;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_ENABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_8_LINES;
    xspi_regular_cmd_struct.DataLength = 1;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 4;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_auto_polling_struct.MatchValue = 1UL << 1;
    xspi_auto_polling_struct.MatchMask = 1UL << 1;
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_CE;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_NONE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_RDSR;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.Address = 0x00000000;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_ENABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_8_LINES;
    xspi_regular_cmd_struct.DataLength = 1;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 4;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_auto_polling_struct.MatchValue = 0UL << 0;
    xspi_auto_polling_struct.MatchMask = 1UL << 0;
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, 150000UL) != HAL_OK)
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
static uint8_t mx25um25645g_erase_block(XSPI_HandleTypeDef *hxspi, uint32_t address)
{
    XSPI_RegularCmdTypeDef xspi_regular_cmd_struct = {0};
    XSPI_AutoPollingTypeDef xspi_auto_polling_struct = {0};
    
    if (hxspi == NULL)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_WREN;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_NONE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_RDSR;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.Address = 0x00000000;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_ENABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_8_LINES;
    xspi_regular_cmd_struct.DataLength = 1;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 4;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_auto_polling_struct.MatchValue = 1UL << 1;
    xspi_auto_polling_struct.MatchMask = 1UL << 1;
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_BE4B;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.Address = address;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_ENABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_NONE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_RDSR;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.Address = 0x00000000;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_ENABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_8_LINES;
    xspi_regular_cmd_struct.DataLength = 1;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 4;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_auto_polling_struct.MatchValue = 0UL << 0;
    xspi_auto_polling_struct.MatchMask = 1UL << 0;
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
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
static uint8_t mx25um25645g_erase_sector(XSPI_HandleTypeDef *hxspi, uint32_t address)
{
    XSPI_RegularCmdTypeDef xspi_regular_cmd_struct = {0};
    XSPI_AutoPollingTypeDef xspi_auto_polling_struct = {0};
    
    if (hxspi == NULL)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_WREN;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_NONE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_RDSR;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.Address = 0x00000000;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_ENABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_8_LINES;
    xspi_regular_cmd_struct.DataLength = 1;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 4;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_auto_polling_struct.MatchValue = 1UL << 1;
    xspi_auto_polling_struct.MatchMask = 1UL << 1;
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_SE4B;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.Address = address;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_ENABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_NONE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_RDSR;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.Address = 0x00000000;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_ENABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_8_LINES;
    xspi_regular_cmd_struct.DataLength = 1;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 4;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_auto_polling_struct.MatchValue = 0UL << 0;
    xspi_auto_polling_struct.MatchMask = 1UL << 0;
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
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
static uint8_t mx25um25645g_program_page(XSPI_HandleTypeDef *hxspi, uint32_t address, uint8_t *data, uint32_t length)
{
    XSPI_RegularCmdTypeDef xspi_regular_cmd_struct = {0};
    XSPI_AutoPollingTypeDef xspi_auto_polling_struct = {0};
    
    if (hxspi == NULL)
    {
        return 1;
    }
    
    if (length > MX25UM25645G_PAGE_SIZE)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_WREN;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_NONE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_RDSR;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.Address = 0x00000000;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_ENABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_8_LINES;
    xspi_regular_cmd_struct.DataLength = 1;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 4;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_auto_polling_struct.MatchValue = 1UL << 1;
    xspi_auto_polling_struct.MatchMask = 1UL << 1;
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_PP4B;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.Address = address;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_ENABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_8_LINES;
    xspi_regular_cmd_struct.DataLength = length;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_ENABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
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
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_RDSR;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.Address = 0x00000000;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_ENABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_8_LINES;
    xspi_regular_cmd_struct.DataLength = 1;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 4;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_auto_polling_struct.MatchValue = 0UL << 0;
    xspi_auto_polling_struct.MatchMask = 1UL << 0;
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
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
static uint8_t mx25um25645g_read(XSPI_HandleTypeDef *hxspi, uint32_t address, uint8_t *data, uint32_t length)
{
    XSPI_RegularCmdTypeDef xspi_regular_cmd_struct = {0};
    
    if (hxspi == NULL)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_8DTRD;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.Address = address;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_ENABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_8_LINES;
    xspi_regular_cmd_struct.DataLength = length;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_ENABLE;
    xspi_regular_cmd_struct.DummyCycles = 20;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
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
static uint8_t mx25um25645g_memory_mapped(XSPI_HandleTypeDef *hxspi)
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
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_WREN;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_NONE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_NONE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_RDSR;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.Address = 0x00000000;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_ENABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_8_LINES;
    xspi_regular_cmd_struct.DataLength = 1;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_DISABLE;
    xspi_regular_cmd_struct.DummyCycles = 4;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    xspi_auto_polling_struct.MatchValue = 1UL << 1;
    xspi_auto_polling_struct.MatchMask = 1UL << 1;
    xspi_auto_polling_struct.MatchMode = HAL_XSPI_MATCH_MODE_AND;
    xspi_auto_polling_struct.AutomaticStop = HAL_XSPI_AUTOMATIC_STOP_ENABLE;
    xspi_auto_polling_struct.IntervalTime = 0x10;
    if (HAL_XSPI_AutoPolling(hxspi, &xspi_auto_polling_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_READ_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_8DTRD;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_ENABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_8_LINES;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_ENABLE;
    xspi_regular_cmd_struct.DummyCycles = 20;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
    if (HAL_XSPI_Command(hxspi, &xspi_regular_cmd_struct, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK)
    {
        return 1;
    }
    
    xspi_regular_cmd_struct.OperationType = HAL_XSPI_OPTYPE_WRITE_CFG;
    xspi_regular_cmd_struct.IOSelect = HAL_XSPI_SELECT_IO_7_0;
    xspi_regular_cmd_struct.Instruction = MX25UM25645G_COMMAND_OPI_PP4B;
    xspi_regular_cmd_struct.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
    xspi_regular_cmd_struct.InstructionWidth = HAL_XSPI_INSTRUCTION_16_BITS;
    xspi_regular_cmd_struct.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_ENABLE;
    xspi_regular_cmd_struct.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
    xspi_regular_cmd_struct.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
    xspi_regular_cmd_struct.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_ENABLE;
    xspi_regular_cmd_struct.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
    xspi_regular_cmd_struct.DataMode = HAL_XSPI_DATA_8_LINES;
    xspi_regular_cmd_struct.DataDTRMode = HAL_XSPI_DATA_DTR_ENABLE;
    xspi_regular_cmd_struct.DummyCycles = 0;
    xspi_regular_cmd_struct.DQSMode = HAL_XSPI_DQS_ENABLE;
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

/* MX25UM25645G设备定义 */
const norflash_t norflash_mx25um25645g = {
    .type = NORFlash_MX25UM25645G,
    .parameter = {
        .empty_value = MX25UM25645G_EMPTY_VALUE,
        .chip_size = MX25UM25645G_CHIP_SIZE,
        .block_size = MX25UM25645G_BLOCK_SIZE,
        .sector_size = MX25UM25645G_SECTOR_SIZE,
        .page_size = MX25UM25645G_PAGE_SIZE,
    },
    .ops = {
        .init = mx25um25645g_init,
        .deinit = NULL,
        .erase_chip = mx25um25645g_erase_chip,
        .erase_block = mx25um25645g_erase_block,
        .erase_sector = mx25um25645g_erase_sector,
        .program_page = mx25um25645g_program_page,
        .read = mx25um25645g_read,
        .memory_mapped = mx25um25645g_memory_mapped,
    },
};
