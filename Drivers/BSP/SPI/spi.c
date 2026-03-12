#include "./BSP/SPI/spi.h"
#include "./BSP/SDNAND/spi_sdnand.h"
#include "./BSP/TIMER/gtim.h"
#include "./BSP/DMA_LIST/dma_list.h"

SPI_HandleTypeDef g_spi_handle[3];  
 
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
    // g_spi_handle[i].Instance = spi_periph;
    g_spi_handle[i].Init.Mode = SPI_MODE_MASTER;
    g_spi_handle[i].Init.Direction = SPI_DIRECTION_2LINES;
    g_spi_handle[i].Init.DataSize = SPI_DATASIZE_8BIT;
    g_spi_handle[i].Init.CLKPolarity = SPI_POLARITY_LOW;
    g_spi_handle[i].Init.CLKPhase = SPI_PHASE_2EDGE;
    g_spi_handle[i].Init.NSS = SPI_NSS_SOFT;
    g_spi_handle[i].Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    g_spi_handle[i].Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
    g_spi_handle[i].Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    g_spi_handle[i].Init.FirstBit = SPI_FIRSTBIT_MSB;
    g_spi_handle[i].Init.TIMode = SPI_TIMODE_DISABLE;
    g_spi_handle[i].Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    g_spi_handle[i].Init.CRCPolynomial = 7;

    // 添加FIFO阈值配置（默认为0，即SPI_FIFO_THRESHOLD_01DATA，表示每传输一个数据就触发一次中断或者DMA）
    //    g_spi_handle[i].Init.FifoThreshold = SPI_FIFO_THRESHOLD_02DATA;

    HAL_SPI_Init(&g_spi_handle[i]);

    // 屏蔽使能SPI传输，等DMA准备好后再使能，确保SPI传输与DMA传输之间的同步问题
    __HAL_SPI_ENABLE(&g_spi_handle[i]); /* ʹ��SPI2 */
}
 
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef gpio_init_struct = {0};
    RCC_PeriphCLKInitTypeDef rcc_periph_clk_init = {0};

    if (hspi->Instance == SPI1_SPIx)
    {
        SPI1_SPI_CLK_ENABLE();       /* SPI1ʱ��ʹ�� */
        SPI1_SCK_GPIO_CLK_ENABLE();  /* SPI1_SCK��ʱ��ʹ�� */
        SPI1_MISO_GPIO_CLK_ENABLE(); /* SPI1_MISO��ʱ��ʹ�� */
        SPI1_MOSI_GPIO_CLK_ENABLE(); /* SPI1_MOSI��ʱ��ʹ�� */

        /* ����SPI1��ʱ��Դ */
        rcc_periph_clk_init.PeriphClockSelection = RCC_PERIPHCLK_SPI1;    /* ����SPI1ʱ��Դ */
        rcc_periph_clk_init.Spi1ClockSelection = RCC_SPI1CLKSOURCE_PLL1Q; /* SPI1ʱ��Դʹ��PLL1Q */
        HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init);

        gpio_init_struct.Pin = SPI1_SCK_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;              /* ����������� */
        gpio_init_struct.Pull = GPIO_PULLUP;                  /* ���� */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;   /* ���� */
        gpio_init_struct.Alternate = SPI1_SCK_GPIO_AF;        /* ���� */
        HAL_GPIO_Init(SPI1_SCK_GPIO_PORT, &gpio_init_struct); /* ��ʼ��SCK���� */

        gpio_init_struct.Pin = SPI1_MISO_GPIO_PIN;
        gpio_init_struct.Alternate = SPI1_MISO_GPIO_AF;        /* ���� */
        HAL_GPIO_Init(SPI1_MISO_GPIO_PORT, &gpio_init_struct); /* ��ʼ��MISO���� */

        gpio_init_struct.Pin = SPI1_MOSI_GPIO_PIN;
        gpio_init_struct.Alternate = SPI1_MOSI_GPIO_AF;        /* ���� */
        HAL_GPIO_Init(SPI1_MOSI_GPIO_PORT, &gpio_init_struct); /* ��ʼ��MOSI���� */
    }
    else if (hspi->Instance == SPI2_SPIx)
    {
        SPI2_SPI_CLK_ENABLE();       /* SPI2ʱ��ʹ�� */
        SPI2_SCK_GPIO_CLK_ENABLE();  /* SPI2_SCK��ʱ��ʹ�� */
        SPI2_MISO_GPIO_CLK_ENABLE(); /* SPI2_MISO��ʱ��ʹ�� */
        SPI2_MOSI_GPIO_CLK_ENABLE(); /* SPI2_MOSI��ʱ��ʹ�� */

        /* ����SPI2��ʱ��Դ */
        rcc_periph_clk_init.PeriphClockSelection = RCC_PERIPHCLK_SPI23;     /* ����SPI2ʱ��Դ */
        rcc_periph_clk_init.Spi23ClockSelection = RCC_SPI23CLKSOURCE_PLL1Q; /* SPI2ʱ��Դʹ��PLL1Q */
        HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init);

        gpio_init_struct.Pin = SPI2_SCK_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;              /* ����������� */
        gpio_init_struct.Pull = GPIO_PULLUP;                  /* ���� */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;   /* ���� */
        gpio_init_struct.Alternate = SPI2_SCK_GPIO_AF;        /* ���� */
        HAL_GPIO_Init(SPI2_SCK_GPIO_PORT, &gpio_init_struct); /* ��ʼ��SCK���� */

        gpio_init_struct.Pin = SPI2_MISO_GPIO_PIN;
        gpio_init_struct.Alternate = SPI2_MISO_GPIO_AF;        /* ���� */
        HAL_GPIO_Init(SPI2_MISO_GPIO_PORT, &gpio_init_struct); /* ��ʼ��MISO���� */

        gpio_init_struct.Pin = SPI2_MOSI_GPIO_PIN;
        gpio_init_struct.Alternate = SPI2_MOSI_GPIO_AF;        /* ���� */
        HAL_GPIO_Init(SPI2_MOSI_GPIO_PORT, &gpio_init_struct); /* ��ʼ��MOSI���� */
    }
    else if (hspi->Instance == SPI3_SPIx)
    {
        SPI3_SPI_CLK_ENABLE();       /* SPI3ʱ��ʹ�� */
        SPI3_SCK_GPIO_CLK_ENABLE();  /* SPI3_SCK��ʱ��ʹ�� */
        SPI3_MISO_GPIO_CLK_ENABLE(); /* SPI3_MISO��ʱ��ʹ�� */
        SPI3_MOSI_GPIO_CLK_ENABLE(); /* SPI3_MOSI��ʱ��ʹ�� */

        /* ����SPI3��ʱ��Դ */
        rcc_periph_clk_init.PeriphClockSelection = RCC_PERIPHCLK_SPI45;     /* ����SPI3ʱ��Դ */
        rcc_periph_clk_init.Spi23ClockSelection = RCC_SPI45CLKSOURCE_PLL2Q; /* SPI3ʱ��Դʹ��PLL1Q */
        HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init);

        gpio_init_struct.Pin = SPI3_SCK_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;              /* ����������� */
        gpio_init_struct.Pull = GPIO_PULLUP;                  /* ���� */
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;   /* ���� */
        gpio_init_struct.Alternate = SPI3_SCK_GPIO_AF;        /* ���� */
        HAL_GPIO_Init(SPI3_SCK_GPIO_PORT, &gpio_init_struct); /* ��ʼ��SCK���� */

        gpio_init_struct.Pin = SPI3_MISO_GPIO_PIN;
        gpio_init_struct.Alternate = SPI3_MISO_GPIO_AF;        /* ���� */
        HAL_GPIO_Init(SPI3_MISO_GPIO_PORT, &gpio_init_struct); /* ��ʼ��MISO���� */

        gpio_init_struct.Pin = SPI3_MOSI_GPIO_PIN;
        gpio_init_struct.Alternate = SPI3_MOSI_GPIO_AF;        /* ���� */
        HAL_GPIO_Init(SPI3_MOSI_GPIO_PORT, &gpio_init_struct); /* ��ʼ��MOSI���� */
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
    assert_param(IS_SPI_BAUDRATE_PRESCALER(speed)); /* �ж���Ч�� */
    __HAL_SPI_DISABLE(&g_spi_handle[i]);            /* �ر�SPI */
    g_spi_handle[i].Instance->CFG1 &= ~(0X7 << 28); /* λ30-28���㣬�������ò����� */
    g_spi_handle[i].Instance->CFG1 |= speed;        /* ����SPI�ٶ� */
    __HAL_SPI_ENABLE(&g_spi_handle[i]);             /* ʹ��SPI */
}

