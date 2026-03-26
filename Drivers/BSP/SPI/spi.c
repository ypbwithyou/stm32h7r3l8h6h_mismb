#include "./BSP/SPI/spi.h"
#include "./BSP/SDNAND/spi_sdnand.h"
#include "./BSP/ADS8319/ads8319.h"

SPI_HandleTypeDef g_spi_handle[3];
DMA_HandleTypeDef g_spi_dma_rx_handle[3];
DMA_HandleTypeDef g_spi_dma_tx_handle[3];

static uint32_t spi_get_dma_rx_request(uint8_t idx);
static uint32_t spi_get_dma_tx_request(uint8_t idx);
static DMA_Channel_TypeDef *spi_get_dma_rx_instance(uint8_t idx);
static DMA_Channel_TypeDef *spi_get_dma_tx_instance(uint8_t idx);
static IRQn_Type spi_get_irqn(uint8_t idx);
static IRQn_Type spi_get_dma_rx_irqn(uint8_t idx);
static IRQn_Type spi_get_dma_tx_irqn(uint8_t idx);
static void spi_dma_config(DMA_HandleTypeDef *hdma,
                           DMA_Channel_TypeDef *instance,
                           uint32_t request,
                           uint32_t direction);
static void spi_cache_clean_range(const void *addr, uint32_t size);
static void spi_cache_invalidate_range(const void *addr, uint32_t size);
static void spi_sdnand_msp_init(void);
static SPI_TypeDef *spi_get_instance(unsigned int spi_periph);
static void spi_rx_dma_cplt_callback(DMA_HandleTypeDef *hdma);
static void spi_rx_dma_error_callback(DMA_HandleTypeDef *hdma);
static HAL_StatusTypeDef spi_start_rx_dma_only(SPI_HandleTypeDef *hspi,
                                               const uint8_t *txdata,
                                               uint8_t *rxdata,
                                               uint16_t size);
static void spi_finish_rx_dma_only(SPI_HandleTypeDef *hspi);
static void spi_drain_rx_fifo(SPI_HandleTypeDef *hspi);
static uint32_t spi_collect_error_flags(SPI_HandleTypeDef *hspi);

uint8_t spi_get_index(unsigned int spi_periph)
{
    switch (spi_periph)
    {
    case SPI1_SPI:
        return 0U;
    case SPI2_SPI:
        return 1U;
    case SPI3_SPI:
        return 2U;
    default:
        return 0xFFU;
    }
}

unsigned int spi_get_periph_from_instance(SPI_TypeDef *instance)
{
    if (instance == SPI1_SPIx)
        return SPI1_SPI;
    if (instance == SPI2_SPIx)
        return SPI2_SPI;
    if (instance == SPI3_SPIx)
        return SPI3_SPI;
    return 0U;
}

static SPI_TypeDef *spi_get_instance(unsigned int spi_periph)
{
    switch (spi_periph)
    {
    case SPI1_SPI:
        return SPI1_SPIx;
    case SPI2_SPI:
        return SPI2_SPIx;
    case SPI3_SPI:
        return SPI3_SPIx;
    default:
        return NULL;
    }
}

