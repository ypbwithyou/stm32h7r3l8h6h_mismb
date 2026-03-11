#include "./BSP/IIC/myiic.h"
#include "./BSP/PCA9554A/pca9554a.h"
#include "ida_config.h"
#include "usbd_cdc_if.h"

void PCA9554A_Out_Init(void);

/**
 * @brief   系统电源供电控制
 * @param   无
 * @retval  无
 */
static void sys_pwr_ctl_io_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    /* ʹ��GPIO�˿�ʱ�� */
    MCU_PWR_GPIO_CLK_ENABLE();

    /* ����LED0�������� */
    gpio_init_struct.Pin = MCU_PWR_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(MCU_PWR_GPIO_PORT, &gpio_init_struct);

    /* �ر�LED0��LED1 */
    MCU_PWR_CTL(1);
}

/**
 * @brief   系统电源供电状态监测端口初始化
 * @param   无
 * @retval  无
 */
static void sys_pwr_io_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    /* IO时钟初始化 */
    EXT_PWR_GPIO_CLK_ENABLE();
    USB_PWR_GPIO_CLK_ENABLE();

    /* 配置EXT_PWR控制引脚 */
    gpio_init_struct.Pin = EXT_PWR_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_INPUT;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(EXT_PWR_GPIO_PORT, &gpio_init_struct);

    /* 配置USB_PWR控制引脚 */
    gpio_init_struct.Pin = USB_PWR_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_INPUT;
    gpio_init_struct.Pull = GPIO_PULLDOWN;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(USB_PWR_GPIO_PORT, &gpio_init_struct);
}

/**
 * @brief   事件及触发引脚初始化
 * @param   无
 * @retval  无
 */
void external_io_init(void)
{
    GPIO_InitTypeDef gpio_init_struct = {0};

    /* IO时钟初始化 */
    START_GPIO_CLK_ENABLE();
    EVENT_GPIO_CLK_ENABLE();

    /* 配置START控制引脚 */
    gpio_init_struct.Pin = START_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_IT_FALLING;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(START_GPIO_PORT, &gpio_init_struct);

    /* 配置EVENT控制引脚 */
    gpio_init_struct.Pin = EVENT_GPIO_PIN;
    gpio_init_struct.Mode = GPIO_MODE_IT_FALLING;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(EVENT_GPIO_PORT, &gpio_init_struct);

    /* 配置中断优先级并使能中断 */
    HAL_NVIC_SetPriority(START_INT_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(START_INT_IRQn);
    HAL_NVIC_SetPriority(EVENT_INT_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(EVENT_INT_IRQn);
}

/**
 * @brief   START按键外部中断服务函数
 * @param   无
 * @retval  无
 */
void START_INT_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(START_GPIO_PIN);
}

/**
 * @brief   EVENT按键外部中断服务函数
 * @param   无
 * @retval  无
 */
void EVENT_INT_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(EVENT_GPIO_PIN);
}

/**
 * @brief       中断服务程序中需要做的事情
                在HAL库中所有的外部中断服务函数都会调用此函数
 * @param       GPIO_Pin:中断引脚号
 * @retval      无
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    switch (GPIO_Pin)
    {
    case START_GPIO_PIN: /* WKUP按键对应引脚发生中断 */
    {
        g_IdaSystemStatus.st_dev_offline.offline_mode = 1;

        usb_printf("---------------------------> HAL_GPIO_EXTI_Callback START_GPIO_PIN \r\n");
    }
    break;
    case EVENT_GPIO_PIN: /* KEY0按键对应引脚发生中断 */
    {
        // g_IdaSystemStatus.st_dev_offline.event_flag = 1;

        usb_printf("---------------------------> HAL_GPIO_EXTI_Callback EVENT_GPIO_PIN \r\n");
    }
    break;
    }
}

/**
 * @brief       PCA9554A初始化
 * @param       无
 * @retval      无
 */
void PCA9554A_init(void)
{
    sys_pwr_ctl_io_init();
    sys_pwr_io_init();
    iic_init();
    PCA9554A_Out_Init();
}

