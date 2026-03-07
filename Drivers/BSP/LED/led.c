#include "./BSP/LED/led.h"

/**
 * @brief   初始化LED
 * @param   无
 * @retval  无
 */
void led_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};
    
    /* 使能GPIO端口时钟 */
    LED0_GPIO_CLK_ENABLE();
    
    /* 配置LED0控制引脚 */
    gpio_init_struct.Pin = LED0_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED0_GPIO_PORT, &gpio_init_struct);
    
    /* 关闭LED0 */
    LED0(1);
}
