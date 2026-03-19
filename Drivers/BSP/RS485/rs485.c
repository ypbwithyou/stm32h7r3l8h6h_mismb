#include "./BSP/RS485/rs485.h"
#include <string.h>

UART_HandleTypeDef rs485_uartx_handle = {0};

static uint8_t s_rx_byte = 0U;
static uint8_t s_assembling = 0U;
static uint8_t s_rx_buf[RS485_RX_BUF_LEN];
static uint16_t s_rx_len = 0U;
static uint16_t s_expected_len = 0U;
static uint32_t s_last_rx_tick = 0U;

/* Single ready frame slot (no multi-frame queue) */
static uint8_t s_ready_buf[RS485_RX_BUF_LEN];
static uint16_t s_ready_len = 0U;
static uint8_t s_ready_flag = 0U;

static void rs485_reset_parser(void)
{
    s_assembling = 0U;
    s_rx_len = 0U;
    s_expected_len = 0U;
}

static void rs485_publish_frame(const uint8_t *data, uint16_t len)
{
    if ((data == NULL) || (len < 7U) || (len > RS485_RX_BUF_LEN))
    {
        return;
    }

    __disable_irq();
    memcpy(s_ready_buf, data, len);
    s_ready_len = len;
    s_ready_flag = 1U; /* overwrite old frame if not consumed */
    __enable_irq();
}