/**
 * @brief       SPI2��дһ���ֽ�����
 * @param       txdata: Ҫ���͵�����(1�ֽ�)
 * @retval      ���յ�������(1�ֽ�)
 */
unsigned char spi_read_write_byte(unsigned int spi_periph, unsigned char *txdata, unsigned char *rxdata, unsigned char size)
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
    return size; /* �����յ������� */
}
 
//  unsigned char spi_read_write_byte_fast(unsigned int spi_periph,
//                                         unsigned char *txdata,
//                                         unsigned char *rxdata,
//                                         unsigned char size)
// {
//     SPI_TypeDef *SPIx = NULL;
//     uint8_t idx = 0;

//     switch (spi_periph) {
//         case SPI1_SPI: SPIx = SPI1_SPIx; idx = 0; break;
//         case SPI2_SPI: SPIx = SPI2_SPIx; idx = 1; break;
//         case SPI3_SPI: SPIx = SPI3_SPIx; idx = 2; break;
//         default: return 0;
//     }

//     SPI_HandleTypeDef *hspi = &g_spi_handle[idx];

//     /* 设置传输字节数 */
//     MODIFY_REG(SPIx->CR2, SPI_CR2_TSIZE, size);
//     __HAL_SPI_ENABLE(hspi);
//     SET_BIT(SPIx->CR1, SPI_CR1_CSTART);

