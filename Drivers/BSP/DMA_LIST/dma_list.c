#include "./BSP/DMA_LIST/dma_list.h"

#include "./BSP/ADS8319/ads8319.h"
#include "collector_processor.h"
#include <string.h>

#define DMA_SPI_CHANNELS 3U
#define DMA_SPI_FRAME_BYTES TOTAL_DATA_BYTES
#define DMA_SPI_CACHE_LINE_BYTES 32U

static DMA_HandleTypeDef g_spi_dma_tx[DMA_SPI_CHANNELS];
static DMA_HandleTypeDef g_spi_dma_rx[DMA_SPI_CHANNELS];

static const DMA_Channel_TypeDef *g_spi_dma_tx_instance[DMA_SPI_CHANNELS] = {
    GPDMA1_Channel0, GPDMA1_Channel2, GPDMA1_Channel4};
static const DMA_Channel_TypeDef *g_spi_dma_rx_instance[DMA_SPI_CHANNELS] = {
    GPDMA1_Channel1, GPDMA1_Channel3, GPDMA1_Channel5};
static const uint32_t g_spi_dma_tx_request[DMA_SPI_CHANNELS] = {
    GPDMA1_REQUEST_SPI1_TX, GPDMA1_REQUEST_SPI2_TX, GPDMA1_REQUEST_SPI4_TX};
static const uint32_t g_spi_dma_rx_request[DMA_SPI_CHANNELS] = {
    GPDMA1_REQUEST_SPI1_RX, GPDMA1_REQUEST_SPI2_RX, GPDMA1_REQUEST_SPI4_RX};
static const IRQn_Type g_spi_dma_tx_irqn[DMA_SPI_CHANNELS] = {
    GPDMA1_Channel0_IRQn, GPDMA1_Channel2_IRQn, GPDMA1_Channel4_IRQn};
static const IRQn_Type g_spi_dma_rx_irqn[DMA_SPI_CHANNELS] = {
    GPDMA1_Channel1_IRQn, GPDMA1_Channel3_IRQn, GPDMA1_Channel5_IRQn};

static volatile uint8_t g_dma_frame_busy = 0U;
static volatile uint8_t g_dma_frame_done_mask = 0U;
static volatile uint8_t g_dma_frame_active_mask = 0U;
static volatile uint8_t g_dma_frame_error = 0U;
static volatile uint8_t g_dma_frame_bytes[DMA_SPI_CHANNELS] = {0U};
static volatile uint32_t g_dma_frame_seq = 0U;
static volatile uint32_t g_dma_frame_start_tick = 0U;
static volatile uint32_t g_dma_stat_start_req = 0U;
static volatile uint32_t g_dma_stat_start_ok = 0U;
static volatile uint32_t g_dma_stat_start_fail = 0U;
static volatile uint32_t g_dma_stat_cplt_irq = 0U;
static volatile uint32_t g_dma_stat_timeout = 0U;
static volatile uint32_t g_dma_stat_error = 0U;
static volatile uint32_t g_dma_stat_start_busy = 0U;
static volatile int32_t g_dma_last_start_status = 0;

__ALIGNED(32) static uint8_t g_spi_dma_tx_buf[DMA_SPI_CACHE_LINE_BYTES];
__ALIGNED(32) static uint8_t g_spi_dma_rx_buf[DMA_SPI_CHANNELS][DMA_SPI_CACHE_LINE_BYTES];

static void dma_spi_config_channel(DMA_HandleTypeDef *hdma,
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

    HAL_DMA_Init(hdma);
    HAL_DMA_ConfigChannelAttributes(hdma, DMA_CHANNEL_NPRIV);
}

static void dma_spi_force_recover(uint8_t spi)
{
    SPI_HandleTypeDef *hspi = &g_spi_handle[spi];

    /* Stop SPI DMA request path and clear sticky flags/state so next frame can restart cleanly. */
    (void)HAL_SPI_DMAStop(hspi);
    __HAL_SPI_DISABLE(hspi);
    __HAL_SPI_CLEAR_EOTFLAG(hspi);
    __HAL_SPI_CLEAR_TXTFFLAG(hspi);
    hspi->ErrorCode = HAL_SPI_ERROR_NONE;
    hspi->State = HAL_SPI_STATE_READY;
    __HAL_UNLOCK(hspi);
}

