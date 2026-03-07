#include "./BSP/RS485/rs485.h"
#include "./SYSTEM/delay/delay.h"
#include <string.h>
#include <stdlib.h>

/* UART句柄 */
UART_HandleTypeDef rs485_uartx_handle = {0};

/* 裸机环境下的接收帧管理 */
#define MAX_PENDING_FRAMES 10  /* 最大待处理帧数 */
static uint8_t s_pending_frames[MAX_PENDING_FRAMES][RS485_RX_BUF_LEN];  /* 帧缓冲区 */
static uint16_t s_frame_lengths[MAX_PENDING_FRAMES];                    /* 帧长度 */
static uint8_t s_frame_read_index = 0;                                  /* 读索引 */
static uint8_t s_frame_write_index = 0;                                 /* 写索引 */
static uint8_t s_frame_count = 0;                                       /* 当前帧数量 */

/* 接收缓冲区相关定义 */
static uint8_t s_rx_byte = 0;                                           /* 中断接收的单个字节 */
static uint8_t s_rx_frame_buffer[RS485_RX_BUF_LEN];                     /* 帧缓冲区 */
static uint16_t s_rx_frame_index = 0;                                   /* 帧缓冲区索引（16位以防长帧） */
static rs485_rx_state_t s_rx_state = RX_STATE_IDLE;                     /* 当前接收状态 */
static uint32_t s_frame_timeout_tick = 0;                               /* 帧超时时间戳 */

/**
 * @brief   重置接收状态机
 * @param   无
 * @retval  无
 */
static void rs485_reset_rx_state(void)
{
    s_rx_state = RX_STATE_IDLE;
    s_rx_frame_index = 0;
}

/**
 * @brief   初始化帧缓冲区
 * @param   无
 * @retval  无
 */
static void rs485_init_frame_buffer(void)
{
    s_frame_read_index = 0;
    s_frame_write_index = 0;
    s_frame_count = 0;
    for (int i = 0; i < MAX_PENDING_FRAMES; i++) {
        s_frame_lengths[i] = 0;
    }
}

/**
 * @brief   向帧缓冲区添加一帧数据
 * @param   data: 数据指针
 * @param   len: 数据长度
 * @retval  0:成功, -1:缓冲区满
 */
static int8_t rs485_add_frame_to_buffer(uint8_t *data, uint16_t len)
{
    if (s_frame_count >= MAX_PENDING_FRAMES) {
        return -1;  /* 缓冲区满 */
    }
    
    /* 检查长度是否合法 */
    if (len > RS485_RX_BUF_LEN) {
        len = RS485_RX_BUF_LEN;  /* 截断超长数据 */
    }
    
    /* 拷贝数据 */
    if (len > 0) {
        memcpy(s_pending_frames[s_frame_write_index], data, len);
    }
    
    /* 存储长度 */
    s_frame_lengths[s_frame_write_index] = len;
    
    /* 更新索引和计数 */
    s_frame_write_index = (s_frame_write_index + 1) % MAX_PENDING_FRAMES;
    s_frame_count++;
    
    return 0;
}

/**
 * @brief   从帧缓冲区读取一帧数据
 * @param   buf: 输出缓冲区
 * @param   len: 输出参数，实际读取的长度
 * @retval  0:成功, -1:缓冲区空
 */
static int8_t rs485_read_frame_from_buffer(uint8_t *buf, uint8_t *len)
{
    if (s_frame_count == 0) {
        *len = 0;
        return -1;  /* 缓冲区空 */
    }
    
    /* 获取数据长度 */
    uint16_t data_len = s_frame_lengths[s_frame_read_index];
    
    /* 检查长度是否合法 */
    if (data_len > RS485_RX_BUF_LEN) {
        data_len = RS485_RX_BUF_LEN;
    }
    
    /* 拷贝数据 */
    if (data_len > 0) {
        memcpy(buf, s_pending_frames[s_frame_read_index], data_len);
    }
    
    /* 更新输出参数 */
    *len = data_len;
    
    /* 更新索引和计数 */
    s_frame_read_index = (s_frame_read_index + 1) % MAX_PENDING_FRAMES;
    s_frame_count--;
    
    return 0;
}

/**
 * @brief   初始化RS485
 * @param   baudrate: 通信波特率
 * @retval  无
 */