void spi_init(unsigned int spi_periph)
{
    uint8_t idx = spi_get_index(spi_periph);

    if (idx >= SPI_USED_MAX)
    {
        return;
    }

    g_spi_handle[idx].Instance = spi_get_instance(spi_periph);
    g_spi_handle[idx].Init.Mode = SPI_MODE_MASTER;
    g_spi_handle[idx].Init.Direction = SPI_DIRECTION_2LINES;
    g_spi_handle[idx].Init.DataSize = SPI_DATASIZE_8BIT;
    g_spi_handle[idx].Init.CLKPolarity = SPI_POLARITY_LOW;
    g_spi_handle[idx].Init.CLKPhase = SPI_PHASE_2EDGE;
    g_spi_handle[idx].Init.NSS = SPI_NSS_SOFT;
    g_spi_handle[idx].Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    g_spi_handle[idx].Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
    g_spi_handle[idx].Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    g_spi_handle[idx].Init.FirstBit = SPI_FIRSTBIT_MSB;
    g_spi_handle[idx].Init.TIMode = SPI_TIMODE_DISABLE;
    g_spi_handle[idx].Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    g_spi_handle[idx].Init.CRCPolynomial = 7;

    HAL_SPI_Init(&g_spi_handle[idx]);
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef gpio_init_struct = {0};
    RCC_PeriphCLKInitTypeDef rcc_periph_clk_init = {0};
    uint8_t idx = 0xFFU;

    if (hspi->Instance == SPI1_SPIx)
    {
        SPI1_SPI_CLK_ENABLE();
        SPI1_SCK_GPIO_CLK_ENABLE();
        SPI1_MISO_GPIO_CLK_ENABLE();
        SPI1_MOSI_GPIO_CLK_ENABLE();

        rcc_periph_clk_init.PeriphClockSelection = RCC_PERIPHCLK_SPI1;
        rcc_periph_clk_init.Spi1ClockSelection = RCC_SPI1CLKSOURCE_PLL1Q;
        HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init);

        gpio_init_struct.Pin = SPI1_SCK_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init_struct.Alternate = SPI1_SCK_GPIO_AF;
        HAL_GPIO_Init(SPI1_SCK_GPIO_PORT, &gpio_init_struct);

        gpio_init_struct.Pin = SPI1_MISO_GPIO_PIN;
        gpio_init_struct.Alternate = SPI1_MISO_GPIO_AF;
        HAL_GPIO_Init(SPI1_MISO_GPIO_PORT, &gpio_init_struct);

        gpio_init_struct.Pin = SPI1_MOSI_GPIO_PIN;
        gpio_init_struct.Alternate = SPI1_MOSI_GPIO_AF;
        HAL_GPIO_Init(SPI1_MOSI_GPIO_PORT, &gpio_init_struct);
        idx = 0U;
    }
    else if (hspi->Instance == SPI2_SPIx)
    {
        SPI2_SPI_CLK_ENABLE();
        SPI2_SCK_GPIO_CLK_ENABLE();
        SPI2_MISO_GPIO_CLK_ENABLE();
        SPI2_MOSI_GPIO_CLK_ENABLE();

        rcc_periph_clk_init.PeriphClockSelection = RCC_PERIPHCLK_SPI23;
        rcc_periph_clk_init.Spi23ClockSelection = RCC_SPI23CLKSOURCE_PLL1Q;
        HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init);

        gpio_init_struct.Pin = SPI2_SCK_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init_struct.Alternate = SPI2_SCK_GPIO_AF;
        HAL_GPIO_Init(SPI2_SCK_GPIO_PORT, &gpio_init_struct);

        gpio_init_struct.Pin = SPI2_MISO_GPIO_PIN;
        gpio_init_struct.Alternate = SPI2_MISO_GPIO_AF;
        HAL_GPIO_Init(SPI2_MISO_GPIO_PORT, &gpio_init_struct);

        gpio_init_struct.Pin = SPI2_MOSI_GPIO_PIN;
        gpio_init_struct.Alternate = SPI2_MOSI_GPIO_AF;
        HAL_GPIO_Init(SPI2_MOSI_GPIO_PORT, &gpio_init_struct);
        idx = 1U;
    }
    else if (hspi->Instance == SPI3_SPIx)
    {
        SPI3_SPI_CLK_ENABLE();
        SPI3_SCK_GPIO_CLK_ENABLE();
        SPI3_MISO_GPIO_CLK_ENABLE();
        SPI3_MOSI_GPIO_CLK_ENABLE();

        rcc_periph_clk_init.PeriphClockSelection = RCC_PERIPHCLK_SPI45;
        rcc_periph_clk_init.Spi45ClockSelection = RCC_SPI45CLKSOURCE_PLL2Q;
        HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init);

        gpio_init_struct.Pin = SPI3_SCK_GPIO_PIN;
        gpio_init_struct.Mode = GPIO_MODE_AF_PP;
        gpio_init_struct.Pull = GPIO_PULLUP;
        gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        gpio_init_struct.Alternate = SPI3_SCK_GPIO_AF;
        HAL_GPIO_Init(SPI3_SCK_GPIO_PORT, &gpio_init_struct);

        gpio_init_struct.Pin = SPI3_MISO_GPIO_PIN;
        gpio_init_struct.Alternate = SPI3_MISO_GPIO_AF;
        HAL_GPIO_Init(SPI3_MISO_GPIO_PORT, &gpio_init_struct);

        gpio_init_struct.Pin = SPI3_MOSI_GPIO_PIN;
        gpio_init_struct.Alternate = SPI3_MOSI_GPIO_AF;
        HAL_GPIO_Init(SPI3_MOSI_GPIO_PORT, &gpio_init_struct);
        idx = 2U;
    }
    else if (hspi->Instance == SDNAND_SPI)
    {
        spi_sdnand_msp_init();
        return;
    }
    else
    {
        return;
    }

    __HAL_RCC_GPDMA1_CLK_ENABLE();

    spi_dma_config(&g_spi_dma_rx_handle[idx], spi_get_dma_rx_instance(idx), spi_get_dma_rx_request(idx), DMA_PERIPH_TO_MEMORY);
    __HAL_LINKDMA(hspi, hdmarx, g_spi_dma_rx_handle[idx]);

    spi_dma_config(&g_spi_dma_tx_handle[idx], spi_get_dma_tx_instance(idx), spi_get_dma_tx_request(idx), DMA_MEMORY_TO_PERIPH);
    __HAL_LINKDMA(hspi, hdmatx, g_spi_dma_tx_handle[idx]);

    HAL_NVIC_SetPriority(spi_get_dma_rx_irqn(idx), 0, 0);
    HAL_NVIC_EnableIRQ(spi_get_dma_rx_irqn(idx));
    HAL_NVIC_SetPriority(spi_get_dma_tx_irqn(idx), 3, 0);
    HAL_NVIC_EnableIRQ(spi_get_dma_tx_irqn(idx));
    HAL_NVIC_SetPriority(spi_get_irqn(idx), 3, 1);
    HAL_NVIC_EnableIRQ(spi_get_irqn(idx));
}

