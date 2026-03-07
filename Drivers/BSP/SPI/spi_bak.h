/**
 ****************************************************************************************************
 * @file        spi.h
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       SPI ��������
 * @license     Copyright (c) 2020-2032, �������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� H7R3������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
 * 
 ****************************************************************************************************
 */

#ifndef __SPI_H
#define __SPI_H

#include "./SYSTEM/sys/sys.h"


extern SPI_HandleTypeDef g_spi_handle[3];  /* SPI句柄 */

/******************************************************************************************/
#define SPI_USED_MAX                    3

/* SPI1相关定义 */ 
#define SPI1_SPIx                       SPI1
#define SPI1_SPI                        1 
#define SPI1_SPI_CLK_ENABLE()           do{ __HAL_RCC_SPI1_CLK_ENABLE(); }while(0)
/* SPI1 引脚 定义 */ 
#define SPI1_SCK_GPIO_PORT              GPIOG 
#define SPI1_SCK_GPIO_PIN               GPIO_PIN_11 
#define SPI1_SCK_GPIO_AF                GPIO_AF5_SPI1
#define SPI1_SCK_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOG_CLK_ENABLE();}while(0) 
 
#define SPI1_MISO_GPIO_PORT             GPIOB 
#define SPI1_MISO_GPIO_PIN              GPIO_PIN_4 
#define SPI1_MISO_GPIO_AF               GPIO_AF5_SPI1
#define SPI1_MISO_GPIO_CLK_ENABLE()     do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0) 
 
#define SPI1_MOSI_GPIO_PORT             GPIOB 
#define SPI1_MOSI_GPIO_PIN              GPIO_PIN_5 
#define SPI1_MOSI_GPIO_AF               GPIO_AF5_SPI1
#define SPI1_MOSI_GPIO_CLK_ENABLE()     do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0) 


/* SPI2相关定义 */ 
#define SPI2_SPIx                       SPI2
#define SPI2_SPI                        2 
#define SPI2_SPI_CLK_ENABLE()           do{ __HAL_RCC_SPI2_CLK_ENABLE(); }while(0)
/* SPI2 引脚 定义 */ 
#define SPI2_SCK_GPIO_PORT              GPIOD 
#define SPI2_SCK_GPIO_PIN               GPIO_PIN_3 
#define SPI2_SCK_GPIO_AF                GPIO_AF5_SPI2
#define SPI2_SCK_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOD_CLK_ENABLE();}while(0) 
 
#define SPI2_MISO_GPIO_PORT             GPIOC 
#define SPI2_MISO_GPIO_PIN              GPIO_PIN_2 
#define SPI2_MISO_GPIO_AF               GPIO_AF5_SPI2
#define SPI2_MISO_GPIO_CLK_ENABLE()     do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0) 
 
#define SPI2_MOSI_GPIO_PORT             GPIOC 
#define SPI2_MOSI_GPIO_PIN              GPIO_PIN_1 
#define SPI2_MOSI_GPIO_AF               GPIO_AF5_SPI2
#define SPI2_MOSI_GPIO_CLK_ENABLE()     do{ __HAL_RCC_GPIOC_CLK_ENABLE(); }while(0) 


/* SPI3相关定义 */ 
#define SPI3_SPIx                       SPI4
#define SPI3_SPI                        3 
#define SPI3_SPI_CLK_ENABLE()           do{ __HAL_RCC_SPI4_CLK_ENABLE(); }while(0)
/* SPI3 引脚 定义 */ 
#define SPI3_SCK_GPIO_PORT              GPIOE 
#define SPI3_SCK_GPIO_PIN               GPIO_PIN_2 
#define SPI3_SCK_GPIO_AF                GPIO_AF5_SPI4
#define SPI3_SCK_GPIO_CLK_ENABLE()      do{ __HAL_RCC_GPIOE_CLK_ENABLE();}while(0) 
 
#define SPI3_MISO_GPIO_PORT             GPIOE 
#define SPI3_MISO_GPIO_PIN              GPIO_PIN_13 
#define SPI3_MISO_GPIO_AF               GPIO_AF5_SPI4
#define SPI3_MISO_GPIO_CLK_ENABLE()     do{ __HAL_RCC_GPIOE_CLK_ENABLE(); }while(0) 
 
#define SPI3_MOSI_GPIO_PORT             GPIOE 
#define SPI3_MOSI_GPIO_PIN              GPIO_PIN_14 
#define SPI3_MOSI_GPIO_AF               GPIO_AF5_SPI4
#define SPI3_MOSI_GPIO_CLK_ENABLE()     do{ __HAL_RCC_GPIOE_CLK_ENABLE(); }while(0) 

/*********************************************************************************************************/

void spi_init(unsigned int spi_periph);
unsigned char spi_read_write_byte(unsigned int spi_periph, unsigned char* txdata, unsigned char* rxdata, unsigned char size);
void spi_set_speed(unsigned int spi_periph, unsigned int speed);

#endif
























