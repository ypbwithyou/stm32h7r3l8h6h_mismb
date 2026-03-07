/**
 ****************************************************************************************************
 * @file        spi.c
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

#include "./BSP/SPI/spi.h"
#include "./BSP/SDNAND/spi_sdnand.h"
#include "./BSP/TIMER/gtim.h"
#include "./BSP/DMA_LIST/dma_list.h"

SPI_HandleTypeDef g_spi_handle[3];  /* SPI��� */

/**
 * @brief       SPI��ʼ������
 *   @note      ����ģʽ,8λ����,��ֹӲ��Ƭѡ
 * @param       ��
 * @retval      ��
 */
void spi_init(unsigned int spi_periph)
{
    char i = 0;

    // sd8922b_chip_select_init(spi_periph);
    switch (spi_periph)
    {
    case SPI1_SPI:
        i = 0;
        g_spi_handle[i].Instance = SPI1_SPIx;  
        break;
    case SPI2_SPI:
        i = 1;
        g_spi_handle[i].Instance = SPI2_SPIx;  
        break;
    case SPI3_SPI:
        i = 2;
        g_spi_handle[i].Instance = SPI3_SPIx;  
        break;
    default:
        break;
    }
    // g_spi_handle[i].Instance = spi_periph;                                     /* SP2 */
    g_spi_handle[i].Init.Mode = SPI_MODE_MASTER;                               /* ����SPI����ģʽ������Ϊ��ģʽ */
    g_spi_handle[i].Init.Direction = SPI_DIRECTION_2LINES;                     /* ����SPI�������˫�������ģʽ:SPI����Ϊ˫��ģʽ */
    g_spi_handle[i].Init.DataSize = SPI_DATASIZE_8BIT;                         /* ����SPI�����ݴ�С:SPI���ͽ���8λ֡�ṹ */
    g_spi_handle[i].Init.CLKPolarity = SPI_POLARITY_HIGH;                       /* ����ͬ��ʱ�ӵĿ���״̬Ϊ�͵�ƽ */
    g_spi_handle[i].Init.CLKPhase = SPI_PHASE_2EDGE;                           /* ����ͬ��ʱ�ӵĵ�һ�������أ��������½������ݱ����� */
    g_spi_handle[i].Init.NSS = SPI_NSS_SOFT;                                   /* NSS�ź���Ӳ����NSS�ܽţ�����������ʹ��SSIλ������:�ڲ�NSS�ź���SSIλ���� */
    g_spi_handle[i].Init.NSSPMode = SPI_NSS_PULSE_DISABLE;                     /* NSS�ź�����ʧ�� */
    g_spi_handle[i].Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;  /* SPI��ģʽIO״̬����ʹ�� */
    g_spi_handle[i].Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;          /* ���岨����Ԥ��Ƶ��ֵ:������Ԥ��ƵֵΪ256 */
    g_spi_handle[i].Init.FirstBit = SPI_FIRSTBIT_MSB;                          /* ָ�����ݴ����MSBλ����LSBλ��ʼ:���ݴ����MSBλ��ʼ */
    g_spi_handle[i].Init.TIMode = SPI_TIMODE_DISABLE;                          /* �ر�TIģʽ */
    g_spi_handle[i].Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;          /* �ر�Ӳ��CRCУ�� */
    g_spi_handle[i].Init.CRCPolynomial = 7;                                    /* CRCֵ����Ķ���ʽ */

        // 添加FIFO阈值配置（默认为0，即SPI_FIFO_THRESHOLD_01DATA，表示每传输一个数据就触发一次中断或者DMA）
//    g_spi_handle[i].Init.FifoThreshold = SPI_FIFO_THRESHOLD_02DATA;
    
    HAL_SPI_Init(&g_spi_handle[i]);
    
    // 屏蔽使能SPI传输，等DMA准备好后再使能，确保SPI传输与DMA传输之间的同步问题
    __HAL_SPI_ENABLE(&g_spi_handle[i]);                                        /* ʹ��SPI2 */
}