//     uint8_t tx_sent = 0;
//     uint8_t rx_recv = 0;

//     /* 流水线：TX和RX同时推进，不等单字节完成 */
//     while (rx_recv < size)
//     {
//         /* 有FIFO空间且还有数据要发 */
//         if (tx_sent < size && (SPIx->SR & SPI_SR_TXP))
//         {
//             *(__IO uint8_t *)&SPIx->TXDR = txdata ? txdata[tx_sent] : 0xFF;
//             tx_sent++;
//         }
//         /* 有数据可读 */
//         if (SPIx->SR & SPI_SR_RXP)
//         {
//             uint8_t rx_byte = *(__IO uint8_t *)&SPIx->RXDR;
//             if (rxdata) rxdata[rx_recv] = rx_byte;
//             rx_recv++;
//         }
//     }

//     /* 等待传输彻底完成 */
//     uint32_t timeout = 10000;
//     while (!(SPIx->SR & SPI_SR_EOT) && --timeout);
//     SPIx->IFCR = SPI_IFCR_EOTC | SPI_IFCR_TXTFC;
//     hspi->State = HAL_SPI_STATE_READY;

//     return size;
// }

unsigned char spi_read_write_byte_fast(unsigned int spi_periph, unsigned char *txdata, unsigned char *rxdata, unsigned char size)
{
    SPI_TypeDef *SPIx = NULL;
    uint8_t i = 0;
    SPI_HandleTypeDef *hspi;

    // 获取SPI实例
    switch (spi_periph)
    {
    case SPI1_SPI:
        SPIx = SPI1_SPIx;
        i = 0;
        break;
    case SPI2_SPI:
        SPIx = SPI2_SPIx;
        i = 1;
        break;
    case SPI3_SPI:
        SPIx = SPI3_SPIx;
        i = 2;
        break;
    default:
        return 0;
    }

    hspi = &g_spi_handle[i];

    /* Set the transaction information */
    hspi->State = HAL_SPI_STATE_BUSY_TX_RX;
    hspi->ErrorCode = HAL_SPI_ERROR_NONE;
    hspi->pRxBuffPtr = (uint8_t *)rxdata;
    hspi->RxXferCount = size;
    hspi->RxXferSize = size;
    hspi->pTxBuffPtr = (const uint8_t *)txdata;
    hspi->TxXferCount = size;
    hspi->TxXferSize = size;

    /*Init field not used in handle to zero */
    hspi->RxISR = NULL;
    hspi->TxISR = NULL;

    /* Set Full-Duplex mode */
    SPI_2LINES(hspi);

    /* Set the number of data at current transfer */
    MODIFY_REG(hspi->Instance->CR2, SPI_CR2_TSIZE, size);

    __HAL_SPI_ENABLE(hspi);

    if (hspi->Init.Mode == SPI_MODE_MASTER)
    {
        /* Master transfer start */
        SET_BIT(hspi->Instance->CR1, SPI_CR1_CSTART);
    }

    // 直接使用HAL库的寄存器配置，但用轮询方式读取
    for (uint8_t byte_count = 0; byte_count < size; byte_count++)
    {
        uint8_t tx_byte = txdata ? txdata[byte_count] : 0xFF;

        // 等待TXE标志（发送缓冲区空）
        while (!(__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_TXP)))
            ;

        // 写入数据
        *(__IO uint8_t *)&SPIx->TXDR = tx_byte;

        // 等待RXNE标志（接收缓冲区非空）
        while (!(__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_RXP)))
            ;

        // 读取数据
        if (rxdata)
        {
            rxdata[byte_count] = *(__IO uint8_t *)&SPIx->RXDR;
        }
        else
        {
            (void)*(__IO uint8_t *)&SPIx->RXDR;
        }
    }

    // 等待传输完成
    while (!__HAL_SPI_GET_FLAG(hspi, SPI_FLAG_EOT))
        ;

    // 清除状态标志
    SPIx->IFCR = SPI_IFCR_EOTC | SPI_IFCR_TXTFC;

    return size;
}

