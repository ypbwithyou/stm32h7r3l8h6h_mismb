
#include "./BSP/ADS8319/ads8319.h"
#include "./BSP/SPI/spi.h"
#include "string.h"
#include "./SYSTEM/delay/delay.h"
//#include "./BSP/DMA/dma.h"
#include "./BSP/DMA_LIST/dma_list.h"

extern DMA_HandleTypeDef g_dma_rx_handle;   /* DMA句柄 */

// 缓冲区管理变量
volatile unsigned char current_rx_buffer = 0;     // 当前正在填充的缓冲区
volatile unsigned char ready_rx_buffer = 0;       // 准备就绪等待处理的缓冲区
volatile unsigned int transfer_count = 0;       // 传输计数

// 全局缓冲区
unsigned char spi_rx_buffer[SPI_USED_MAX][RX_BUFFER_SIZE];  // SPI接收缓冲区
unsigned char spi_tx_buffer[TOTAL_DATA_BYTES];  // SPI发送缓冲区


void ads8319_common_gpio_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    ADS8319_CONVST_CLK_ENABLE();
    ADS8319_1_IRQ_CLK_ENABLE();
    ADS8319_2_IRQ_CLK_ENABLE();
    ADS8319_3_IRQ_CLK_ENABLE();

    gpio_init_struct.Pin = ADS8319_CONVST_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;                        /* 复用推挽输出 */
    gpio_init_struct.Pull = GPIO_PULLUP;                                /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;                 /* 高速 */
    HAL_GPIO_Init(ADS8319_CONVST_GPIO, &gpio_init_struct);              /* 初始化CONVST引脚 */

    gpio_init_struct.Pin = ADS8319_1_IRQ_PIN | ADS8319_2_IRQ_PIN | ADS8319_3_IRQ_PIN;
    gpio_init_struct.Mode = GPIO_MODE_INPUT;                            /* 输入端 */
    HAL_GPIO_Init(ADS8319_1_IRQ_GPIO, &gpio_init_struct);               /* 初始化IRQ引脚 */

    ADS8319_CONVST_LOW();
}
void ads8319_spi_gpio_init(unsigned int spi_periph)
{
    spi_init(spi_periph);
}

void ads8319_start_convst(void)
{
    // while (HAL_GPIO_ReadPin(SPI1_SCK_GPIO_PORT, SPI1_SCK_GPIO_PIN) == GPIO_PIN_RESET);
    // while (HAL_GPIO_ReadPin(SPI2_SCK_GPIO_PORT, SPI2_SCK_GPIO_PIN) == GPIO_PIN_RESET);
    // while (HAL_GPIO_ReadPin(SPI3_SCK_GPIO_PORT, SPI3_SCK_GPIO_PIN) == GPIO_PIN_RESET);
    // __NOP();
    // __NOP();
    // __NOP();
    
    // 拉高CONVST 启动转换
    ADS8319_CONVST_HIGH();
    
    // while (HAL_GPIO_ReadPin(ADS8319_1_IRQ_GPIO, ADS8319_1_IRQ_PIN) == GPIO_PIN_RESET);
    // while (HAL_GPIO_ReadPin(ADS8319_2_IRQ_GPIO, ADS8319_2_IRQ_PIN) == GPIO_PIN_RESET);
    // while (HAL_GPIO_ReadPin(ADS8319_3_IRQ_GPIO, ADS8319_3_IRQ_PIN) == GPIO_PIN_RESET);
    
   for (uint16_t i=0; i<1000; i++)
   {
       __NOP();
   }
}

void ads8319_stop_transfer(void)
{
    ADS8319_CONVST_LOW();
    __NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();
    __NOP();__NOP();__NOP();__NOP();
}

void ads8319_read_daisy_chain(unsigned int spi_periph, uint16_t *adc_data)
{
    uint8_t i = 0;
//    // 启动DMA传输
//    dma_start_transfer();

    // 发送读取命令并接收菊花链数据
    // 发送空数据来生成时钟，同时接收数据
    spi_read_write_byte(spi_periph, &spi_tx_buffer[0], &spi_rx_buffer[spi_periph - 1][0], TOTAL_DATA_BYTES);
    for (i=0; i<ADS8319_CHAIN_LENGTH; i++) {
        adc_data[i] = spi_rx_buffer[spi_periph - 1][0+2*i];
        adc_data[i] = ((adc_data[i]<<8) | spi_rx_buffer[spi_periph - 1][1+2*i]);
    }
}
/**
 * @brief   快速读取菊花链数据（使用寄存器版本SPI函数）
 * @param   spi_periph: SPI外设编号
 * @param   adc_data: ADC数据缓冲区
 */
void ads8319_read_daisy_chain_fast(uint32_t spi_periph, uint8_t adc_num, uint16_t *adc_data)
{
    uint8_t i = 0;
    // 直接使用快速SPI读取函数
    spi_read_write_byte_fast(spi_periph, &spi_tx_buffer[0], &spi_rx_buffer[spi_periph - 1][0], (adc_num * ADS8319_DATA_BITS + 7) / 8);
    for (i=0; i<adc_num; i++) {
        adc_data[i] = spi_rx_buffer[spi_periph - 1][0+2*i];
        adc_data[i] = ((adc_data[i]<<8) | spi_rx_buffer[spi_periph - 1][1+2*i]);
    }
}

///**
// * @brief   优化的读取菊花链函数（兼容原有接口）
// */
//void ads8319_read_daisy_chain(unsigned int spi_periph, uint16_t *adc_data)
//{
//    // 调用快速版本
//    ads8319_read_daisy_chain_fast(spi_periph, adc_data);
//}