void spi_set_speed(unsigned int spi_periph, unsigned int speed)
{
    uint8_t idx = spi_get_index(spi_periph);

    if (idx >= SPI_USED_MAX)
    {
        return;
    }

    assert_param(IS_SPI_BAUDRATE_PRESCALER(speed));
    __HAL_SPI_DISABLE(&g_spi_handle[idx]);
    g_spi_handle[idx].Instance->CFG1 &= ~(0X7UL << 28);
    g_spi_handle[idx].Instance->CFG1 |= speed;
    __HAL_SPI_ENABLE(&g_spi_handle[idx]);
}

HAL_StatusTypeDef spi_read_write_dma_start(unsigned int spi_periph, const uint8_t *txdata, uint8_t *rxdata, uint16_t size)
{
    uint8_t idx = spi_get_index(spi_periph);

    if ((idx >= SPI_USED_MAX) || (rxdata == NULL) || (size == 0U))
    {
        return HAL_ERROR;
    }

    if (txdata != NULL)
    {
        spi_cache_clean_range(txdata, size);
    }
    spi_cache_invalidate_range(rxdata, size);

    return spi_start_rx_dma_only(&g_spi_handle[idx], txdata, rxdata, size);
}

void spi_dma_stop(unsigned int spi_periph)
{
    uint8_t idx = spi_get_index(spi_periph);

    if (idx < SPI_USED_MAX)
    {
        (void)HAL_SPI_DMAStop(&g_spi_handle[idx]);
        spi_finish_rx_dma_only(&g_spi_handle[idx]);
    }
}