/**
 * @brief       SPI超快速批量传输函数（寄存器版本，用于连续读取多个字节）
 * @param       spi_periph: SPI外设编号
 * @param       rxdata: 接收数据缓冲区
 * @param       size: 传输字节数
 * @retval      实际传输的字节数
 */
unsigned char spi_read_bulk_fast(unsigned int spi_periph, unsigned char *rxdata, unsigned char size)
{
    SPI_TypeDef *SPIx = NULL;
    uint8_t i = 0;
    SPI_HandleTypeDef *hspi;

    // 获取SPI实例
    switch (spi_periph)
    {
    case SPI1_SPI:
        SPIx = SPI1_SPIx;
        i = 0;
        break;
    case SPI2_SPI:
        SPIx = SPI2_SPIx;
        i = 1;
        break;
    case SPI3_SPI:
        SPIx = SPI3_SPIx;
        i = 2;
        break;
    default:
        return 0;
    }

    hspi = &g_spi_handle[i];

    // 确保SPI已使能
    __HAL_SPI_ENABLE(hspi);

    // 设置SPI时钟为最高速度（30MHz，2分频为15MHz）
    SPIx->CFG1 &= ~SPI_CFG1_MBR;
    SPIx->CFG1 |= SPI_CFG1_MBR_0; // 预分频2

    // 使用FIFO进行批量传输
    for (uint8_t byte_count = 0; byte_count < size; byte_count++)
    {
        // 发送空字节以生成时钟
        *((volatile uint8_t *)&SPIx->TXDR) = 0xFF;

        // 等待RXFIFO有数据
        while ((SPIx->SR & SPI_SR_RXP) == 0)
            ;

        // 读取数据
        rxdata[byte_count] = *((volatile uint8_t *)&SPIx->RXDR);
    }

    // 等待传输完成
    while ((SPIx->SR & SPI_SR_EOT) == 0)
        ;

    // 清除状态标志
    SPIx->IFCR = SPI_IFCR_EOTC | SPI_IFCR_TXTFC;

    return size;
}

/**
 * @brief       SPI快速传输16位数据函数（寄存器版本）
 * @param       spi_periph: SPI外设编号
 * @param       txdata: 发送数据缓冲区（16位）
 * @param       rxdata: 接收数据缓冲区（16位）
 * @param       size: 传输16位数据个数
 * @retval      实际传输的16位数据个数
 */