static void dma_spi_finish_frame_if_ready(void)
{
    uint8_t expected_mask;

    if (g_dma_frame_busy == 0U)
    {
        return;
    }

    expected_mask = g_dma_frame_active_mask;
    if ((expected_mask == 0U) || (g_dma_frame_done_mask != expected_mask))
    {
        return;
    }

    ads8319_stop_transfer();

    for (uint8_t spi = 0; spi < DMA_SPI_CHANNELS; spi++)
    {
        uint8_t adc_num = g_spi_adc_cnt[spi];
        uint8_t byte_len = g_dma_frame_bytes[spi];

        if ((expected_mask & (1U << spi)) == 0U)
        {
            continue;
        }

        SCB_InvalidateDCache_by_Addr((uint32_t *)g_spi_dma_rx_buf[spi], DMA_SPI_CACHE_LINE_BYTES);
        memcpy(spi_rx_buffer[spi], g_spi_dma_rx_buf[spi], byte_len);

        for (uint8_t pos = 0; pos < adc_num; pos++)
        {
            uint16_t sample = ((uint16_t)g_spi_dma_rx_buf[spi][2U * pos] << 8) |
                              g_spi_dma_rx_buf[spi][2U * pos + 1U];
            uint8_t ch = pos * SPI_NUM + spi;

            if (ch >= ADC_CH_TOTAL)
            {
                continue;
            }
            if ((g_ch_enable_mask & (1UL << ch)) == 0U)
            {
                continue;
            }

            cb_write(g_cb_ch[ch], (const char *)&sample, ADC_DATA_LEN);
        }
    }

    g_dma_frame_busy = 0U;
    g_dma_frame_active_mask = 0U;
    g_dma_frame_done_mask = 0U;
}

static uint8_t dma_spi_index_from_handle(SPI_HandleTypeDef *hspi)
{
    if (hspi == &g_spi_handle[0])
    {
        return 0U;
    }
    if (hspi == &g_spi_handle[1])
    {
        return 1U;
    }
    if (hspi == &g_spi_handle[2])
    {
        return 2U;
    }
    return 0xFFU;
}

void dma_list_rtx_init(void)
{
    __HAL_RCC_GPDMA1_CLK_ENABLE();

    memset(g_spi_dma_tx_buf, 0xFF, sizeof(g_spi_dma_tx_buf));
    memset(g_spi_dma_rx_buf, 0x00, sizeof(g_spi_dma_rx_buf));

    for (uint8_t spi = 0U; spi < DMA_SPI_CHANNELS; spi++)
    {
        dma_spi_config_channel(&g_spi_dma_tx[spi],
                               (DMA_Channel_TypeDef *)g_spi_dma_tx_instance[spi],
                               g_spi_dma_tx_request[spi],
                               DMA_MEMORY_TO_PERIPH);
        dma_spi_config_channel(&g_spi_dma_rx[spi],
                               (DMA_Channel_TypeDef *)g_spi_dma_rx_instance[spi],
                               g_spi_dma_rx_request[spi],
                               DMA_PERIPH_TO_MEMORY);

        __HAL_LINKDMA(&g_spi_handle[spi], hdmatx, g_spi_dma_tx[spi]);
        __HAL_LINKDMA(&g_spi_handle[spi], hdmarx, g_spi_dma_rx[spi]);

        HAL_NVIC_SetPriority(g_spi_dma_tx_irqn[spi], 0, 0);
        HAL_NVIC_EnableIRQ(g_spi_dma_tx_irqn[spi]);
        HAL_NVIC_SetPriority(g_spi_dma_rx_irqn[spi], 0, 0);
        HAL_NVIC_EnableIRQ(g_spi_dma_rx_irqn[spi]);
    }

    g_dma_frame_busy = 0U;
    g_dma_frame_done_mask = 0U;
    g_dma_frame_active_mask = 0U;
    g_dma_frame_error = 0U;
    g_dma_frame_seq = 0U;
    g_dma_frame_start_tick = 0U;
    g_dma_stat_start_req = 0U;
    g_dma_stat_start_ok = 0U;
    g_dma_stat_start_fail = 0U;
    g_dma_stat_cplt_irq = 0U;
    g_dma_stat_timeout = 0U;
    g_dma_stat_error = 0U;
    g_dma_stat_start_busy = 0U;
    g_dma_last_start_status = 0;
}

uint8_t dma_ads8319_frame_busy(void)
{
    return g_dma_frame_busy;
}