unsigned char spi_read_write_byte(unsigned int spi_periph, unsigned char *txdata, unsigned char *rxdata, unsigned char size)
{
    uint8_t idx = spi_get_index(spi_periph);

    if (idx >= SPI_USED_MAX)
    {
        return 0U;
    }

    HAL_SPI_TransmitReceive(&g_spi_handle[idx], txdata, rxdata, size, 1000);
    return size;
}

unsigned char spi_read_write_byte_fast(unsigned int spi_periph,
                                       unsigned char *txdata,
                                       unsigned char *rxdata,
                                       unsigned char size)
{
    SPI_TypeDef *SPIx = spi_get_instance(spi_periph);
    uint8_t idx = spi_get_index(spi_periph);
    SPI_HandleTypeDef *hspi;
    uint8_t tx_sent = 0U;
    uint8_t rx_recv = 0U;
    uint32_t timeout = 10000U;

    if ((SPIx == NULL) || (idx >= SPI_USED_MAX))
    {
        return 0U;
    }

    hspi = &g_spi_handle[idx];

    MODIFY_REG(SPIx->CR2, SPI_CR2_TSIZE, size);
    __HAL_SPI_ENABLE(hspi);
    SET_BIT(SPIx->CR1, SPI_CR1_CSTART);

    while (rx_recv < size)
    {
        if ((tx_sent < size) && ((SPIx->SR & SPI_SR_TXP) != 0U))
        {
            *(__IO uint8_t *)&SPIx->TXDR = txdata ? txdata[tx_sent] : 0xFFU;
            tx_sent++;
        }

        if ((SPIx->SR & SPI_SR_RXP) != 0U)
        {
            uint8_t rx_byte = *(__IO uint8_t *)&SPIx->RXDR;
            if (rxdata != NULL)
            {
                rxdata[rx_recv] = rx_byte;
            }
            rx_recv++;
        }
    }

    while (((SPIx->SR & SPI_SR_EOT) == 0U) && (--timeout != 0U))
    {
    }

    SPIx->IFCR = SPI_IFCR_EOTC | SPI_IFCR_TXTFC;
    hspi->State = HAL_SPI_STATE_READY;

    return size;
}

unsigned char spi_read_bulk_fast(unsigned int spi_periph, unsigned char *rxdata, unsigned char size)
{
    return spi_read_write_byte_fast(spi_periph, NULL, rxdata, size);
}

unsigned char spi_read_write_halfword_fast(unsigned int spi_periph, uint16_t *txdata, uint16_t *rxdata, unsigned char size)
{
    SPI_TypeDef *SPIx = spi_get_instance(spi_periph);
    uint8_t idx = spi_get_index(spi_periph);
    SPI_HandleTypeDef *hspi;
    uint32_t timeout = 1000U;
    uint8_t word_count;

    if ((SPIx == NULL) || (idx >= SPI_USED_MAX))
    {
        return 0U;
    }

    hspi = &g_spi_handle[idx];

    MODIFY_REG(SPIx->CFG1, SPI_CFG1_DSIZE, (15U << SPI_CFG1_DSIZE_Pos));
    __HAL_SPI_ENABLE(hspi);

    SPIx->CFG1 &= ~SPI_CFG1_MBR;
    SPIx->CFG1 |= SPI_CFG1_MBR_0;

    for (word_count = 0U; word_count < size; word_count++)
    {
        timeout = 1000U;
        while (((SPIx->SR & SPI_SR_TXP) == 0U) && (--timeout != 0U))
        {
        }
        if (timeout == 0U)
        {
            break;
        }

        *(__IO uint16_t *)&SPIx->TXDR = txdata ? txdata[word_count] : 0xFFFFU;

        timeout = 1000U;
        while (((SPIx->SR & SPI_SR_RXP) == 0U) && (--timeout != 0U))
        {
        }
        if (timeout == 0U)
        {
            break;
        }

        if (rxdata != NULL)
        {
            rxdata[word_count] = *(__IO uint16_t *)&SPIx->RXDR;
        }
        else
        {
            (void)*(__IO uint16_t *)&SPIx->RXDR;
        }
    }

    timeout = 1000U;
    while (((SPIx->SR & SPI_SR_EOT) == 0U) && (--timeout != 0U))
    {
    }

    MODIFY_REG(SPIx->CFG1, SPI_CFG1_DSIZE, (7U << SPI_CFG1_DSIZE_Pos));
    SPIx->IFCR = SPI_IFCR_EOTC | SPI_IFCR_TXTFC;

    return word_count;
}

