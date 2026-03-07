#ifndef __LIB_DWT_TIMESTAMP_H
#define __LIB_DWT_TIMESTAMP_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GET_TIME_MS(n_t)    ((long long)n_t.tv_sec * 1000 + n_t.tv_usec / 1000) 
#define TIME_DIFF_MS(n_t, l_t)  ((n_t.tv_sec - l_t.tv_sec) * 1000 + (n_t.tv_usec - l_t.tv_usec) / 1000)
#define GET_TIME_NANOS(n_t)    ((long long)n_t.tv_sec * 1000000000 + n_t.tv_nsec)

// 时间戳结构体
typedef struct {
    uint32_t tv_sec;      // 秒部分
    uint32_t tv_usec; // 微秒部分（0-999999）
} timestamp_t;

// 初始化DWT计时器
bool dwt_init(void);

// 控制函数
void dwt_start(void);
void dwt_stop(void);
void dwt_reset(void);

// 获取时间戳
timestamp_t dwt_get_timestamp(void);
uint64_t dwt_get_timestamp_raw(void); // 返回64位值

// 获取当前微秒数（自启动）
uint64_t dwt_get_us(void);
// 获取纳秒数
uint64_t dwt_get_ns(void);

#ifdef __cplusplus
}
#endif

#endif // __LIB_DWT_TIMESTAMP_H