/**
 * @brief       SPI�ײ�������ʱ��ʹ�ܣ���������
 * @param       hspi:SPI���
 * @note        �˺����ᱻHAL_SPI_Init()����
 * @retval      ��
 */
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef gpio_init_struct = {0};
    RCC_PeriphCLKInitTypeDef rcc_periph_clk_init = {0};

    if (hspi->Instance == SPI1_SPIx) {
        SPI1_SPI_CLK_ENABLE();                                              /* SPI1ʱ��ʹ�� */
        SPI1_SCK_GPIO_CLK_ENABLE();                                         /* SPI1_SCK��ʱ��ʹ�� */
        SPI1_MISO_GPIO_CLK_ENABLE();                                        /* SPI1_MISO��ʱ��ʹ�� */
        SPI1_MOSI_GPIO_CLK_ENABLE();                                        /* SPI1_MOSI��ʱ��ʹ�� */

        /* ����SPI1��ʱ��Դ */
        rcc_periph_clk_init.PeriphClockSelection = RCC_PERIPHCLK_SPI1;      /* ����SPI1ʱ��Դ */
        rcc_periph_clk_init.Spi1ClockSelection = RCC_SPI1CLKSOURCE_PLL1Q;   /* SPI1ʱ��Դʹ��PLL1Q */
        HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init);

        gpio_init_struct.Pin = SPI1_SCK_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;                            /* ����������� */
        gpio_init_struct.Pull = GPIO_PULLUP;                                /* ���� */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;                 /* ���� */
        gpio_init_struct.Alternate = SPI1_SCK_GPIO_AF;                      /* ���� */
        HAL_GPIO_Init(SPI1_SCK_GPIO_PORT, &gpio_init_struct);               /* ��ʼ��SCK���� */

        gpio_init_struct.Pin = SPI1_MISO_GPIO_PIN;
        gpio_init_struct.Alternate = SPI1_MISO_GPIO_AF;                     /* ���� */
        HAL_GPIO_Init(SPI1_MISO_GPIO_PORT, &gpio_init_struct);              /* ��ʼ��MISO���� */
        
        gpio_init_struct.Pin = SPI1_MOSI_GPIO_PIN;
        gpio_init_struct.Alternate = SPI1_MOSI_GPIO_AF;                     /* ���� */
        HAL_GPIO_Init(SPI1_MOSI_GPIO_PORT, &gpio_init_struct);              /* ��ʼ��MOSI���� */

    } else if (hspi->Instance == SPI2_SPIx)
    {
        SPI2_SPI_CLK_ENABLE();                                              /* SPI2ʱ��ʹ�� */
        SPI2_SCK_GPIO_CLK_ENABLE();                                         /* SPI2_SCK��ʱ��ʹ�� */
        SPI2_MISO_GPIO_CLK_ENABLE();                                        /* SPI2_MISO��ʱ��ʹ�� */
        SPI2_MOSI_GPIO_CLK_ENABLE();                                        /* SPI2_MOSI��ʱ��ʹ�� */

        /* ����SPI2��ʱ��Դ */
        rcc_periph_clk_init.PeriphClockSelection = RCC_PERIPHCLK_SPI23;     /* ����SPI2ʱ��Դ */
        rcc_periph_clk_init.Spi23ClockSelection = RCC_SPI23CLKSOURCE_PLL1Q; /* SPI2ʱ��Դʹ��PLL1Q */
        HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init);

        gpio_init_struct.Pin = SPI2_SCK_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;                            /* ����������� */
        gpio_init_struct.Pull = GPIO_PULLUP;                                /* ���� */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;                 /* ���� */
        gpio_init_struct.Alternate = SPI2_SCK_GPIO_AF;                      /* ���� */
        HAL_GPIO_Init(SPI2_SCK_GPIO_PORT, &gpio_init_struct);               /* ��ʼ��SCK���� */

        gpio_init_struct.Pin = SPI2_MISO_GPIO_PIN;
        gpio_init_struct.Alternate = SPI2_MISO_GPIO_AF;                     /* ���� */
        HAL_GPIO_Init(SPI2_MISO_GPIO_PORT, &gpio_init_struct);              /* ��ʼ��MISO���� */
        
        gpio_init_struct.Pin = SPI2_MOSI_GPIO_PIN;
        gpio_init_struct.Alternate = SPI2_MOSI_GPIO_AF;                     /* ���� */
        HAL_GPIO_Init(SPI2_MOSI_GPIO_PORT, &gpio_init_struct);              /* ��ʼ��MOSI���� */

    } else if (hspi->Instance == SPI3_SPIx)
    {
        SPI3_SPI_CLK_ENABLE();                                              /* SPI3ʱ��ʹ�� */
        SPI3_SCK_GPIO_CLK_ENABLE();                                         /* SPI3_SCK��ʱ��ʹ�� */
        SPI3_MISO_GPIO_CLK_ENABLE();                                        /* SPI3_MISO��ʱ��ʹ�� */
        SPI3_MOSI_GPIO_CLK_ENABLE();                                        /* SPI3_MOSI��ʱ��ʹ�� */

        /* ����SPI3��ʱ��Դ */
        rcc_periph_clk_init.PeriphClockSelection = RCC_PERIPHCLK_SPI45;     /* ����SPI3ʱ��Դ */
        rcc_periph_clk_init.Spi23ClockSelection = RCC_SPI45CLKSOURCE_PLL2Q; /* SPI3ʱ��Դʹ��PLL1Q */
        HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init);

        gpio_init_struct.Pin = SPI3_SCK_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;                            /* ����������� */
        gpio_init_struct.Pull = GPIO_PULLUP;                                /* ���� */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;                 /* ���� */
        gpio_init_struct.Alternate = SPI3_SCK_GPIO_AF;                      /* ���� */
        HAL_GPIO_Init(SPI3_SCK_GPIO_PORT, &gpio_init_struct);               /* ��ʼ��SCK���� */

        gpio_init_struct.Pin = SPI3_MISO_GPIO_PIN;
        gpio_init_struct.Alternate = SPI3_MISO_GPIO_AF;                     /* ���� */
        HAL_GPIO_Init(SPI3_MISO_GPIO_PORT, &gpio_init_struct);              /* ��ʼ��MISO���� */
        
        gpio_init_struct.Pin = SPI3_MOSI_GPIO_PIN;
        gpio_init_struct.Alternate = SPI3_MOSI_GPIO_AF;                     /* ���� */
        HAL_GPIO_Init(SPI3_MOSI_GPIO_PORT, &gpio_init_struct);              /* ��ʼ��MOSI���� */
    
    } 
    else if (hspi->Instance == SDNAND_SPI)
    {
        /* ����ʱ��Դ */
        rcc_periph_clk_init.PeriphClockSelection = RCC_PERIPHCLK_SPI45;
        rcc_periph_clk_init.Xspi1ClockSelection = RCC_SPI45CLKSOURCE_PLL2Q;
        HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init);
        
        /* ʹ��ʱ�� */
        SDNAND_SPI_CLK_ENABLE();
        SDNAND_SPI_SCK_GPIO_CLK_ENABLE();
        SDNAND_SPI_MOSI_GPIO_CLK_ENABLE();
        SDNAND_SPI_MISO_GPIO_CLK_ENABLE();
        
        /* ��ʼ��ͨѶ���� */
        gpio_init_struct.Pin = SDNAND_SPI_SCK_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        gpio_init_struct.Alternate = SDNAND_SPI_SCK_GPIO_AF;
        HAL_GPIO_Init(SDNAND_SPI_SCK_GPIO_PORT, &gpio_init_struct);
        gpio_init_struct.Pin = SDNAND_SPI_MOSI_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        gpio_init_struct.Alternate = SDNAND_SPI_MOSI_GPIO_AF;
        HAL_GPIO_Init(SDNAND_SPI_MOSI_GPIO_PORT, &gpio_init_struct);
        gpio_init_struct.Pin = SDNAND_SPI_MISO_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
        gpio_init_struct.Alternate = SDNAND_SPI_MISO_GPIO_AF;
        HAL_GPIO_Init(SDNAND_SPI_MISO_GPIO_PORT, &gpio_init_struct);
    }
 
}