unsigned char spi_read_ads8319_chain_fast(unsigned int spi_periph, uint16_t *adc_data)
{
    return spi_read_write_halfword_fast(spi_periph, NULL, adc_data, ADS8319_CHAIN_LENGTH);
}

static uint32_t spi_get_dma_rx_request(uint8_t idx)
{
    static const uint32_t request_map[SPI_USED_MAX] = {
        GPDMA1_REQUEST_SPI1_RX,
        GPDMA1_REQUEST_SPI2_RX,
        GPDMA1_REQUEST_SPI4_RX
    };
    return request_map[idx];
}

static uint32_t spi_get_dma_tx_request(uint8_t idx)
{
    static const uint32_t request_map[SPI_USED_MAX] = {
        GPDMA1_REQUEST_SPI1_TX,
        GPDMA1_REQUEST_SPI2_TX,
        GPDMA1_REQUEST_SPI4_TX
    };
    return request_map[idx];
}

static DMA_Channel_TypeDef *spi_get_dma_rx_instance(uint8_t idx)
{
    static DMA_Channel_TypeDef *const channel_map[SPI_USED_MAX] = {
        GPDMA1_Channel1,
        GPDMA1_Channel3,
        GPDMA1_Channel5
    };
    return channel_map[idx];
}

static DMA_Channel_TypeDef *spi_get_dma_tx_instance(uint8_t idx)
{
    static DMA_Channel_TypeDef *const channel_map[SPI_USED_MAX] = {
        GPDMA1_Channel2,
        GPDMA1_Channel4,
        GPDMA1_Channel6
    };
    return channel_map[idx];
}

static IRQn_Type spi_get_dma_rx_irqn(uint8_t idx)
{
    static const IRQn_Type irqn_map[SPI_USED_MAX] = {
        GPDMA1_Channel1_IRQn,
        GPDMA1_Channel3_IRQn,
        GPDMA1_Channel5_IRQn
    };
    return irqn_map[idx];
}

static IRQn_Type spi_get_irqn(uint8_t idx)
{
    static const IRQn_Type irqn_map[SPI_USED_MAX] = {
        SPI1_IRQn,
        SPI2_IRQn,
        SPI4_IRQn
    };
    return irqn_map[idx];
}

static IRQn_Type spi_get_dma_tx_irqn(uint8_t idx)
{
    static const IRQn_Type irqn_map[SPI_USED_MAX] = {
        GPDMA1_Channel2_IRQn,
        GPDMA1_Channel4_IRQn,
        GPDMA1_Channel6_IRQn
    };
    return irqn_map[idx];
}

