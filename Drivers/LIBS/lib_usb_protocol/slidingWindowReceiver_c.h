#ifndef SLIDING_WINDOW_RECEIVER_C_H
#define SLIDING_WINDOW_RECEIVER_C_H

#include <stdint.h>

/* ================= 协议参数 ================= */

#define SWR_HEADER 0x3C3C3C3C
#define SWR_TAIL 0x3E3E3E3E

#define SWR_FIXED_SIZE (20 + 4 + 4 + 4)
#define SWR_MAX_PAYLOAD (512 * 1024)
#define SWR_BUFFER_SIZE (SWR_FIXED_SIZE + SWR_MAX_PAYLOAD)

/* ================= 回调 ================= */

typedef void (*SWR_FrameCallback)(
    const uint8_t *frame,
    uint32_t frame_len,
    void *user_ctx);

/* ================= 接收器对象 ================= */

typedef struct
{
    uint8_t *buffer;

    uint32_t read_pos;
    uint32_t write_pos;

    uint32_t frame_len_current;
    uint8_t frame_flag;
    uint32_t frame_start_pos; // ← 新增

    SWR_FrameCallback on_frame;
    void *user_ctx;
} SlidingWindowReceiver_C;

/* ================= 对外接口 ================= */

/* 协议解析（仅在协议任务中调用） */
void SWR_Init(
    SlidingWindowReceiver_C *r,
    SWR_FrameCallback cb,
    void *ctx);

void SWR_ProcessBytes(
    SlidingWindowReceiver_C *r,
    const uint8_t *data,
    uint32_t len);

#endif
