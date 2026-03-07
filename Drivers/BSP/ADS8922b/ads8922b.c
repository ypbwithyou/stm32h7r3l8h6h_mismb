
#include "./BSP/ADS8922B/ads8922b.h"
#include "./BSP/SPI/spi.h"
#include "string.h"
#include "./SYSTEM/delay/delay.h"
//#include "./BSP/DMA/dma.h"
#include "./BSP/DMA_LIST/dma_list.h"

extern DMA_HandleTypeDef g_dma_rx_handle;   /* DMA¾䱺 */

// 缓冲区管理变量
volatile unsigned char current_rx_buffer = 0;     // 当前正在填充的缓冲区
volatile unsigned char ready_rx_buffer = 0;       // 准备就绪等待处理的缓冲区
volatile unsigned int transfer_count = 0;       // 传输计数

// 全局缓冲区
unsigned char spi_rx_buffer[SPI_USED_MAX][RX_BUFFER_SIZE];  // SPI接收缓冲区
unsigned char spi_tx_buffer[TOTAL_DATA_BYTES];  // SPI发送缓冲区


void ads8922_common_gpio_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    ADS8922B_CONVST_CLK_ENABLE();
    ADS8922B_RVS_CLK_ENABLE();

    gpio_init_struct.Pin = ADS8922B_CONVST_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;                        /* 复用推挽输出 */
    gpio_init_struct.Pull = GPIO_PULLUP;                                /* 上拉 */
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;                 /* 高速 */
    HAL_GPIO_Init(ADS8922B_CONVST_GPIO, &gpio_init_struct);             /* 初始化CONVST引脚 */

    gpio_init_struct.Pin = ADS8922B_RVS_PIN;
    gpio_init_struct.Mode = GPIO_MODE_INPUT;                            /* 输入端 */
    HAL_GPIO_Init(ADS8922B_RVS_GPIO, &gpio_init_struct);                /* 初始化RVS引脚 */

    ADS8922B_CONVST_LOW();
}
void ads8922_spi_gpio_init(unsigned int spi_periph)
{
    spi_init(SPI1_SPI);
    // spi_init(SPI2_SPI);
    // spi_init(SPI3_SPI);
}

void ads8922_start_convst(void)
{
    // 拉高CONVST 启动转换
    ADS8922B_CONVST_HIGH();
    
    // 查询RVS，启动转换时RVS变低，转换完成时RVS变高
    while(HAL_GPIO_ReadPin(ADS8922B_RVS_GPIO, ADS8922B_RVS_PIN) == GPIO_PIN_RESET);
    
    // 拉低CONVST 结束转换
    ADS8922B_CONVST_LOW();
}

void ads8922_stop_transfer(unsigned int spi_periph)
{
    // 拉低CS开始读取
    switch (spi_periph)
    {
        case 1:
            CS1_HIGH();
            break;
        case 2:
            CS2_HIGH();
            break;
        case 3:
            CS3_HIGH();
            break;
    }
}

void ads8922_read_daisy_chain(unsigned int spi_periph, unsigned short *adc_data)
{
    // 清空缓冲区
    memset(spi_tx_buffer, 0x00, TOTAL_DATA_BYTES);
    memset(spi_rx_buffer, 0x00, TOTAL_DATA_BYTES);

    // 拉低CS 开启传输
    ALL_CS_LOW();
    
    // 启动DMA传输
    dma_start_transfer();
    
//    // 发送读取命令并接收菊花链数据
//    // 发送22字节(176位)空数据来生成时钟，同时接收数据
//    spi_read_write_byte(spi_periph, &spi_tx_buffer[0], &spi_rx_buffer[spi_periph - 1][0], TOTAL_DATA_BYTES);
//    
//    // 拉高CS结束传输
//    ads8922_stop_transfer(spi_periph);

////    // 处理接收到的数据，提取每个ADC的22位数据
//    extract_adc_data_from_buffer(&spi_rx_buffer[spi_periph - 1][0], adc_data);
}

/* 
* 从接收缓冲区提取每个ADC的22位数据，并移位计算出有效的16位数据
* 数据格式: 每个ADC输出22位 = 16位转换数据 + 6位状态信息
* 菊花链数据流: ADC7 -> ADC6 -> ... -> ADC0 (最后接收到的数据来自链首ADC0)
* 
* 缓冲区布局(22字节 = 176位):
* 字节0-2: ADC7的数据 (位0-21)
* 字节3-5: ADC6的数据 (位22-43) 
* ...
* 字节19-21: ADC0的数据 (位154-175)
*/
void extract_adc_data_from_buffer(unsigned char *data_in, unsigned short *adc_data)
{
    for (char i=0; i<ADS8922_CHAIN_LENGTH; i++) {
        unsigned char bit_max = 0;
        char index = 0;
        char bit_offset = 0;
        char byte_offset = 0;
        int adc_val = 0;
        bit_max = 16 * (i + 1) + 6 * i;
        bit_offset = bit_max % 8;
        if (bit_offset == 0) {
            index = bit_max/8;
            byte_offset = 2;
            adc_val |= data_in[index-byte_offset];
            adc_val = (adc_val<<8) | data_in[index-byte_offset+1];
        } else {
            index = bit_max/8 + 1;
            byte_offset = 3;
            adc_val |= data_in[index-byte_offset];
            adc_val = (adc_val<<8) | data_in[index-byte_offset+1];
            adc_val = (adc_val<<8) | data_in[index-byte_offset+2];
            adc_val <<= bit_offset;
            adc_val >>= 8;
        }
        adc_data[ADS8922_CHAIN_LENGTH - 1 - i] = adc_val;
    }
}

void process_adc_data(unsigned short *raw_data, float *converted_data)
{
    /*
     * 处理原始16位数据，转换为浮点型电压值
     */
    
    for(int i = 0; i < ADS8922_CHAIN_LENGTH; i++) {
        if (raw_data[i] & 0x8000) {
            converted_data[i] = - (0xffff - raw_data[i]) * 4.5 / 32767.0;
        } else {
            converted_data[i] = raw_data[i] * 4.5 / 32767.0;
        }
    }
}
 
static void sd8922b_chip_select_init(unsigned int spi_periph)
{
    GPIO_InitTypeDef gpio_init_struct = {0};
    
    switch (spi_periph)
    {
    case SPI1_SPI:
        SPI1_CS_GPIO_CLK_ENABLE();

        gpio_init_struct.Pin = SPI1_CS_GPIO_PIN;
        gpio_init_struct.Alternate = GPIO_MODE_OUTPUT_PP;                   /* ���� */
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(SPI1_CS_GPIO_PORT, &gpio_init_struct);                /* ��ʼ��CS���� */
        CS1_HIGH();
        break;
    case SPI2_SPI:
        SPI2_CS_GPIO_CLK_ENABLE();

        gpio_init_struct.Pin = SPI2_CS_GPIO_PIN;
        gpio_init_struct.Alternate = GPIO_MODE_OUTPUT_PP;                   /* ���� */
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(SPI2_CS_GPIO_PORT, &gpio_init_struct);                /* ��ʼ��CS���� */
        CS2_HIGH();
        break;
    case SPI3_SPI:
        SPI3_CS_GPIO_CLK_ENABLE();

        gpio_init_struct.Pin = SPI3_CS_GPIO_PIN;
        gpio_init_struct.Alternate = GPIO_MODE_OUTPUT_PP;                   /* ���� */
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        HAL_GPIO_Init(SPI3_CS_GPIO_PORT, &gpio_init_struct);                /* ��ʼ��CS���� */
        CS3_HIGH();
        break;
    default:
        break;
    }
}

