#ifndef __LIB_DWT_TIMESTAMP_H
#define __LIB_DWT_TIMESTAMP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define GET_TIME_MS(n_t) ((long long)(n_t).tv_sec * 1000 + (n_t).tv_usec / 1000)
#define TIME_DIFF_MS(n_t, l_t) (((n_t).tv_sec - (l_t).tv_sec) * 1000 + \
                                ((n_t).tv_usec - (l_t).tv_usec) / 1000)
#define GET_TIME_NANOS(n_t) ((long long)(n_t).tv_sec * 1000000000LL + \
                             (long long)(n_t).tv_usec * 1000)

    typedef struct
    {
        uint32_t tv_sec;
        uint32_t tv_usec;
    } timestamp_t;

    bool dwt_init(void);
    void dwt_start(void);
    void dwt_stop(void);
    void dwt_reset(void);

    timestamp_t dwt_get_timestamp(void);
    uint64_t dwt_get_timestamp_raw(void);
    uint64_t dwt_get_us(void);
    uint64_t dwt_get_ns(void);

    /* 在用户 SysTick_Handler 里调用此函数 */
    void dwt_systick_hook(void);

#ifdef __cplusplus
}
#endif

#endif
