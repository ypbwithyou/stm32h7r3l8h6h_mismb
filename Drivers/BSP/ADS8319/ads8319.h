#include "./BSP/SPI/spi.h"

#ifndef ADS8319_H
#define ADS8319_H


// ADS8319菊花链配置
#define ADS8319_CHAIN_LENGTH                8       // 菊花链中ADC数量
#define ADS8319_DATA_BITS                   16      // 每个ADC的数据位数
#define TOTAL_DATA_BITS                     (ADS8319_CHAIN_LENGTH * ADS8319_DATA_BITS)  // 总数据位数 = 128
#define TOTAL_DATA_BYTES                    ((TOTAL_DATA_BITS + 7) / 8)                 // 总字节数 = 16
#define RX_BUFFER_SIZE                       TOTAL_DATA_BYTES                           // 接收缓冲区大小(字节)
#define TRANSFERS_PER_BUFFER                (RX_BUFFER_SIZE / TOTAL_DATA_BYTES)         // 每个缓冲区可容纳的传输次数


/* 共用引脚配置：CONVST(PB0)*/
#define ADS8319_CONVST_GPIO                 GPIOB
#define ADS8319_CONVST_PIN                  GPIO_PIN_0
#define ADS8319_CONVST_CLK_ENABLE()         do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0) 

#define ADS8319_CONVST_LOW()                ADS8319_CONVST_GPIO->BRR = (unsigned int)ADS8319_CONVST_PIN;
#define ADS8319_CONVST_HIGH()               ADS8319_CONVST_GPIO->BSRR = (unsigned int)ADS8319_CONVST_PIN;

#define ADS8319_1_IRQ_GPIO                  GPIOB
#define ADS8319_1_IRQ_PIN                   GPIO_PIN_6
#define ADS8319_1_IRQ_CLK_ENABLE()          do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0) 
#define ADS8319_2_IRQ_GPIO                  GPIOB
#define ADS8319_2_IRQ_PIN                   GPIO_PIN_7
#define ADS8319_2_IRQ_CLK_ENABLE()          do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0) 
#define ADS8319_3_IRQ_GPIO                  GPIOB
#define ADS8319_3_IRQ_PIN                   GPIO_PIN_8
#define ADS8319_3_IRQ_CLK_ENABLE()          do{ __HAL_RCC_GPIOB_CLK_ENABLE(); }while(0) 




extern unsigned char spi_rx_buffer[SPI_USED_MAX][RX_BUFFER_SIZE];  // SPI接收缓冲区
extern unsigned char spi_tx_buffer[TOTAL_DATA_BYTES];  // SPI发送缓冲区

// 函数声明

void ads8319_spi_gpio_init(unsigned int spi_periph);
void ads8319_common_gpio_init(void);
void ads8319_start_convst(void);
void ads8319_stop_transfer(void);
void ads8319_read_daisy_chain(unsigned int spi_periph, uint16_t *adc_data);
//void ads8319_read_daisy_chain(unsigned int spi_periph);
//void process_adc_data(unsigned short *raw_data, float *converted_data);

void extract_adc_data_from_buffer(unsigned char *data_in, unsigned short *adc_data);

void ads8319_read_daisy_chain_fast(uint32_t spi_periph, uint8_t adc_num, uint16_t *adc_data);


#endif /* ADS8319_H */
