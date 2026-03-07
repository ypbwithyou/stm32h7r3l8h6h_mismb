#ifndef __PCA9554A_H
#define __PCA9554A_H

#include <stdint.h>

// PCA9554A设备地址
#define PCA9554A_DEVICE_ADDR    0x38    /*A0=0,A1=0,A2=0*/

// PCA9554A读写定义
#define PCA9554A_WRITE  0
#define PCA9554A_READ   1

// PCA9554寄存器地址定义
#define PCA9554A_REG_INPUT       0x00
#define PCA9554A_REG_OUTPUT      0x01
#define PCA9554A_REG_POL_INV     0x02
#define PCA9554A_REG_CONFIG      0x03

typedef enum {
    PCA9554A_OK = 0,
    PCA9554A_ERROR = 1,
} PCA9554A_StatusTypeDef;

/**PCA9554A输出引脚定义
 * P0       PWR_R
 * P1       PWR_G
 * P2       PWR_B
 * P3       STS_R
 * P4       STS_G
 * P5       STS_B
 * P6       RESET_HUB
 * P7       STUS_OUT
 */
#define PCA9544A_PWR_R(data)  (((data | 0x07) & 0xFE))  // 仅输出红灯
#define PCA9544A_PWR_G(data)  (((data | 0x07) & 0xFD))  // 仅输出绿灯
#define PCA9544A_PWR_B(data)  (((data | 0x07) & 0xFB))  // 仅输出蓝灯
#define PCA9544A_PWR_I(data)  (((data | 0x07) & 0xFF))  // 电源电量指示灯灭

#define PCA9544A_STS_R(data)  (((data | 0x38) & 0xF7))  // 仅输出红灯
#define PCA9544A_STS_G(data)  (((data | 0x38) & 0xEF))  // 仅输出绿灯
#define PCA9544A_STS_B(data)  (((data | 0x38) & 0xDF))  // 仅输出蓝灯
#define PCA9544A_STS_I(data)  (((data | 0x38) & 0xFF))  // 系统状态指示灯灭
/**系统供电控制引脚定义
 * MCU_PWR_ON	PG0	输出	高
 */
#define MCU_PWR_GPIO_PORT          GPIOG
#define MCU_PWR_GPIO_PIN           GPIO_PIN_0
#define MCU_PWR_GPIO_CLK_ENABLE()  do { __HAL_RCC_GPIOG_CLK_ENABLE(); } while (0)
/* 系统供电控制引脚IO操作*/
#define MCU_PWR_CTL(x)              do { (x) ?                                                              \
                                        HAL_GPIO_WritePin(MCU_PWR_GPIO_PORT, MCU_PWR_GPIO_PIN, GPIO_PIN_SET):     \
                                        HAL_GPIO_WritePin(MCU_PWR_GPIO_PORT, MCU_PWR_GPIO_PIN, GPIO_PIN_RESET);   \
                                    } while (0)

/**系统供电状态监测引脚定义
 * EXT_PWR_OK	PM0     输入
 * USB_PWR_OK	PM1     输入
 * 
 * 电源指示灯定义
 * 红色     R       EXT_PWR_OK=0        电量弱
 *                  USB_PWR_OK=0
 * 蓝色     B       EXT_PWR_OK=0        USB供电
 *                  USB_PWR_OK=1
 * 绿色     G       EXT_PWR_OK=1        外部电源供电
 */
#define EXT_PWR_GPIO_PORT          GPIOM
#define EXT_PWR_GPIO_PIN           GPIO_PIN_0
#define EXT_PWR_GPIO_CLK_ENABLE()  do { __HAL_RCC_GPIOM_CLK_ENABLE(); } while (0)
#define USB_PWR_GPIO_PORT          GPIOM
#define USB_PWR_GPIO_PIN           GPIO_PIN_1
#define USB_PWR_GPIO_CLK_ENABLE()  do { __HAL_RCC_GPIOM_CLK_ENABLE(); } while (0)

/* 系统供电状态监测引脚IO操作 */ 
#define EXT_PWR_OK                    ((HAL_GPIO_ReadPin(EXT_PWR_GPIO_PORT, EXT_PWR_GPIO_PIN) == GPIO_PIN_RESET) ? 0 : 1)
#define USB_PWR_OK                    ((HAL_GPIO_ReadPin(USB_PWR_GPIO_PORT, USB_PWR_GPIO_PIN) == GPIO_PIN_RESET) ? 0 : 1)

/* 事件及触发引脚定义*/
#define START_GPIO_PORT             GPIOA
#define START_GPIO_PIN              GPIO_PIN_1
#define START_GPIO_CLK_ENABLE()     do { __HAL_RCC_GPIOA_CLK_ENABLE(); } while (0)
#define START_INT_IRQn              EXTI1_IRQn 
#define START_INT_IRQHandler        EXTI1_IRQHandler 

#define EVENT_GPIO_PORT             GPIOA
#define EVENT_GPIO_PIN              GPIO_PIN_2
#define EVENT_GPIO_CLK_ENABLE()     do { __HAL_RCC_GPIOA_CLK_ENABLE(); } while (0)
#define EVENT_INT_IRQn              EXTI2_IRQn 
#define EVENT_INT_IRQHandler        EXTI2_IRQHandler 

//#define START_STATUS                ((HAL_GPIO_ReadPin(START_GPIO_PORT, START_GPIO_PIN) == GPIO_PIN_RESET) ? 0 : 1)
//#define EVENT_STATUS                ((HAL_GPIO_ReadPin(EVENT_GPIO_PORT, EVENT_GPIO_PIN) == GPIO_PIN_RESET) ? 0 : 1)

void PCA9554A_init(void);
uint8_t PCA9554A_WriteReg(uint8_t dev_addr, uint8_t reg, uint8_t data);
uint8_t PCA9554A_ReadReg(uint8_t dev_addr, uint8_t reg, uint8_t *data);
uint8_t PCA9554_Set(uint8_t led_mask);
uint8_t PCA9554_Get(uint8_t *led_mask);
void SysPwrLED_Output(void);
void SysStsLED_Output(void);

void external_io_init(void);

#endif /* __PCA9554A_H */

