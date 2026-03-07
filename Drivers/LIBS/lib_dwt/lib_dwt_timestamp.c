#include "lib_dwt_timestamp.h"
#include "stm32h7r3xx.h"

// 系统参数（STM32H7R3 600MHz）
#define SYS_CLOCK_HZ     600000000U
#define CYCLES_PER_US    600U          // 600MHz / 1MHz

// DWT状态变量
static struct {
    volatile uint64_t overflow_count;  // 溢出次数（每2^32周期）
    volatile uint32_t last_cycle;      // 上次读取的周期值
    bool initialized;                  // 初始化标志
    bool running;                      // 运行状态
} dwt_state = {0};

// 初始化DWT
bool dwt_init(void)
{
    // 启用DWT跟踪
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    
    // 检查DWT是否可用
    if (DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk) {
        return false;  // 不支持DWT
    }
    
    // 启动周期计数器
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    
    // 初始化状态
    dwt_state.overflow_count = 0;
    dwt_state.last_cycle = 0;
    dwt_state.initialized = true;
    dwt_state.running = true;
    
    return true;
}

// 检查并更新溢出（必须在禁用中断或原子操作中调用）
static void update_overflow(void)
{
    uint32_t current = DWT->CYCCNT;
    
    // 检测溢出：当前值 < 上次值
    if (current < dwt_state.last_cycle) {
        dwt_state.overflow_count++;
    }
    
    dwt_state.last_cycle = current;
}

// 获取64位周期计数
static uint64_t get_total_cycles(void)
{
    uint32_t primask;
    uint32_t cycles_low, cycles_high1, cycles_high2;
    uint64_t total_cycles;
    
    // 禁用中断确保原子性
    primask = __get_PRIMASK();
    __disable_irq();
    
    // 更新溢出计数
    update_overflow();
    
    // 读取当前值（防止读取过程中被中断）
    do {
        cycles_high1 = (uint32_t)dwt_state.overflow_count;
        cycles_low = DWT->CYCCNT;
        cycles_high2 = (uint32_t)dwt_state.overflow_count;
    } while (cycles_high1 != cycles_high2);
    
    // 恢复中断状态
    if (!primask) {
        __enable_irq();
    }
    
    // 组合64位值
    total_cycles = ((uint64_t)cycles_high1 << 32) | cycles_low;
    
    return total_cycles;
}

// 控制函数
void dwt_start(void)
{
    if (!dwt_state.initialized) return;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    dwt_state.running = true;
}
// 停止DWT
void dwt_stop(void)
{
    if (!dwt_state.initialized) return;
    DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
    dwt_state.running = false;
}
// 重置DWT
void dwt_reset(void)
{
    if (!dwt_state.initialized) return;
    
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    
    DWT->CYCCNT = 0;
    dwt_state.overflow_count = 0;
    dwt_state.last_cycle = 0;
    
    if (!primask) __enable_irq();
}

// 获取时间戳
timestamp_t dwt_get_timestamp(void)
{
    timestamp_t ts = {0};
    
    if (!dwt_state.initialized || !dwt_state.running) {
        return ts;
    }
    
    uint64_t total_cycles = get_total_cycles();
    
    // 转换为秒和微秒
    ts.tv_sec = total_cycles / SYS_CLOCK_HZ;
    ts.tv_usec = (total_cycles % SYS_CLOCK_HZ) / CYCLES_PER_US;
    
    return ts;
}

// 获取64位原始时间戳
uint64_t dwt_get_timestamp_raw(void)
{
    timestamp_t ts = dwt_get_timestamp();
    return ((uint64_t)ts.tv_sec << 32) | ts.tv_usec;
}

// 获取微秒数
uint64_t dwt_get_us(void)
{
    uint64_t total_cycles = get_total_cycles();
    return total_cycles / CYCLES_PER_US;
}

// 获取纳秒数
uint64_t dwt_get_ns(void)
{
    uint64_t total_cycles = get_total_cycles();
    return total_cycles * 1000 / CYCLES_PER_US;
}