HAL_StatusTypeDef dma_ads8319_start_frame(void)
{
    HAL_StatusTypeDef status = HAL_OK;
    uint8_t active_mask = 0U;
    g_dma_stat_start_req++;

    if (g_dma_frame_busy != 0U)
    {
        g_dma_stat_start_busy++;
        g_dma_last_start_status = HAL_BUSY;
        return HAL_BUSY;
    }

    for (uint8_t spi = 0U; spi < DMA_SPI_CHANNELS; spi++)
    {
        uint8_t adc_num = g_spi_adc_cnt[spi];
        uint8_t byte_len;

        if (adc_num == 0U)
        {
            g_dma_frame_bytes[spi] = 0U;
            continue;
        }

        byte_len = (uint8_t)(((uint16_t)adc_num * ADS8319_DATA_BITS + 7U) / 8U);
        g_dma_frame_bytes[spi] = byte_len;
        memset(g_spi_dma_rx_buf[spi], 0x00, sizeof(g_spi_dma_rx_buf[spi]));
        active_mask |= (uint8_t)(1U << spi);
    }

    if (active_mask == 0U)
    {
        g_dma_last_start_status = HAL_ERROR;
        return HAL_ERROR;
    }

    SCB_CleanDCache_by_Addr((uint32_t *)g_spi_dma_tx_buf, DMA_SPI_CACHE_LINE_BYTES);

    g_dma_frame_busy = 1U;
    g_dma_frame_done_mask = 0U;
    g_dma_frame_active_mask = active_mask;
    g_dma_frame_error = 0U;
    g_dma_frame_seq++;
    g_dma_frame_start_tick = HAL_GetTick();

    for (uint8_t spi = 0U; spi < DMA_SPI_CHANNELS; spi++)
    {
        if ((active_mask & (1U << spi)) == 0U)
        {
            continue;
        }

        status = HAL_SPI_TransmitReceive_DMA(&g_spi_handle[spi],
                                             g_spi_dma_tx_buf,
                                             g_spi_dma_rx_buf[spi],
                                             g_dma_frame_bytes[spi]);
        if (status != HAL_OK)
        {
            g_dma_frame_busy = 0U;
            g_dma_frame_active_mask = 0U;
            g_dma_frame_done_mask = 0U;
            g_dma_frame_error = 1U;
            g_dma_stat_start_fail++;
            g_dma_last_start_status = status;
            return status;
        }
    }

    g_dma_stat_start_ok++;
    g_dma_last_start_status = HAL_OK;
    return HAL_OK;
}

void dma_start_transfer(void)
{
    (void)dma_ads8319_start_frame();
}

void dma_list_data_init(void)
{
}

uint8_t dma_get_ready_data(uint32_t *node_index)
{
    (void)node_index;
    return 0U;
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    uint8_t spi = dma_spi_index_from_handle(hspi);

    if (spi >= DMA_SPI_CHANNELS)
    {
        return;
    }

    g_dma_frame_done_mask |= (uint8_t)(1U << spi);
    g_dma_stat_cplt_irq++;

    dma_spi_finish_frame_if_ready();
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    uint8_t spi = dma_spi_index_from_handle(hspi);

    if (spi >= DMA_SPI_CHANNELS)
    {
        return;
    }

    g_dma_frame_error = 1U;
    ads8319_stop_transfer();
    g_dma_frame_busy = 0U;
    g_dma_frame_active_mask = 0U;
    g_dma_frame_done_mask = 0U;
    g_dma_stat_error++;
}

void dma_ads8319_watchdog_kick(void)
{
    uint32_t now;
    const uint32_t timeout_ms = 2U;

    if (g_dma_frame_busy == 0U)
    {
        return;
    }

    now = HAL_GetTick();
    if ((now - g_dma_frame_start_tick) < timeout_ms)
    {
        return;
    }
    g_dma_stat_timeout++;

    for (uint8_t spi = 0U; spi < DMA_SPI_CHANNELS; spi++)
    {
        if ((g_dma_frame_active_mask & (1U << spi)) == 0U)
        {
            continue;
        }

        dma_spi_force_recover(spi);
    }

    ads8319_stop_transfer();
    g_dma_frame_busy = 0U;
    g_dma_frame_active_mask = 0U;
    g_dma_frame_done_mask = 0U;
    g_dma_frame_error = 1U;
}

void dma_ads8319_get_stats(uint32_t *start_req,
                           uint32_t *start_ok,
                           uint32_t *start_fail,
                           uint32_t *cplt_irq,
                           uint32_t *timeouts,
                           uint32_t *errors)
{
    if (start_req)
    {
        *start_req = g_dma_stat_start_req;
    }
    if (start_ok)
    {
        *start_ok = g_dma_stat_start_ok;
    }
    if (start_fail)
    {
        *start_fail = g_dma_stat_start_fail;
    }
    if (cplt_irq)
    {
        *cplt_irq = g_dma_stat_cplt_irq;
    }
    if (timeouts)
    {
        *timeouts = g_dma_stat_timeout;
    }
    if (errors)
    {
        *errors = g_dma_stat_error;
    }
}

int32_t dma_ads8319_get_last_start_status(void)
{
    return g_dma_last_start_status;
}

uint32_t dma_ads8319_get_start_busy_count(void)
{
    return g_dma_stat_start_busy;
}

void GPDMA1_Channel0_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_spi_dma_tx[0]);
}

void GPDMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_spi_dma_rx[0]);
}

void GPDMA1_Channel2_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_spi_dma_tx[1]);
}

void GPDMA1_Channel3_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_spi_dma_rx[1]);
}

void GPDMA1_Channel4_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_spi_dma_tx[2]);
}

void GPDMA1_Channel5_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&g_spi_dma_rx[2]);
}