static void spi_dma_config(DMA_HandleTypeDef *hdma,
                           DMA_Channel_TypeDef *instance,
                           uint32_t request,
                           uint32_t direction)
{
    hdma->Instance = instance;
    hdma->Init.Request = request;
    hdma->Init.BlkHWRequest = DMA_BREQ_SINGLE_BURST;
    hdma->Init.Direction = direction;
    hdma->Init.SrcInc = (direction == DMA_MEMORY_TO_PERIPH) ? DMA_SINC_INCREMENTED : DMA_SINC_FIXED;
    hdma->Init.DestInc = (direction == DMA_MEMORY_TO_PERIPH) ? DMA_DINC_FIXED : DMA_DINC_INCREMENTED;
    hdma->Init.SrcDataWidth = DMA_SRC_DATAWIDTH_BYTE;
    hdma->Init.DestDataWidth = DMA_DEST_DATAWIDTH_BYTE;
    hdma->Init.Priority = DMA_HIGH_PRIORITY;
    hdma->Init.SrcBurstLength = 1U;
    hdma->Init.DestBurstLength = 1U;
    hdma->Init.TransferAllocatedPort = DMA_SRC_ALLOCATED_PORT0 | DMA_DEST_ALLOCATED_PORT0;
    hdma->Init.TransferEventMode = DMA_TCEM_BLOCK_TRANSFER;
    hdma->Init.Mode = DMA_NORMAL;

    HAL_DMA_DeInit(hdma);
    HAL_DMA_Init(hdma);
    HAL_DMA_ConfigChannelAttributes(hdma, DMA_CHANNEL_NPRIV);
}

static void spi_cache_clean_range(const void *addr, uint32_t size)
{
    uintptr_t start;
    uintptr_t end;

    if ((addr == NULL) || (size == 0U))
    {
        return;
    }

    start = ((uintptr_t)addr) & ~((uintptr_t)31U);
    end = (((uintptr_t)addr) + size + 31U) & ~((uintptr_t)31U);
    SCB_CleanDCache_by_Addr((uint32_t *)start, (int32_t)(end - start));
}

static void spi_cache_invalidate_range(const void *addr, uint32_t size)
{
    uintptr_t start;
    uintptr_t end;

    if ((addr == NULL) || (size == 0U))
    {
        return;
    }

    start = ((uintptr_t)addr) & ~((uintptr_t)31U);
    end = (((uintptr_t)addr) + size + 31U) & ~((uintptr_t)31U);
    SCB_InvalidateDCache_by_Addr((uint32_t *)start, (int32_t)(end - start));
}

static void spi_drain_rx_fifo(SPI_HandleTypeDef *hspi)
{
    while ((hspi->Instance->SR & SPI_SR_RXP) != 0U)
    {
        (void)*(__IO uint8_t *)&hspi->Instance->RXDR;
    }
}

static uint32_t spi_collect_error_flags(SPI_HandleTypeDef *hspi)
{
    uint32_t sr = hspi->Instance->SR;
    uint32_t err = HAL_SPI_ERROR_NONE;

    if ((sr & SPI_SR_OVR) != 0U)
    {
        err |= HAL_SPI_ERROR_OVR;
    }
    if ((sr & SPI_SR_UDR) != 0U)
    {
        err |= HAL_SPI_ERROR_UDR;
    }
    if ((sr & SPI_SR_TIFRE) != 0U)
    {
        err |= HAL_SPI_ERROR_FRE;
    }
    if ((sr & SPI_SR_MODF) != 0U)
    {
        err |= HAL_SPI_ERROR_MODF;
    }

    return err;
}

static void spi_finish_rx_dma_only(SPI_HandleTypeDef *hspi)
{
    __HAL_SPI_DISABLE_IT(hspi, (SPI_IT_EOT | SPI_IT_TXP | SPI_IT_RXP | SPI_IT_DXP |
                                SPI_IT_UDR | SPI_IT_OVR | SPI_IT_FRE | SPI_IT_MODF));
    CLEAR_BIT(hspi->Instance->CFG1, SPI_CFG1_TXDMAEN | SPI_CFG1_RXDMAEN);
    spi_drain_rx_fifo(hspi);
    hspi->Instance->IFCR = SPI_IFCR_EOTC | SPI_IFCR_TXTFC | SPI_IFCR_UDRC |
                           SPI_IFCR_OVRC | SPI_IFCR_TIFREC | SPI_IFCR_MODFC |
                           SPI_IFCR_SUSPC;
    __HAL_SPI_DISABLE(hspi);
}

