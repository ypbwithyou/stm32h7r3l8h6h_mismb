/**
  ****************************************************************************************************
  * @file        emmc_arbitrator.h
  * @brief       eMMC 访问仲裁：本机 FatFS 与 USB MSC(只读U盘) 共用同一块 eMMC 时的互斥与接管策略
  *
  * 策略（主机独占 + 本机暂停）：
  *   1) 扇区级互斥(硬保护)：
  *      - 本机 disk_read/disk_write/disk_ioctl/disk_initialize 进出时 emmc_local_lock()/unlock()
  *      - MSC 侧 IsReady() 检测到 emmc_local_busy() 时返回"未就绪"，
  *        主机收到 NOT_READY sense 自动重试（SCSI_Read10 在 usbd_msc_scsi.c:729 先查 IsReady）
  *      - MSC 回调运行在 USB 中断上下文，绝不允许在此等待本机操作完成
  *        （本机传输依赖 SDMMC 中断完成，中断内忙等会死锁）
  *   2) 文件系统级接管(策略)：
  *      - 仅当主机发起真实数据读(SCSI Read -> USBD_MSC_fops.Read -> emmc_msc_touch)
  *        才视为占用；INQUIRY/READ CAPACITY/TEST UNIT READY 等探测不触发
  *      - 主循环轮询 emmc_media_poll()：满足条件后置 emmc_media_taken()==1，
  *        本机所有 FatFS 访问快速失败(RES_NOTRDY)；主机停止读 10s 或 USB 拔出/挂起后自动恢复
  ****************************************************************************************************
  */

#ifndef __EMMC_ARBITRATOR_H
#define __EMMC_ARBITRATOR_H

#include <stdint.h>

/* 主机停止数据访问超过该时长后，本机恢复对 eMMC 的访问 */
#define EMMC_MEDIA_HOLD_MS      10000U

/* ---------- 本机侧（diskio.c 调用） ---------- */
void    emmc_local_lock(void);         /* 本机开始一次磁盘操作 */
void    emmc_local_unlock(void);       /* 本机结束一次磁盘操作 */
uint8_t emmc_local_busy(void);         /* 本机是否正在操作磁盘 (MSC IsReady 用) */

/* ---------- 主机侧（usbd_msc_storage_if.c 调用） ---------- */
void    emmc_msc_touch(void);          /* 主机发起真实数据读：刷新活动时间戳并请求接管 */
uint8_t emmc_media_taken(void);        /* 1 = 主机已接管，本机应暂停 FatFS 访问 */
void    emmc_media_poll(void);         /* 主循环轮询：执行接管/释放 (可放在 while(1) 每圈) */

/* ---------- 上层会话控制 ---------- */
void    emmc_set_defer(uint8_t defer); /* 1 = 暂不接管 (CDC 文件下载等本机长写会话期间) */

/* ---------- 可选钩子（weak 空实现，模块可覆盖） ---------- */
/* 注意：钩子在 emmc_media_taken 置位/清零之前调用，此时本机磁盘访问仍被允许 */
void    emmc_fs_suspend_hook(void);    /* 主机接管前：可在此刷缓存/收尾 */
void    emmc_fs_resume_hook(void);     /* 主机释放后：可在此恢复状态 */

#endif
