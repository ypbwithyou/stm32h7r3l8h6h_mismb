#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "ida_config.h"
 
#include "usb_processor.h"
#include "usbd_cdc_if.h"
#include "stm32h7rsxx_it.h"

int8_t IdaSystemInit()
{
    sys_mpu_config();                /* 配置MPU */
    sys_cache_enable();              /* 使能Cache */
    HAL_Init();                      /* 初始化HAL库 */
    sys_stm32_clock_init(300, 6, 2); /* 配置时钟，600MHz */
    sys_tick_priority_init();        /* 提高SysTick中断优先级（在时钟配置后） */

    return IdaDeviceInit();
}

int main(void)
{
    int8_t ret = RET_OK;

#if UPGRADE_ON
    /*设置NVIc的向量表偏移寄存器，VTOR低9位保留，即[8:0]保留*/
    SCB->VTOR = FLASH_BASE | (0x5000 & (uint32_t)0xFFFFFE00);
#endif

    ret = IdaSystemInit();

    app_processor();
 
    return -1;
}