void rs485_init(uint32_t baudrate)
{
    GPIO_InitTypeDef gpio_init_struct;
    RCC_PeriphCLKInitTypeDef usart2_clk_init = {0};
    
    /* 初始化帧缓冲区 */
    rs485_init_frame_buffer();
    
    /* 配置时钟 */
    usart2_clk_init.PeriphClockSelection |= RCC_PERIPHCLK_USART234578;
    usart2_clk_init.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_PCLK1;
    HAL_RCCEx_PeriphCLKConfig(&usart2_clk_init);
    
    /* 使能时钟 */
    RS485_UARTX_CLK_ENABLE();
    RS485_UARTX_TX_GPIO_CLK_ENABLE();
    RS485_UARTX_RX_GPIO_CLK_ENABLE();
    RS485_RE_GPIO_CLK_ENABLE();         /* 使能 RS485_RE 脚时钟 */
    
    /* 初始化TX引脚 */
    gpio_init_struct.Pin = RS485_UARTX_TX_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Alternate = RS485_UARTX_TX_GPIO_AF;
    HAL_GPIO_Init(RS485_UARTX_TX_GPIO_PORT, &gpio_init_struct);
    
    /* 初始化RX引脚 */
    gpio_init_struct.Pin = RS485_UARTX_RX_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Alternate = RS485_UARTX_RX_GPIO_AF;
    HAL_GPIO_Init(RS485_UARTX_RX_GPIO_PORT, &gpio_init_struct);
    
    /* 初始化RE引脚 */
    gpio_init_struct.Pin = RS485_RE_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(RS485_RE_GPIO_PORT, &gpio_init_struct);
    
    /* 配置UART中断 */
    HAL_NVIC_SetPriority(RS485_UARTX_IRQn, 4, 0);
    HAL_NVIC_EnableIRQ(RS485_UARTX_IRQn);
    
    /* 初始化UART */
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
    
    /* 默认为接收模式 */
    RS485_RE(0);        
    
    /* 启动第一次中断接收 */
    HAL_UART_Receive_IT(&rs485_uartx_handle, &s_rx_byte, 1);
    
    /* 初始化接收状态 */
    rs485_reset_rx_state();
    s_frame_timeout_tick = HAL_GetTick();  /* 使用HAL的tick函数 */
}

/**
 * @brief   UART中断服务函数
 * @param   无
 * @retval  无
 */
void RS485_UARTX_IRQHandler(void)
{
    /* 只调用HAL库的处理函数 */
    HAL_UART_IRQHandler(&rs485_uartx_handle);
}

/**
 * @brief   处理接收到的完整帧
 * @param   无
 * @retval  无
 */
static void rs485_process_complete_frame(void)
{
    /* 注意：s_rx_frame_index 现在包含整个帧的长度 */
    if (s_rx_frame_index >= 7) { /* 帧头+设备号+功能码+长度+数据+校验+帧尾 */
        /* 将完整帧添加到帧缓冲区 */
        rs485_add_frame_to_buffer(s_rx_frame_buffer, s_rx_frame_index);
    }
    
    /* 重置接收状态 */
    rs485_reset_rx_state();
}

/**
 * @brief   UART接收完成回调函数
 * @param   huart: UART句柄
 * @retval  无
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    /* 必须检查是哪个UART的回调 */
    if (huart->Instance != RS485_UARTX) {
        return;
    }
    
    /* 帧超时检测（100ms） */
    uint32_t current_tick = HAL_GetTick();
    if ((current_tick - s_frame_timeout_tick) > 100) {
        /* 超时，重置状态机 */
        rs485_reset_rx_state();
    }
    s_frame_timeout_tick = current_tick;
    
    /* 根据状态机处理接收到的字节 */
    switch (s_rx_state) {
        case RX_STATE_IDLE:
            /* 等待帧头0x3C */
            if (s_rx_byte == RS485_FRAME_HEAD) {
                s_rx_state = RX_STATE_HEADER;
                s_rx_frame_buffer[s_rx_frame_index++] = s_rx_byte;
            }
            break;
            
        case RX_STATE_HEADER:
            /* 已收到帧头，开始接收其他数据 */
            s_rx_state = RX_STATE_RECEIVING;
            if (s_rx_frame_index < RS485_RX_BUF_LEN) {
                s_rx_frame_buffer[s_rx_frame_index++] = s_rx_byte;
            } else {
                /* 缓冲区溢出，重置状态机 */
                rs485_reset_rx_state();
            }
            break;
            
        case RX_STATE_RECEIVING:
            /* 接收数据，检查帧尾 */
            if (s_rx_frame_index < RS485_RX_BUF_LEN) {
                s_rx_frame_buffer[s_rx_frame_index++] = s_rx_byte;
                
                /* 检查是否收到帧尾0x3E */
                if (s_rx_byte == RS485_FRAME_END) {
                    s_rx_state = RX_STATE_FOOTER;
                    rs485_process_complete_frame();
                }
            } else {
                /* 缓冲区溢出，重置状态机 */
                rs485_reset_rx_state();
            }
            break;
            
        default:
            rs485_reset_rx_state();
            break;
    }
    
    /* 重新启动接收中断 */
    HAL_UART_Receive_IT(&rs485_uartx_handle, &s_rx_byte, 1);
}

/**
 * @brief   RS485发送数据（自动添加帧头帧尾）
 * @param   buf: 数据
 * @param   len: 数据长度
 * @retval  无
 */
void rs485_send_data(uint8_t *buf, uint8_t len)
{
    /* 发送数据 */
    RS485_RE(1);
    HAL_UART_Transmit(&rs485_uartx_handle, buf, len, 1000);
    RS485_RE(0);
}

/**
 * @brief   RS485接收数据 - 非阻塞版本
 * @param   buf: 输出缓冲区
 * @param   len: 实际接收的数据长度（不包括帧头帧尾）
 * @retval  0:成功, -1:无数据
 */
int8_t rs485_recv_data(uint8_t *buf, uint8_t *len)
{
    /* 从帧缓冲区读取数据 */
    return rs485_read_frame_from_buffer(buf, len);
}

/**
 * @brief   获取RS485接收队列中待处理帧的数量
 * @param   无
 * @retval  待处理帧的数量
 */
uint8_t rs485_get_pending_frames(void)
{
    return s_frame_count;
}

/**
 * @brief   清空所有待处理帧
 * @param   无
 * @retval  无
 */
void rs485_clear_pending_frames(void)
{
    rs485_init_frame_buffer();
}