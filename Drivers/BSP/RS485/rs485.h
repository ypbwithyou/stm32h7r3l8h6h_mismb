#ifndef __RS485_H
#define __RS485_H

#include "./SYSTEM/sys/sys.h"
#include <stdint.h>
#include "dataType.h"

/* GPIO/UART mapping */
#define RS485_RE_GPIO_PORT                  GPIOF
#define RS485_RE_GPIO_PIN                   GPIO_PIN_11
#define RS485_RE_GPIO_CLK_ENABLE()          do { __HAL_RCC_GPIOF_CLK_ENABLE(); } while (0)

#define RS485_UARTX                         USART2
#define RS485_UARTX_IRQn                    USART2_IRQn
#define RS485_UARTX_IRQHandler              USART2_IRQHandler
#define RS485_UARTX_CLK_ENABLE()            do { __HAL_RCC_USART2_CLK_ENABLE(); } while (0)

#define RS485_UARTX_TX_GPIO_PORT            GPIOD
#define RS485_UARTX_TX_GPIO_PIN             GPIO_PIN_6
#define RS485_UARTX_TX_GPIO_AF              GPIO_AF7_USART2
#define RS485_UARTX_TX_GPIO_CLK_ENABLE()    do { __HAL_RCC_GPIOD_CLK_ENABLE(); } while (0)

#define RS485_UARTX_RX_GPIO_PORT            GPIOD
#define RS485_UARTX_RX_GPIO_PIN             GPIO_PIN_5
#define RS485_UARTX_RX_GPIO_AF              GPIO_AF7_USART2
#define RS485_UARTX_RX_GPIO_CLK_ENABLE()    do { __HAL_RCC_GPIOD_CLK_ENABLE(); } while (0)

/* RE pin: 0=RX mode, 1=TX mode */
#define RS485_RE(x) do { \
    if (x) { \
        HAL_GPIO_WritePin(RS485_RE_GPIO_PORT, RS485_RE_GPIO_PIN, GPIO_PIN_SET); \
    } else { \
        HAL_GPIO_WritePin(RS485_RE_GPIO_PORT, RS485_RE_GPIO_PIN, GPIO_PIN_RESET); \
    } \
} while (0)

/* Protocol delimiters */
#define RS485_FRAME_HEAD 0x3CU
#define RS485_FRAME_END  0x3EU
#define RS485_MASTER_ADDR 0U
#define RS485_SLAVE_ADDR_MIN 1U
#define RS485_SLAVE_ADDR_MAX 8U

/* Buffers/timeout */
#define RS485_RX_BUF_LEN        256U
#define RS485_RX_TIMEOUT_MS     20U

void rs485_init(uint32_t baudrate);

/* Low-level raw send/read (contains whole frame bytes) */
int8_t rs485_send_raw(const uint8_t *buf, uint16_t len);
int8_t rs485_recv_frame_poll(uint8_t *buf, uint16_t buf_size, uint16_t *out_len, uint32_t frame_timeout_ms);

/* Compatibility aliases for old call sites */
int8_t rs485_read_raw_frame(uint8_t *buf, uint16_t buf_size, uint16_t *out_len);
uint8_t rs485_get_pending_frames(void); /* always 0 in polling mode */
void rs485_clear_pending_frames(void);  /* no-op in polling mode */

/* Backward-compatible wrappers */
void rs485_send_data(uint8_t *buf, uint8_t len);
int8_t rs485_recv_data(uint8_t *buf, uint8_t *len);

#endif