// 写PCA9554A寄存器
// reg: 寄存器地址（0x00~0x03）
// data: 要写入的数据
// 返回0成功，1失败
uint8_t PCA9554A_WriteReg(uint8_t dev_addr, uint8_t reg, uint8_t data)
{
    iic_start();

    // 发送设备地址+写位(0)
    iic_send_byte((dev_addr << 1) | 0);
    if (iic_wait_ack())
    {
        iic_stop();
        return PCA9554A_ERROR;
    }

    // 发送寄存器地址
    iic_send_byte(reg);
    if (iic_wait_ack())
    {
        iic_stop();
        return PCA9554A_ERROR;
    }

    // 发送数据
    iic_send_byte(data);
    if (iic_wait_ack())
    {
        iic_stop();
        return PCA9554A_ERROR;
    }

    iic_stop();
    return PCA9554A_OK;
}
// 读PCA9554A寄存器
// reg: 寄存器地址
// data: 读取数据指针
// 返回0成功，1失败
uint8_t PCA9554A_ReadReg(uint8_t dev_addr, uint8_t reg, uint8_t *data)
{
    iic_start();

    // 发送设备地址+写位(0)，先写寄存器地址
    iic_send_byte((dev_addr << 1) | PCA9554A_WRITE);
    if (iic_wait_ack())
    {
        iic_stop();
        return PCA9554A_ERROR;
    }

    // 发送寄存器地址
    iic_send_byte(reg);
    if (iic_wait_ack())
    {
        iic_stop();
        return PCA9554A_ERROR;
    }

    // 重复起始信号
    iic_start();

    // 发送设备地址+读位(1)
    iic_send_byte((dev_addr << 1) | PCA9554A_READ);
    if (iic_wait_ack())
    {
        iic_stop();
        return PCA9554A_ERROR;
    }

    // 读取数据，发送NACK
    *data = iic_read_byte(ACK);
    iic_wait_ack(); // 0表示NACK，结束读取

    iic_stop();
    return PCA9554A_OK;
}

// 初始化PCA9554A，配置所有IO为输出，关闭所有LED（假设低电平点亮）
void PCA9554A_Out_Init(void)
{
    PCA9554A_WriteReg(PCA9554A_DEVICE_ADDR, PCA9554A_REG_CONFIG, 0x00); // 全部输出
    PCA9554A_WriteReg(PCA9554A_DEVICE_ADDR, PCA9554A_REG_OUTPUT, 0xFF); // 全部LED关闭（高电平）
}

// 设置PCA9554A输出值
uint8_t PCA9554_Set(uint8_t led_mask)
{
    return PCA9554A_WriteReg(PCA9554A_DEVICE_ADDR, PCA9554A_REG_OUTPUT, led_mask);
}

// 获取PCA9554A当前输出值
uint8_t PCA9554_Get(uint8_t *led_mask)
{
    return PCA9554A_ReadReg(PCA9554A_DEVICE_ADDR, PCA9554A_REG_OUTPUT, led_mask);
}

// 电源电量指示灯输出、监测与控制
void SysPwrLED_Output(void)
{
    uint8_t ret = PCA9554A_OK;
    uint8_t read_val = 0;

    ret = PCA9554_Get(&read_val);
    if (ret != PCA9554A_OK)
    {
        // printf("PCA9554A_ReadReg err.\n");
    }

    if ((EXT_PWR_OK == 0) && (USB_PWR_OK == 0)) // 系统电量弱，仅红灯亮
    {
        ret = PCA9554_Set(PCA9544A_PWR_R(read_val));
        if (ret != PCA9554A_OK)
        {
            // printf("PCA9554_SetLED err. system power low.\n");
        }
    }
    else if ((EXT_PWR_OK == 0) && (USB_PWR_OK == 1)) // USB供电，仅蓝灯亮
    {
        ret = PCA9554_Set(PCA9544A_PWR_B(read_val));
        if (ret != PCA9554A_OK)
        {
            // printf("PCA9554_SetLED err. usb power on.\n");
        }
    }
    else // 外部供电，仅绿灯亮
    {
        ret = PCA9554_Set(PCA9544A_PWR_G(read_val));
        if (ret != PCA9554A_OK)
        {
            // printf("PCA9554_SetLED err. extern power on\n");
        }
    }
}

// 系统状态指示灯输出、监测与控制
void SysStsLED_Output(void)
{
    static uint16_t i = 0;
    uint8_t ret = PCA9554A_OK;
    uint8_t read_val = 0;

    ret = PCA9554_Get(&read_val);
    if (ret != PCA9554A_OK)
    {
        // printf("PCA9554A_ReadReg err.\n");
    }

    i++;

    if (i < 20)
    {
        PCA9554_Set(PCA9544A_STS_R(read_val));
    }
    else
    {
        PCA9554_Set(PCA9544A_STS_G(read_val));
        i = 20;
    }
}
