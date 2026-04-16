#include "./BSP/RS485/rs485.h"
#include <string.h>
#include <stdio.h>
#include "usbd_cdc_if.h"
#include "./MALLOC/malloc.h"
#include "./SYSTEM/delay/delay.h"

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
static volatile uint32_t s_ready_seq = 0U; /* even: stable, odd: writing */

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

    s_ready_seq++;
    // __DMB();
    memcpy(s_ready_buf, data, len);
    s_ready_len = len;
    s_ready_flag = 1U; /* overwrite old frame if not consumed */
    // __DMB();
    s_ready_seq++;
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
    uint16_t payload_len;
    uint16_t old_expected_len = 0U;
    uint16_t new_expected_len = 0U;

    /* Debug: confirm entering interrupt */
    static uint32_t rx_irq_cnt = 0;
    rx_irq_cnt++;
    if ((rx_irq_cnt % 10) == 0)  /* 每10次打印一次，避免刷屏 */
    {
        usb_printf("[DBG] Rx IRQ cnt=%lu, byte=0x%02X\r\n", rx_irq_cnt, s_rx_byte);
    }

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

        /*
         * Try to calculate expected frame length for both old and new formats
         * Old format: Header(1) + Addr(1) + Func(1) + Len(2) + Data(n) + XOR(1) + Tail(1) = 7 + n
         * New format: Header(1) + SrcAddr(1) + DstAddr(1) + Func(1) + Len(2) + Data(n) + XOR(1) + Tail(1) = 8 + n
         *
         * We need at least 5 bytes to calculate old format expected length
         * We need at least 6 bytes to calculate new format expected length
         *
         * The approach: calculate BOTH possible expected lengths, then check if received
         * data matches either one (ending with FRAME_END)
         */
        if ((s_rx_len >= 6U) && (s_expected_len == 0U))
        {
            /* Try old format: Len is at bytes 3-4 */
            payload_len = (uint16_t)(((uint16_t)s_rx_buf[4] << 8) | s_rx_buf[3]);
            old_expected_len = (uint16_t)(7U + payload_len); /* Old: 7 + payload */

            /* Try new format: Len is at bytes 4-5 */
            payload_len = (uint16_t)(((uint16_t)s_rx_buf[5] << 8) | s_rx_buf[4]);
            new_expected_len = (uint16_t)(8U + payload_len); /* New: 8 + payload */

            /* Debug: print received bytes */
            usb_printf("[RX] %02X %02X %02X %02X %02X %02X old_exp=%u new_exp=%u\r\n",
                       s_rx_buf[0], s_rx_buf[1], s_rx_buf[2],
                       s_rx_buf[3], s_rx_buf[4], s_rx_buf[5],
                       old_expected_len, new_expected_len);

            /* Validate expected lengths */
            if ((old_expected_len >= 7U) && (old_expected_len <= RS485_RX_BUF_LEN))
            {
                /* Old format looks valid */
            }
            else
            {
                old_expected_len = 0U; /* Invalid */
            }

            if ((new_expected_len >= 8U) && (new_expected_len <= RS485_RX_BUF_LEN))
            {
                /* New format looks valid */
            }
            else
            {
                new_expected_len = 0U; /* Invalid */
            }

            /* Choose the expected length:
             * - If both are valid, wait until one matches
             * - If only one is valid, use that one
             * - If neither is valid, keep waiting for more data or timeout
             */
            if ((old_expected_len > 0U) && (new_expected_len > 0U))
            {
                /* Both could be valid - prefer the one that ends with FRAME_END */
                /* Check if we've received exactly the old format length */
                if (s_rx_len == old_expected_len)
                {
                    if (s_rx_buf[s_rx_len - 1U] == RS485_FRAME_END)
                    {
                        /* Old format frame complete */
                        s_expected_len = old_expected_len;
                    }
                    else
                    {
                        /* Not old format, try new format if possible */
                        if (s_rx_len >= new_expected_len)
                        {
                            if (s_rx_buf[new_expected_len - 1U] == RS485_FRAME_END)
                            {
                                s_expected_len = new_expected_len;
                            }
                        }
                        else
                        {
                            /* Need more data for new format */
                            s_expected_len = new_expected_len;
                        }
                    }
                }
                else if (s_rx_len == new_expected_len)
                {
                    if (s_rx_buf[s_rx_len - 1U] == RS485_FRAME_END)
                    {
                        /* New format frame complete */
                        s_expected_len = new_expected_len;
                    }
                    else
                    {
                        /* Not new format, maybe old format has extra bytes? */
                        if (old_expected_len < s_rx_len && s_rx_buf[old_expected_len - 1U] == RS485_FRAME_END)
                        {
                            s_expected_len = old_expected_len;
                        }
                    }
                }
                else if (old_expected_len < new_expected_len)
                {
                    /* Old format is shorter, set it first and wait for bytes */
                    s_expected_len = old_expected_len;
                }
                else
                {
                    /* New format is shorter or equal */
                    s_expected_len = new_expected_len;
                }
            }
            else if (old_expected_len > 0U)
            {
                s_expected_len = old_expected_len;
            }
            else if (new_expected_len > 0U)
            {
                s_expected_len = new_expected_len;
            }
            /* else: keep s_expected_len = 0, wait for more data */
        }
        else if ((s_rx_len >= 5U) && (s_expected_len == 0U))
        {
            /* Only have 5 bytes, can only calculate old format */
            payload_len = (uint16_t)(((uint16_t)s_rx_buf[4] << 8) | s_rx_buf[3]);
            s_expected_len = (uint16_t)(7U + payload_len); /* Old: 7 + payload */

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
        else if ((s_expected_len > 0U) && (s_rx_len > s_expected_len))
        {
            /* Received more than expected - frame error */
            rs485_reset_parser();
        }
    }