static void spi_rx_dma_cplt_callback(DMA_HandleTypeDef *hdma)
{
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)hdma->Parent;
    uint32_t timeout = 2048U;
    uint32_t err;
    uint32_t sr;

    if (hspi == NULL)
    {
        return;
    }

    while (((hspi->Instance->SR & SPI_SR_TXC) == 0U) && (--timeout != 0U))
    {
    }

    sr = hspi->Instance->SR;
    err = spi_collect_error_flags(hspi);
    if ((sr & SPI_SR_RXP) != 0U)
    {
        spi_drain_rx_fifo(hspi);
    }

    if ((timeout == 0U) && ((sr & SPI_SR_TXC) == 0U))
    {
        err |= HAL_SPI_ERROR_FLAG;
    }

    spi_finish_rx_dma_only(hspi);
    hspi->State = HAL_SPI_STATE_READY;
    hspi->ErrorCode = err;

    if (err != HAL_SPI_ERROR_NONE)
    {
        HAL_SPI_ErrorCallback(hspi);
        return;
    }

    HAL_SPI_TxRxCpltCallback(hspi);
}

static void spi_rx_dma_error_callback(DMA_HandleTypeDef *hdma)
{
    SPI_HandleTypeDef *hspi = (SPI_HandleTypeDef *)hdma->Parent;
    uint32_t err;

    if (hspi == NULL)
    {
        return;
    }

    err = spi_collect_error_flags(hspi);
    spi_finish_rx_dma_only(hspi);
    hspi->State = HAL_SPI_STATE_READY;
    hspi->ErrorCode |= HAL_SPI_ERROR_DMA;
    hspi->ErrorCode |= err;
    HAL_SPI_ErrorCallback(hspi);
}

static HAL_StatusTypeDef spi_start_rx_dma_only(SPI_HandleTypeDef *hspi,
                                               const uint8_t *txdata,
                                               uint8_t *rxdata,
                                               uint16_t size)
{
    HAL_StatusTypeDef errorcode;
    uint16_t tx_sent = 0U;
    uint32_t timeout;

    if ((hspi == NULL) || (rxdata == NULL) || (size == 0U))
    {
        return HAL_ERROR;
    }

    __HAL_LOCK(hspi);

    if (hspi->State != HAL_SPI_STATE_READY)
    {
        __HAL_UNLOCK(hspi);
        return HAL_BUSY;
    }

    hspi->State = HAL_SPI_STATE_BUSY_TX_RX;
    hspi->ErrorCode = HAL_SPI_ERROR_NONE;
    hspi->pTxBuffPtr = txdata;
    hspi->TxXferSize = size;
    hspi->TxXferCount = size;
    hspi->pRxBuffPtr = rxdata;
    hspi->RxXferSize = size;
    hspi->RxXferCount = size;
    hspi->TxISR = NULL;
    hspi->RxISR = NULL;

    SPI_2LINES(hspi);
    spi_finish_rx_dma_only(hspi);

    hspi->hdmarx->XferHalfCpltCallback = NULL;
    hspi->hdmarx->XferCpltCallback = spi_rx_dma_cplt_callback;
    hspi->hdmarx->XferErrorCallback = spi_rx_dma_error_callback;
    hspi->hdmarx->XferAbortCallback = NULL;
    hspi->hdmatx->XferHalfCpltCallback = NULL;
    hspi->hdmatx->XferCpltCallback = NULL;
    hspi->hdmatx->XferErrorCallback = NULL;
    hspi->hdmatx->XferAbortCallback = NULL;

    errorcode = HAL_DMA_Start_IT(hspi->hdmarx,
                                 (uint32_t)&hspi->Instance->RXDR,
                                 (uint32_t)hspi->pRxBuffPtr,
                                 size);
    if (errorcode != HAL_OK)
    {
        hspi->State = HAL_SPI_STATE_READY;
        hspi->ErrorCode = HAL_SPI_ERROR_DMA;
        __HAL_UNLOCK(hspi);
        return HAL_ERROR;
    }

    MODIFY_REG(hspi->Instance->CR2, SPI_CR2_TSIZE, size);
    SET_BIT(hspi->Instance->CFG1, SPI_CFG1_RXDMAEN);
    __HAL_SPI_ENABLE(hspi);
    SET_BIT(hspi->Instance->CR1, SPI_CR1_CSTART);
    __HAL_UNLOCK(hspi);

    for (tx_sent = 0U; tx_sent < size; tx_sent++)
    {
        timeout = 512U;
        while (((hspi->Instance->SR & SPI_SR_TXP) == 0U) && (--timeout != 0U))
        {
        }

        if (timeout == 0U)
        {
            (void)HAL_DMA_Abort(hspi->hdmarx);
            spi_finish_rx_dma_only(hspi);
            hspi->State = HAL_SPI_STATE_READY;
            hspi->ErrorCode = HAL_SPI_ERROR_FLAG;
            return HAL_ERROR;
        }

        *(__IO uint8_t *)&hspi->Instance->TXDR = (txdata != NULL) ? txdata[tx_sent] : 0xFFU;
    }

    return HAL_OK;
}