unsigned char spi_read_write_halfword_fast(unsigned int spi_periph, uint16_t *txdata, uint16_t *rxdata, unsigned char size)
{
    SPI_TypeDef *SPIx = NULL;
    uint8_t i = 0;
    uint32_t timeout = 0;
    SPI_HandleTypeDef *hspi;

    // 获取SPI实例
    switch (spi_periph)
    {
    case SPI1_SPI:
        SPIx = SPI1_SPIx;
        i = 0;
        break;
    case SPI2_SPI:
        SPIx = SPI2_SPIx;
        i = 1;
        break;
    case SPI3_SPI:
        SPIx = SPI3_SPIx;
        i = 2;
        break;
    default:
        return 0;
    }

    hspi = &g_spi_handle[i];

    // 设置数据大小为16位
    SPIx->CFG1 &= ~SPI_CFG1_DSIZE;
    SPIx->CFG1 |= (15 << SPI_CFG1_DSIZE_Pos); // 16位数据

    // 确保SPI已使能
    __HAL_SPI_ENABLE(hspi);

    // 设置SPI时钟为最高速度
    SPIx->CFG1 &= ~SPI_CFG1_MBR;
    SPIx->CFG1 |= SPI_CFG1_MBR_0; // 预分频2

    for (uint8_t word_count = 0; word_count < size; word_count++)
    {
        // 等待TXFIFO有空闲位置
        timeout = 1000;
        while (((SPIx->SR & SPI_SR_TXP) == 0) && timeout--)
        {
            if (timeout == 0)
                return word_count;
        }

        // 写入发送数据（16位）
        if (txdata)
        {
            *((volatile uint16_t *)&SPIx->TXDR) = txdata[word_count];
        }
        else
        {
            *((volatile uint16_t *)&SPIx->TXDR) = 0xFFFF; // 发送空数据
        }

        // 等待RXFIFO有数据
        timeout = 1000;
        while (((SPIx->SR & SPI_SR_RXP) == 0) && timeout--)
        {
            if (timeout == 0)
                return word_count;
        }

        // 读取接收数据（16位）
        if (rxdata)
        {
            rxdata[word_count] = *((volatile uint16_t *)&SPIx->RXDR);
        }
        else
        {
            (void)*((volatile uint16_t *)&SPIx->RXDR); // 丢弃数据
        }
    }

    // 等待传输完成
    timeout = 1000;
    while ((SPIx->SR & SPI_SR_EOT) == 0 && timeout--)
    {
        if (timeout == 0)
            break;
    }

    // 恢复为8位数据模式
    SPIx->CFG1 &= ~SPI_CFG1_DSIZE;
    SPIx->CFG1 |= (7 << SPI_CFG1_DSIZE_Pos); // 8位数据

    // 清除状态标志
    SPIx->IFCR = SPI_IFCR_EOTC | SPI_IFCR_TXTFC;

    return size;
}

/**
 * @brief       SPI快速读取菊花链数据（专用函数）
 * @param       spi_periph: SPI外设编号
 * @param       adc_data: ADC数据缓冲区（16位数组）
 * @retval      成功读取的ADC数量
 */
unsigned char spi_read_ads8319_chain_fast(unsigned int spi_periph, uint16_t *adc_data)
{
    SPI_TypeDef *SPIx = NULL;
    uint8_t i = 0;
    uint16_t rx_buffer[ADS8319_CHAIN_LENGTH] = {0};
    SPI_HandleTypeDef *hspi;

    // 获取SPI实例
    switch (spi_periph)
    {
    case SPI1_SPI:
        SPIx = SPI1_SPIx;
        i = 0;
        break;
    case SPI2_SPI:
        SPIx = SPI2_SPIx;
        i = 1;
        break;
    case SPI3_SPI:
        SPIx = SPI3_SPIx;
        i = 2;
        break;
    default:
        return 0;
    }

    hspi = &g_spi_handle[i];

    // 设置数据大小为16位（ADS8319输出16位数据）
    SPIx->CFG1 &= ~SPI_CFG1_DSIZE;
    SPIx->CFG1 |= (15 << SPI_CFG1_DSIZE_Pos); // 16位数据

    // 确保SPI已使能
    __HAL_SPI_ENABLE(hspi);

    // 设置SPI时钟为最高速度（30MHz，2分频为15MHz）
    SPIx->CFG1 &= ~SPI_CFG1_MBR;
    SPIx->CFG1 |= SPI_CFG1_MBR_0; // 预分频2

    // 使用FIFO进行批量传输
    for (uint8_t i = 0; i < ADS8319_CHAIN_LENGTH; i++)
    {
        // 发送空数据以生成时钟
        *((volatile uint16_t *)&SPIx->TXDR) = 0xFFFF;

        // 等待RXFIFO有数据
        while ((SPIx->SR & SPI_SR_RXP) == 0)
            ;

        // 读取16位数据
        rx_buffer[i] = *((volatile uint16_t *)&SPIx->RXDR);
    }

    // 等待传输完成
    while ((SPIx->SR & SPI_SR_EOT) == 0)
        ;

    // 恢复为8位数据模式
    SPIx->CFG1 &= ~SPI_CFG1_DSIZE;
    SPIx->CFG1 |= (7 << SPI_CFG1_DSIZE_Pos); // 8位数据

    // 清除状态标志
    SPIx->IFCR = SPI_IFCR_EOTC | SPI_IFCR_TXTFC;

    // 将数据复制到输出缓冲区
    for (uint8_t i = 0; i < ADS8319_CHAIN_LENGTH; i++)
    {
        adc_data[i] = rx_buffer[i];
    }

    return ADS8319_CHAIN_LENGTH;
}