restart_rx_it:
    HAL_UART_Receive_IT(&rs485_uartx_handle, &s_rx_byte, 1U);
}

int8_t rs485_send_raw(const uint8_t *buf, uint16_t len)
{
    HAL_StatusTypeDef hret;
    uint16_t i;
    char *hex_str = NULL;
    int pos;
    int str_size;

    if ((buf == NULL) || (len == 0U))
    {
        return -1;
    }

    /* 动态申请内存构建 hex 字符串 */
    str_size = len * 3 + 32;
    hex_str = (char *)mymalloc(SRAMIN, str_size);
    if (hex_str != NULL)
    {
        /* 构建 hex 字符串 */
        pos = snprintf(hex_str, str_size, "RS485 TX [len=%d]: ", len);
        for (i = 0U; i < len && pos < str_size - 4; i++)
        {
            pos += snprintf(hex_str + pos, str_size - pos, "%02X ", buf[i]);
        }
        strcat(hex_str, "\r\n");
        usb_printf("%s", hex_str);

        myfree(SRAMIN, hex_str);
    }

    RS485_RE(1);
    hret = HAL_UART_Transmit(&rs485_uartx_handle, (uint8_t *)buf, len, 1000U);

    /* Wait for transmission complete (TC flag) before switching to RX mode.
     * This ensures the last byte has left the shift register and is on the bus.
     * Critical for RS485 half-duplex communication.
     */
    if (hret == HAL_OK)
    {
        uint32_t tc_start = HAL_GetTick();
        while (!__HAL_UART_GET_FLAG(&rs485_uartx_handle, UART_FLAG_TC))
        {
            if ((HAL_GetTick() - tc_start) > 100U)  /* 100ms timeout for TC */
            {
                hret = HAL_TIMEOUT;
                break;
            }
        }
    }

    /* Add small delay for RS485 transceiver direction change */
    delay_us(100);  /* 100us delay, adjust based on your transceiver specs */

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
    uint32_t seq1;
    uint32_t seq2;
    uint16_t len;
    uint16_t i;

    if ((buf == NULL) || (out_len == NULL))
    {
        return -1;
    }

    if (s_ready_flag == 0U)
    {
        *out_len = 0U;
        return -1;
    }

    while (1)
    {
        seq1 = s_ready_seq;
        if (seq1 & 1U)
        {
            continue;
        }

        // __DMB();
        len = s_ready_len;
        if ((len == 0U) || (len > buf_size))
        {
            *out_len = 0U;
            return -1;
        }
        memcpy(buf, s_ready_buf, len);
        // __DMB();
        seq2 = s_ready_seq;
        if (seq1 == seq2)
        {
            break;
        }
    }

    /* 调试：打印从驱动层读取到的原始帧数据 */
    {
        char dbg_buf[512];
        int dbg_pos = 0;
        dbg_pos += sprintf(dbg_buf + dbg_pos, "[DBG] rs485_read_raw_frame: s_ready_flag=%d, len=%d, data=", s_ready_flag, len);
        for (i = 0U; i < len; i++)
        {
            dbg_pos += sprintf(dbg_buf + dbg_pos, "%02X ", s_ready_buf[i]);
            if (dbg_pos >= sizeof(dbg_buf) - 10) /* 预留空间避免溢出 */
            {
                break;
            }
        }
        dbg_pos += sprintf(dbg_buf + dbg_pos, "\r\n");
        usb_printf("%s", dbg_buf);
    }

    *out_len = len;
    s_ready_flag = 0U;
    s_ready_len = 0U;

    return 0;
}

uint8_t rs485_get_pending_frames(void)
{
    return s_ready_flag ? 1U : 0U;
}

void rs485_clear_pending_frames(void)
{
    s_ready_flag = 0U;
    s_ready_len = 0U;
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