static void spi_sdnand_msp_init(void)
{
    RCC_PeriphCLKInitTypeDef rcc_periph_clk_init = {0};
    GPIO_InitTypeDef gpio_init_struct = {0};

    rcc_periph_clk_init.PeriphClockSelection = RCC_PERIPHCLK_SPI45;
    rcc_periph_clk_init.Spi45ClockSelection = RCC_SPI45CLKSOURCE_PLL2Q;
    HAL_RCCEx_PeriphCLKConfig(&rcc_periph_clk_init);

    SDNAND_SPI_CLK_ENABLE();
    SDNAND_SPI_SCK_GPIO_CLK_ENABLE();
    SDNAND_SPI_MOSI_GPIO_CLK_ENABLE();
    SDNAND_SPI_MISO_GPIO_CLK_ENABLE();

    gpio_init_struct.Pin = SDNAND_SPI_SCK_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio_init_struct.Alternate = SDNAND_SPI_SCK_GPIO_AF;
    HAL_GPIO_Init(SDNAND_SPI_SCK_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = SDNAND_SPI_MOSI_GPIO_PIN;
    gpio_init_struct.Alternate = SDNAND_SPI_MOSI_GPIO_AF;
    HAL_GPIO_Init(SDNAND_SPI_MOSI_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = SDNAND_SPI_MISO_GPIO_PIN;
    gpio_init_struct.Alternate = SDNAND_SPI_MISO_GPIO_AF;
    HAL_GPIO_Init(SDNAND_SPI_MISO_GPIO_PORT, &gpio_init_struct);
}

void GPDMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_spi_dma_rx_handle[0]);
}

void GPDMA1_Channel2_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_spi_dma_tx_handle[0]);
}

void GPDMA1_Channel3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_spi_dma_rx_handle[1]);
}

void GPDMA1_Channel4_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_spi_dma_tx_handle[1]);
}

void GPDMA1_Channel5_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_spi_dma_rx_handle[2]);
}

void GPDMA1_Channel6_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_spi_dma_tx_handle[2]);
}

void SPI1_IRQHandler(void)
{
    HAL_SPI_IRQHandler(&g_spi_handle[0]);
}

void SPI2_IRQHandler(void)
{
    HAL_SPI_IRQHandler(&g_spi_handle[1]);
}

void SPI4_IRQHandler(void)
{
    HAL_SPI_IRQHandler(&g_spi_handle[2]);
}