/**
 * @brief       SPI�ٶ����ú���
 *   @note      SPIʱ��ѡ������pll1_q_ck, Ϊ250Mhz
 *              SPI�ٶ� = spi_ker_ck / 2^(speed + 1)
 * @param       speed: SPI2ʱ�ӷ�Ƶϵ��,SPI_BAUDRATEPRESCALER_2~SPI_BAUDRATEPRESCALER_256
 * @retval      ��
 */
void spi_set_speed(unsigned int spi_periph, unsigned int speed)
{
    char i = 0;
    switch (spi_periph)
    {
    case 1:
        i = 0;
        break;
    case 2:
        i = 1;
        break;
    case 3:
        i = 2;
        break;
    default:
        break;
    }
    assert_param(IS_SPI_BAUDRATE_PRESCALER(speed));                     /* �ж���Ч�� */
    __HAL_SPI_DISABLE(&g_spi_handle[i]);                                   /* �ر�SPI */
    g_spi_handle[i].Instance->CFG1 &= ~(0X7<<28);                          /* λ30-28���㣬�������ò����� */
    g_spi_handle[i].Instance->CFG1 |= speed;                               /* ����SPI�ٶ� */
    __HAL_SPI_ENABLE(&g_spi_handle[i]);                                    /* ʹ��SPI */
}

/**
 * @brief       SPI2��дһ���ֽ�����
 * @param       txdata: Ҫ���͵�����(1�ֽ�)
 * @retval      ���յ�������(1�ֽ�)
 */
unsigned char spi_read_write_byte(unsigned int spi_periph, unsigned char* txdata, unsigned char* rxdata, unsigned char size)
{
    // unsigned char rxdata;
    char i = 0;
    switch (spi_periph)
    {
    case 1:
        i = 0;
        break;
    case 2:
        i = 1;
        break;
    case 3:
        i = 2;
        break;
    default:
        break;
    }
    HAL_SPI_TransmitReceive(&g_spi_handle[i], txdata, rxdata, size, 1000);
    return size;                                                      /* �����յ������� */
}