void rs485_init(uint32_t baudrate)
{
    GPIO_InitTypeDef gpio_init_struct;
    RCC_PeriphCLKInitTypeDef usart2_clk_init = {0};

    usart2_clk_init.PeriphClockSelection = RCC_PERIPHCLK_USART234578;
    usart2_clk_init.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_PCLK1;
    HAL_RCCEx_PeriphCLKConfig(&usart2_clk_init);

    RS485_UARTX_CLK_ENABLE();
    RS485_UARTX_TX_GPIO_CLK_ENABLE();
    RS485_UARTX_RX_GPIO_CLK_ENABLE();
    RS485_RE_GPIO_CLK_ENABLE();

    gpio_init_struct.Pin = RS485_UARTX_TX_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Alternate = RS485_UARTX_TX_GPIO_AF;
    HAL_GPIO_Init(RS485_UARTX_TX_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = RS485_UARTX_RX_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Alternate = RS485_UARTX_RX_GPIO_AF;
    HAL_GPIO_Init(RS485_UARTX_RX_GPIO_PORT, &gpio_init_struct);

    gpio_init_struct.Pin = RS485_RE_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(RS485_RE_GPIO_PORT, &gpio_init_struct);

    HAL_NVIC_SetPriority(RS485_UARTX_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(RS485_UARTX_IRQn);

    rs485_uartx_handle.Instance = RS485_UARTX;
    rs485_uartx_handle.Init.BaudRate = baudrate;
    rs485_uartx_handle.Init.WordLength = UART_WORDLENGTH_8B;
    rs485_uartx_handle.Init.StopBits = UART_STOPBITS_1;
    rs485_uartx_handle.Init.Parity = UART_PARITY_NONE;
    rs485_uartx_handle.Init.Mode = UART_MODE_TX_RX;
    rs485_uartx_handle.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    rs485_uartx_handle.Init.OverSampling = UART_OVERSAMPLING_16;
    rs485_uartx_handle.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    rs485_uartx_handle.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    HAL_UART_Init(&rs485_uartx_handle);

    RS485_RE(0);
    rs485_reset_parser();
    rs485_clear_pending_frames();
    s_last_rx_tick = HAL_GetTick();
    HAL_UART_Receive_IT(&rs485_uartx_handle, &s_rx_byte, 1U);
}

void RS485_UARTX_IRQHandler(void)
{
    HAL_UART_IRQHandler(&rs485_uartx_handle);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uint32_t now_tick;

    if (huart->Instance != RS485_UARTX)
    {
        return;
    }

    now_tick = HAL_GetTick();
    if ((now_tick - s_last_rx_tick) > RS485_RX_TIMEOUT_MS)
    {
        rs485_reset_parser();
    }
    s_last_rx_tick = now_tick;

    if (s_assembling == 0U)
    {
        if (s_rx_byte == RS485_FRAME_HEAD)
        {
            s_assembling = 1U;
            s_rx_len = 0U;
            s_expected_len = 0U;
            s_rx_buf[s_rx_len++] = s_rx_byte;
        }
    }
    else
    {
        if (s_rx_len >= RS485_RX_BUF_LEN)
        {
            rs485_reset_parser();
            goto restart_rx_it;
        }

        s_rx_buf[s_rx_len++] = s_rx_byte;

        if ((s_rx_len >= 5U) && (s_expected_len == 0U))
        {
            uint16_t payload_len = (uint16_t)(((uint16_t)s_rx_buf[3] << 8) | s_rx_buf[4]);
            s_expected_len = (uint16_t)(7U + payload_len);
            if ((s_expected_len < 7U) || (s_expected_len > RS485_RX_BUF_LEN))
            {
                rs485_reset_parser();
                goto restart_rx_it;
            }
        }

        if ((s_expected_len > 0U) && (s_rx_len == s_expected_len))
        {
            if (s_rx_buf[s_rx_len - 1U] == RS485_FRAME_END)
            {
                rs485_publish_frame(s_rx_buf, s_rx_len);
            }
            rs485_reset_parser();
        }
    }

restart_rx_it:
    HAL_UART_Receive_IT(&rs485_uartx_handle, &s_rx_byte, 1U);
}

int8_t rs485_send_raw(const uint8_t *buf, uint16_t len)
{
    HAL_StatusTypeDef hret;

    if ((buf == NULL) || (len == 0U))
    {
        return -1;
    }

    RS485_RE(1);
    hret = HAL_UART_Transmit(&rs485_uartx_handle, (uint8_t *)buf, len, 1000U);
    RS485_RE(0);

    return (hret == HAL_OK) ? 0 : -1;
}

int8_t rs485_recv_frame_poll(uint8_t *buf, uint16_t buf_size, uint16_t *out_len, uint32_t frame_timeout_ms)
{
    uint32_t start = HAL_GetTick();

    if ((buf == NULL) || (out_len == NULL))
    {
        return -1;
    }

    do
    {
        if (rs485_read_raw_frame(buf, buf_size, out_len) == 0)
        {
            return 0;
        }
    } while ((HAL_GetTick() - start) < frame_timeout_ms);

    *out_len = 0U;
    return -1;
}

int8_t rs485_read_raw_frame(uint8_t *buf, uint16_t buf_size, uint16_t *out_len)
{
    if ((buf == NULL) || (out_len == NULL))
    {
        return -1;
    }

    __disable_irq();
    if ((s_ready_flag == 0U) || (s_ready_len == 0U) || (s_ready_len > buf_size))
    {
        __enable_irq();
        *out_len = 0U;
        return -1;
    }

    memcpy(buf, s_ready_buf, s_ready_len);
    *out_len = s_ready_len;
    s_ready_flag = 0U;
    s_ready_len = 0U;
    __enable_irq();

    return 0;
}

uint8_t rs485_get_pending_frames(void)
{
    uint8_t flag;
    __disable_irq();
    flag = s_ready_flag;
    __enable_irq();
    return flag ? 1U : 0U;
}

void rs485_clear_pending_frames(void)
{
    __disable_irq();
    s_ready_flag = 0U;
    s_ready_len = 0U;
    __enable_irq();
}

void rs485_send_data(uint8_t *buf, uint8_t len)
{
    (void)rs485_send_raw(buf, len);
}

int8_t rs485_recv_data(uint8_t *buf, uint8_t *len)
{
    uint16_t out_len = 0U;
    if ((buf == NULL) || (len == NULL))
    {
        return -1;
    }

    if (rs485_read_raw_frame(buf, RS485_RX_BUF_LEN, &out_len) != 0)
    {
        *len = 0U;
        return -1;
    }
    if (out_len > 255U)
    {
        *len = 0U;
        return -1;
    }

    *len = (uint8_t)out_len;
    return 0;
}
