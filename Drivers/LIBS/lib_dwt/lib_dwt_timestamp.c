#include "lib_dwt_timestamp.h"
#include "stm32h7r3xx.h"

/* ------------------------------------------------------------------ */
/*  编译期校验                                                          */
/* ------------------------------------------------------------------ */
#define SYS_CLOCK_HZ   600000000U
#define CYCLES_PER_US  600U
#define CYCLES_PER_NS  (CYCLES_PER_US / 1000U)   /* 仅供注释，实际用比例 */

_Static_assert(SYS_CLOCK_HZ / 1000000U == CYCLES_PER_US,
               "CYCLES_PER_US 与 SYS_CLOCK_HZ 不匹配，请同步修改");

/* ------------------------------------------------------------------ */
/*  内部状态                                                            */
/* ------------------------------------------------------------------ */
/*
 * overflow_count：32-bit，Cortex-M 单指令写，天然原子。
 * 32-bit 溢出周期 = 2^32 / 600e6 ≈ 7.16 s，
 * overflow_count 本身需要 7.16 s × 2^32 ≈ 30730 年才溢出，无需担心。
 */
static struct
{
    volatile uint32_t overflow_count;
    volatile uint32_t last_cyc;       /* SysTick hook 用于边沿检测 */
    volatile bool     initialized;
    volatile bool     running;
} dwt_state = {0};

/* ------------------------------------------------------------------ */
/*  核心读取：double-read 无锁防撕裂                                    */
/* ------------------------------------------------------------------ */
/*
 * 协议：
 *   写方（SysTick ISR）：先更新 overflow_count（单指令，原子）。
 *   读方（本函数）：先读 overflow，再读 CYCCNT，再读 overflow。
 *   若两次 overflow 不同，说明期间发生了溢出，重试。
 *
 * 边界窗口（CYCCNT 已绕回但 ISR 尚未执行）：
 *   overflow1 == overflow2，但 cyc 接近 0 而 overflow 尚未递增。
 *   通过阈值判断：若 cyc < 阈值 且 当前 overflow 已变化，修正为 overflow+1。
 *   阈值取 2^28（约 0.45 s），远大于任何中断延迟，保守安全。
 */
#define OVERFLOW_EDGE_THRESHOLD  0x10000000U   /* 2^28 cycles ≈ 0.45 s */

static uint64_t get_total_cycles(void)
{
    uint32_t ov1, ov2, cyc;

    do {
        ov1 = dwt_state.overflow_count;
        cyc = DWT->CYCCNT;
        ov2 = dwt_state.overflow_count;
    } while (ov1 != ov2);

    /*
     * 极罕见边界：CYCCNT 已绕回（值很小），但 SysTick ISR 还没跑到，
     * overflow_count 还是旧值。此时再读一次 overflow，若已变则修正。
     */
    if (cyc < OVERFLOW_EDGE_THRESHOLD)
    {
        uint32_t ov3 = dwt_state.overflow_count;
        if (ov3 != ov1)
            return ((uint64_t)ov3 << 32) | cyc;
    }

    return ((uint64_t)ov1 << 32) | cyc;
}

/* ------------------------------------------------------------------ */
/*  SysTick hook（在 SysTick_Handler 里调用）                          */
/* ------------------------------------------------------------------ */
/*
 * SysTick 默认 1 ms 一次，远小于 7.16 s 的溢出周期，
 * 因此每次溢出至多触发一次递增，不会漏计。
 *
 * 使用边沿检测（cur < last）而非固定计数，
 * 即使 SysTick 周期被临时拉长也不会漏溢出。
 */
void dwt_systick_hook(void)
{
    if (!dwt_state.initialized || !dwt_state.running)
        return;

    uint32_t cur = DWT->CYCCNT;

    if (cur < dwt_state.last_cyc)
        dwt_state.overflow_count++;   /* 32-bit 写，单指令原子 */

    dwt_state.last_cyc = cur;
}

/* ------------------------------------------------------------------ */
/*  初始化 / 控制                                                       */
/* ------------------------------------------------------------------ */
bool dwt_init(void)
{
    /* 使能 DWT */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    __DSB();

    /* 检查硬件是否支持 CYCCNT */
    if (DWT->CTRL & DWT_CTRL_NOCYCCNT_Msk)
        return false;

    /* 清零并使能 */
    DWT->CYCCNT = 0;
    __DSB();
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    dwt_state.overflow_count = 0;
    dwt_state.last_cyc       = 0;
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

/*
 * reset 需要关中断：同时清零 CYCCNT、overflow_count、last_cyc，
 * 三者必须原子更新，否则 hook 或 get_total_cycles 会读到不一致状态。
 * reset 是低频操作，关中断代价可接受。
 */
void dwt_reset(void)
{
    if (!dwt_state.initialized) return;

    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    DWT->CYCCNT = 0;
    __DSB();
    dwt_state.overflow_count = 0;
    dwt_state.last_cyc       = 0;

    if (!primask) __enable_irq();
}

/* ------------------------------------------------------------------ */
/*  时间读取                                                            */
/* ------------------------------------------------------------------ */
timestamp_t dwt_get_timestamp(void)
{
    timestamp_t ts = {0};
    if (!dwt_state.initialized || !dwt_state.running)
        return ts;

    uint64_t c   = get_total_cycles();
    ts.tv_sec    = (uint32_t)(c / SYS_CLOCK_HZ);
    ts.tv_usec   = (uint32_t)((c % SYS_CLOCK_HZ) / CYCLES_PER_US);
    return ts;
}

uint64_t dwt_get_timestamp_raw(void)
{
    if (!dwt_state.initialized || !dwt_state.running)
        return 0;
    return get_total_cycles();
}

uint64_t dwt_get_us(void)
{
    if (!dwt_state.initialized || !dwt_state.running)
        return 0;
    return get_total_cycles() / CYCLES_PER_US;
}

uint64_t dwt_get_ns(void)
{
    if (!dwt_state.initialized || !dwt_state.running)
        return 0;
    /*
     * cycles * 1000 / 600 = ns
     * uint64_t 最大值 / 1000 ≈ 1.8×10^16，
     * 600 MHz 下需运行约 975 年才可能溢出，安全。
     */
    return get_total_cycles() * 1000ULL / CYCLES_PER_US;
}
