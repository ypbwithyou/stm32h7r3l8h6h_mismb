#include "slidingWindowReceiver_c.h"
#include <string.h>
#include "usbd_cdc_if.h"
/* =============== 工具函数 =============== */

static uint32_t read_u32_be(const uint8_t *p)
{
    return ((uint32_t)p[0] << 24) |
           ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) |
           (uint32_t)p[3];
}

static uint32_t read_u32_le(const uint8_t *p)
{
    return ((uint32_t)p[3] << 24) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[1] << 8) |
           (uint32_t)p[0];
}

/* =============== 初始化 =============== */

void SWR_Init(
    SlidingWindowReceiver_C *r,
    SWR_FrameCallback cb,
    void *ctx)
{
    r->read_pos = 0;
    r->write_pos = 0;
    r->on_frame = cb;
    r->frame_flag = 0;
    r->user_ctx = ctx;
}

/* =============== 核心解析函数（任务上下文） =============== */

void SWR_ProcessBytes(
    SlidingWindowReceiver_C *r,
    const uint8_t *data,
    uint32_t len)
{
    // usb_printf("frame_flag:%d \n", r->frame_flag);
    if (r->frame_flag == 1)
    {
        return;
    }

    if (!data || len == 0)
        return;

    /* ---------- 写入前空间检查 ---------- */
    if (r->write_pos + len > SWR_BUFFER_SIZE)
    {
        if (r->read_pos > 0)
        {
            uint32_t remain = r->write_pos - r->read_pos;
            memmove(r->buffer, r->buffer + r->read_pos, remain);
            r->write_pos = remain;
            r->read_pos = 0;
        }
        else
        {
            /* 极端异常：直接丢弃 */
            r->write_pos = 0;
            r->read_pos = 0;
        }
    }

    memcpy(r->buffer + r->write_pos, data, len);
    r->write_pos += len;

    /* ---------- 解析 ---------- */
    while (r->write_pos - r->read_pos >= SWR_FIXED_SIZE)
    {
        uint8_t *p = r->buffer + r->read_pos;

        /* 1. Header */
        if (read_u32_be(p) != SWR_HEADER)
        {
            r->read_pos += 1;
            continue;
        }

        // usb_printf("Header:%08x \n", read_u32_be(p));

        /* 2. Length（小端） */
        uint32_t payload_len = read_u32_le(p + 20);
        uint32_t frame_len = SWR_FIXED_SIZE + payload_len;

        // usb_printf("payload_len:%d \n", payload_len);

        if (frame_len > SWR_BUFFER_SIZE)
        {
            r->read_pos += 4;
            continue;
        }

        if (r->write_pos - r->read_pos < frame_len)
        {
            break;
        }

        // usb_printf("Tail:%08x \n", read_u32_be(p + frame_len - 4));

        /* 3. Tail */
        if (read_u32_be(p + frame_len - 4) != SWR_TAIL)
        {
            r->read_pos += 4;
            continue;
        }

        /* 4. 回调（任务上下文，安全） */

        r->frame_len_current = frame_len;
        r->frame_flag = 1;
        // usb_printf(" r->frame_flag = 1: %d\n", frame_len);
        r->read_pos += frame_len;
    }

    /* ---------- 空缓冲快速复位 ---------- */
    if (r->read_pos == r->write_pos)
    {
        r->read_pos = 0;
        r->write_pos = 0;
    }
}
