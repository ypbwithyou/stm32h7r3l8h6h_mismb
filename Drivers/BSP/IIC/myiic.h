#ifndef __MYIIC_H
#define __MYIIC_H

#include "./SYSTEM/sys/sys.h"


/****************************************************************************************************/
/* ���� ���� */

#define IIC_SCL_GPIO_PORT               GPIOF
#define IIC_SCL_GPIO_PIN                GPIO_PIN_1
#define IIC_SCL_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOF_CLK_ENABLE(); }while(0)   /* PF��ʱ��ʹ�� */

#define IIC_SDA_GPIO_PORT               GPIOF
#define IIC_SDA_GPIO_PIN                GPIO_PIN_0
#define IIC_SDA_GPIO_CLK_ENABLE()       do{ __HAL_RCC_GPIOF_CLK_ENABLE(); }while(0)   /* PF��ʱ��ʹ�� */

/****************************************************************************************************/

/* IO输出 */
#define IIC_SCL(x)        do{ x ? \
                              HAL_GPIO_WritePin(IIC_SCL_GPIO_PORT, IIC_SCL_GPIO_PIN, GPIO_PIN_SET) : \
                              HAL_GPIO_WritePin(IIC_SCL_GPIO_PORT, IIC_SCL_GPIO_PIN, GPIO_PIN_RESET); \
                          }while(0)       /* SCL */

#define IIC_SDA(x)        do{ x ? \
                              HAL_GPIO_WritePin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN, GPIO_PIN_SET) : \
                              HAL_GPIO_WritePin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN, GPIO_PIN_RESET); \
                          }while(0)       /* SDA */

#define IIC_READ_SDA     HAL_GPIO_ReadPin(IIC_SDA_GPIO_PORT, IIC_SDA_GPIO_PIN)        /* ��ȡSDA */

#define ACK     (0)
#define NACK    (1)

/* IIC函数声明 */
void iic_init(void);                        /* 初始化IIC */
void iic_start(void);                       /* 产生IIC起始信号 */
void iic_stop(void);                        /* 产生IIC停止信号 */
void iic_ack(void);                         /* 产生ACK应答 */
void iic_nack(void);                        /* 不产生ACK应答 */
uint8_t iic_wait_ack(void);                 /* 等待应答信号到来 */
void iic_send_byte(uint8_t txd);            /* IIC发送一个字节 */
uint8_t iic_read_byte(unsigned char ack);   /* IIC读取一个字节 */

#endif

