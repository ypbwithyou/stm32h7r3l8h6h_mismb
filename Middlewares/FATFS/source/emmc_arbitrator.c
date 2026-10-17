/**
  ****************************************************************************************************
  * @file        emmc_arbitrator.c
  * @brief       eMMC 访问仲裁实现（详见 emmc_arbitrator.h）
  ****************************************************************************************************
  */

#include "./FATFS/source/emmc_arbitrator.h"
#include "./SYSTEM/sys/sys.h"

/* usbd_conf.c 中的 USB 连接状态：1 = 已连接，0 = 拔出/挂起 */
extern uint8_t g_usb_conn_state;

/* -------------------------------------------------------------------------- */
/*  本机侧状态                                                                  */
/* -------------------------------------------------------------------------- */
static volatile uint8_t s_local_busy = 0U;   /* 本机磁盘操作进行中 (中断可见)   */

/* -------------------------------------------------------------------------- */
/*  主机侧状态                                                                  */
/* -------------------------------------------------------------------------- */
static volatile uint8_t  s_media_req   = 0U; /* 主机近期有真实数据访问          */
static volatile uint32_t s_media_tick  = 0U; /* 最近一次数据访问的时间戳        */
static uint8_t           s_media_taken = 0U; /* 已接管(主循环上下文独占修改)    */
static volatile uint8_t  s_defer       = 0U; /* 本机长写会话，推迟接管          */

/* -------------------------------------------------------------------------- */
/*  weak 钩子（默认空实现，可被上层模块覆盖）                                     */
/* -------------------------------------------------------------------------- */
__weak void emmc_fs_suspend_hook(void)
{
}

__weak void emmc_fs_resume_hook(void)
{
}

/* -------------------------------------------------------------------------- */
/*  本机侧                                                                      */
/* -------------------------------------------------------------------------- */
void emmc_local_lock(void)
{
    s_local_busy = 1U;
}

void emmc_local_unlock(void)
{
    s_local_busy = 0U;
}

uint8_t emmc_local_busy(void)
{
    return s_local_busy;
}

/* -------------------------------------------------------------------------- */
/*  主机侧 (USB 中断上下文调用：只写标记，绝不能做阻塞/耗时操作)                  */
/* -------------------------------------------------------------------------- */
void emmc_msc_touch(void)
{
    s_media_tick = HAL_GetTick();
    s_media_req = 1U;
}

void emmc_set_defer(uint8_t defer)
{
    s_defer = (defer != 0U) ? 1U : 0U;
}

uint8_t emmc_media_taken(void)
{
    return s_media_taken;
}

/* -------------------------------------------------------------------------- */
/*  接管/释放状态机（主循环上下文）                                              */
/* -------------------------------------------------------------------------- */
void emmc_media_poll(void)
{
    uint32_t now = HAL_GetTick();

    /* USB 拔出或挂起：立即释放，本机恢复访问 */
    if (g_usb_conn_state == 0U)
    {
        s_media_req = 0U;
        if (s_media_taken != 0U)
        {
            s_media_taken = 0U;
            emmc_fs_resume_hook();
        }
        return;
    }

    if (s_media_req == 0U)
    {
        /* 主机没有真实数据访问（只用 CDC 串口时就是这种状态）：本机独占 eMMC */
        if (s_media_taken != 0U)
        {
            s_media_taken = 0U;
            emmc_fs_resume_hook();
        }
        return;
    }

    /* 主机有数据访问活动 */
    if ((uint32_t)(now - s_media_tick) > EMMC_MEDIA_HOLD_MS)
    {
        /* 超过保持时间没有新读操作（主机已弹出/关闭文件/仅剩周期性 TUR）→ 释放 */
        s_media_req = 0U;
        if (s_media_taken != 0U)
        {
            s_media_taken = 0U;
            emmc_fs_resume_hook();
        }
        return;
    }

    /* 活动期内：申请接管（本机长写会话期间推迟），钩子先于接管标志执行，
       以便钩子内仍可访问磁盘（刷缓存等） */
    if ((s_media_taken == 0U) && (s_defer == 0U))
    {
        emmc_fs_suspend_hook();
        s_media_taken = 1U;
    }
}
