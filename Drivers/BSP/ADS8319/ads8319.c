#include "./BSP/ADS8319/ads8319.h"
#include "./BSP/SPI/spi.h"
#include "string.h"
#include "./SYSTEM/delay/delay.h"

volatile unsigned char current_rx_buffer = 0;
volatile unsigned char ready_rx_buffer = 0;
volatile unsigned int transfer_count = 0;

unsigned char spi_rx_buffer[SPI_USED_MAX][RX_BUFFER_SIZE] __attribute__((aligned(32)));
unsigned char spi_tx_buffer[TOTAL_DATA_BYTES] __attribute__((aligned(32)));

void ads8319_common_gpio_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    ADS8319_CONVST_CLK_ENABLE();
    ADS8319_1_IRQ_CLK_ENABLE();
    ADS8319_2_IRQ_CLK_ENABLE();
    ADS8319_3_IRQ_CLK_ENABLE();

    gpio_init_struct.Pin = ADS8319_CONVST_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(ADS8319_CONVST_GPIO, &gpio_init_struct);

    gpio_init_struct.Pin = ADS8319_1_IRQ_PIN | ADS8319_2_IRQ_PIN | ADS8319_3_IRQ_PIN;
    gpio_init_struct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(ADS8319_1_IRQ_GPIO, &gpio_init_struct);

    ADS8319_CONVST_LOW();
}

void ads8319_spi_gpio_init(unsigned int spi_periph)
{
    spi_init(spi_periph);
}

void ads8319_start_convst(void)
{
    ADS8319_CONVST_HIGH();

    for (uint16_t i = 0; i < 900U; i++)
    {
        __NOP();
    }
}

void ads8319_stop_transfer(void)
{
    ADS8319_CONVST_LOW();
}

void ads8319_read_daisy_chain(unsigned int spi_periph, uint16_t *adc_data)
{
    uint8_t i;

    spi_read_write_byte(spi_periph, &spi_tx_buffer[0], &spi_rx_buffer[spi_periph - 1U][0], TOTAL_DATA_BYTES);
    for (i = 0U; i < ADS8319_CHAIN_LENGTH; i++)
    {
        adc_data[i] = spi_rx_buffer[spi_periph - 1U][2U * i];
        adc_data[i] = (uint16_t)((adc_data[i] << 8) | spi_rx_buffer[spi_periph - 1U][2U * i + 1U]);
    }
}

void ads8319_read_daisy_chain_fast(uint32_t spi_periph, uint8_t adc_num, uint16_t *adc_data)
{
    uint8_t i;
    uint8_t bytes = (uint8_t)((adc_num * ADS8319_DATA_BITS + 7U) / 8U);

    spi_read_write_byte_fast(spi_periph, &spi_tx_buffer[0], &spi_rx_buffer[spi_periph - 1U][0], bytes);
    for (i = 0U; i < adc_num; i++)
    {
        adc_data[i] = spi_rx_buffer[spi_periph - 1U][2U * i];
        adc_data[i] = (uint16_t)((adc_data[i] << 8) | spi_rx_buffer[spi_periph - 1U][2U * i + 1U]);
    }
}
