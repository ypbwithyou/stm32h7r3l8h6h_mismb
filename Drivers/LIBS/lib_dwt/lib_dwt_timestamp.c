#include "lib_dwt_timestamp.h"
#include "stm32h7r3xx.h"

#define SYS_CLOCK_HZ  600000000U
#define CYCLES_PER_US 600U

static struct
{
    volatile uint64_t overflow_count;
    volatile bool     initialized;
    volatile bool     running;
} dwt_state = {0};

/* 无锁读取，自动检测并更新溢出，无需关中断，无需外部定时器 */
static uint64_t get_total_cycles(void)
{
    uint32_t before, after;
    uint64_t overflow;

    do {
        before   = DWT->CYCCNT;
        overflow = dwt_state.overflow_count;
        after    = DWT->CYCCNT;

        if (after < before) {
            // 读取期间发生溢出，就地更新后重试
            dwt_state.overflow_count = overflow + 1;
        }

    } while (after < before);

    return ((uint64_t)overflow << 32) | after;
}

bool dwt_init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    if (DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk)
        return false;

    DWT->CYCCNT = 0;
    (void)DWT->CYCCNT;  // 等待写入生效
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    dwt_state.overflow_count = 0;
    dwt_state.initialized    = true;
    dwt_state.running        = true;

    return true;
}

void dwt_start(void)
{
    if (!dwt_state.initialized) return;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    dwt_state.running = true;
}

void dwt_stop(void)
{
    if (!dwt_state.initialized) return;
    DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
    dwt_state.running = false;
}

void dwt_reset(void)
{
    if (!dwt_state.initialized) return;

    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    DWT->CYCCNT = 0;
    (void)DWT->CYCCNT;  // 等待写入生效
    dwt_state.overflow_count = 0;

    if (!primask) __enable_irq();
}

timestamp_t dwt_get_timestamp(void)
{
    timestamp_t ts = {0};
    if (!dwt_state.initialized || !dwt_state.running)
        return ts;

    uint64_t c = get_total_cycles();
    ts.tv_sec  = (uint32_t)(c / SYS_CLOCK_HZ);
    ts.tv_usec = (uint32_t)((c % SYS_CLOCK_HZ) / CYCLES_PER_US);
    return ts;
}

uint64_t dwt_get_timestamp_raw(void)
{
    return get_total_cycles();
}

uint64_t dwt_get_us(void)
{
    return get_total_cycles() / CYCLES_PER_US;
}

uint64_t dwt_get_ns(void)
{
    uint64_t c = get_total_cycles();
    return (c / CYCLES_PER_US) * 1000
         + (c % CYCLES_PER_US) * 1000 / CYCLES_PER_US;
}
