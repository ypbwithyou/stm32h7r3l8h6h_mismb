#include "./BSP/SPI/spi.h"

#ifndef ADS8922_H
#define ADS8922_H


// ADS8922菊花链配置
#define ADS8922_CHAIN_LENGTH            8      // 菊花链中ADC数量
#define ADS8922_DATA_BITS               22      // 每个ADC的数据位数
#define TOTAL_DATA_BITS                 (ADS8922_CHAIN_LENGTH * ADS8922_DATA_BITS)  // 总数据位数 = 176
#define TOTAL_DATA_BYTES                ((TOTAL_DATA_BITS + 7) / 8)  // 总字节数 = 22
#define RX_BUFFER_SIZE                   TOTAL_DATA_BYTES        // 接收缓冲区大小(字节)
#define TRANSFERS_PER_BUFFER            (RX_BUFFER_SIZE / TOTAL_DATA_BYTES)  // 每个缓冲区可容纳的传输次数


/* 共用引脚配置：CONVST(PA8)、RVS(PA10)*/
#define ADS8922B_CONVST_GPIO            GPIOA
#define ADS8922B_CONVST_PIN             GPIO_PIN_15
#define ADS8922B_CONVST_CLK_ENABLE()    do{ __HAL_RCC_GPIOD_CLK_ENABLE(); }while(0) 

#define ADS8922B_RVS_GPIO               GPIOA
#define ADS8922B_RVS_PIN                GPIO_PIN_0
#define ADS8922B_RVS_CLK_ENABLE()       do{ __HAL_RCC_GPIOA_CLK_ENABLE(); }while(0) 

#define ADS8922B_CONVST_LOW()     ADS8922B_CONVST_GPIO->BRR = (unsigned int)ADS8922B_CONVST_PIN;
#define ADS8922B_CONVST_HIGH()    ADS8922B_CONVST_GPIO->BSRR = (unsigned int)ADS8922B_CONVST_PIN;


/* CS引脚配置 */
#define SPI1_CS_GPIO_PORT               GPIOE
#define SPI1_CS_GPIO_PIN                GPIO_PIN_11 
#define SPI1_CS_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOE_CLK_ENABLE(); }while(0) 

#define SPI2_CS_GPIO_PORT               GPIOP 
#define SPI2_CS_GPIO_PIN                GPIO_PIN_11 
#define SPI2_CS_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOP_CLK_ENABLE(); }while(0) 

#define SPI3_CS_GPIO_PORT               GPIOP 
#define SPI3_CS_GPIO_PIN                GPIO_PIN_12 
#define SPI3_CS_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOP_CLK_ENABLE(); }while(0) 

// CS引脚直接寄存器操作（避免函数调用开销）
#define CS1_LOW()       SPI1_CS_GPIO_PORT->BRR = (unsigned int)SPI1_CS_GPIO_PIN;
#define CS2_LOW()       SPI2_CS_GPIO_PORT->BRR = (unsigned int)SPI2_CS_GPIO_PIN;
#define CS3_LOW()       SPI3_CS_GPIO_PORT->BRR = (unsigned int)SPI3_CS_GPIO_PIN;
#define CS1_HIGH()      SPI1_CS_GPIO_PORT->BSRR = (unsigned int)SPI1_CS_GPIO_PIN;
#define CS2_HIGH()      SPI2_CS_GPIO_PORT->BSRR = (unsigned int)SPI2_CS_GPIO_PIN;
#define CS3_HIGH()      SPI3_CS_GPIO_PORT->BSRR = (unsigned int)SPI3_CS_GPIO_PIN;

// 批量CS控制
#define ALL_CS_LOW()  do { \
    SPI1_CS_GPIO_PORT->BRR = (unsigned int)SPI1_CS_GPIO_PIN; \
    SPI2_CS_GPIO_PORT->BRR = (unsigned int)SPI2_CS_GPIO_PIN; \
    SPI3_CS_GPIO_PORT->BRR = (unsigned int)SPI3_CS_GPIO_PIN; \
} while(0)
#define ALL_CS_HIGH() do { \
    SPI1_CS_GPIO_PORT->BSRR = (unsigned int)SPI1_CS_GPIO_PIN; \
    SPI2_CS_GPIO_PORT->BSRR = (unsigned int)SPI2_CS_GPIO_PIN; \
    SPI3_CS_GPIO_PORT->BSRR = (unsigned int)SPI3_CS_GPIO_PIN; \
} while(0)

#define CS1(x)                 do { (x) ?                                                              \
                                    HAL_GPIO_WritePin(SPI1_CS_GPIO_PORT, SPI1_CS_GPIO_PIN, GPIO_PIN_SET):     \
                                    HAL_GPIO_WritePin(SPI1_CS_GPIO_PORT, SPI1_CS_GPIO_PIN, GPIO_PIN_RESET);   \
                                } while (0)


extern unsigned char spi_rx_buffer[SPI_USED_MAX][RX_BUFFER_SIZE];  // SPI接收缓冲区
extern unsigned char spi_tx_buffer[TOTAL_DATA_BYTES];  // SPI发送缓冲区

// 函数声明

void ads8922_spi_gpio_init(unsigned int spi_periph);
void ads8922_common_gpio_init(void);
void ads8922_start_convst(void);
void ads8922_stop_transfer(unsigned int spi_periph);
void ads8922_read_daisy_chain(unsigned int spi_periph, unsigned short *adc_data);
void process_adc_data(unsigned short *raw_data, float *converted_data);

void extract_adc_data_from_buffer(unsigned char *data_in, unsigned short *adc_data);


#endif /* ADS8922_H */
