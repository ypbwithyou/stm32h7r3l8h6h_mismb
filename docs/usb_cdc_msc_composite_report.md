# USB CDC + MSC 复合设备（只读 U 盘）修改报告

日期：2026-10-17
目标平台：STM32H7R3L8HxH（USB_OTG_HS + 嵌入式 PHY，USE_USB_HS）
USB 栈：ST STM32_USB_Device_Library v2.11+（CompositeBuilder，USE_USBD_COMPOSITE）
文件系统：FatFS（无 RTOS，SYS_SUPPORT_OS = 0）

---

## 1. 目标与用户决策

把原有 CDC 虚拟串口扩展为 CDC + MSC 复合设备：插入 USB 后同时出现串口与只读 U 盘，
U 盘把 eMMC（FATFS 设备 0）暴露给 PC；本机程序（离线采集记录、CDC 文件下载等）仍可通过
FatFS 访问 eMMC，两侧通过仲裁模块互斥。

已确认的决策：

| 项 | 决策 |
| --- | --- |
| 枚举时机 | 插上 USB 即同时枚举（串口 + 磁盘一起出现），不做二选一 |
| 主机侧写保护 | 只读：IsWriteProtected 恒 1，Write 回调直接失败 |
| 仲裁策略 | 主机独占 + 本机暂停：仅主机发起真实数据读（READ10）才触发接管；INQUIRY / READ CAPACITY / TEST UNIT READY 等探测不触发 |
| 释放条件 | 主机停止数据读 10s（EMMC_MEDIA_HOLD_MS），或 USB 拔出/挂起 |

---

## 2. 总体设计

### 2.1 复合设备构成

- IAD 复合设备：bDeviceClass/SubClass/Protocol = 0xEF / 0x02 / 0x01
- 接口 0/1 = CDC ACM（数据 + 通知），接口 2 = MSC（BBB, SCSI 协议）
- PID 由 0x5740 改为 0x5741，配置字符串改为 "Composite Config"，bcdDevice → 0x0110

### 2.2 端点与 FIFO 分配

| 端点 | 用途 | Tx FIFO |
| --- | --- | --- |
| EP0 | 控制 | 0x40 (256B) |
| EP1 IN/OUT | CDC bulk 数据 | 0x80 (512B) |
| EP2 IN | CDC notify（中断） | 0x10 (64B) |
| EP2 OUT | MSC bulk OUT | 共享 Rx FIFO |
| EP3 IN | MSC bulk IN | 0x80 (512B) |

Rx FIFO 0x200 (2048B)。SetTxFiFo 按端点号升序调用（0→1→2→3），满足 HAL 要求。

### 2.3 eMMC 双访问仲裁（emmc_arbitrator）

分两层：

1. 扇区级互斥（硬保护，Level-1）
   - 本机 disk_read/write/ioctl/initialize 进出时 emmc_local_lock()/unlock()；
   - MSC 侧 STORAGE_IsReady / STORAGE_Read 检测 emmc_local_busy()，忙则快速失败，
     SCSI 层发出 NOT_READY sense，Windows 自动重试；
   - MSC 回调运行在 USB 中断上下文，本机传输依赖 SDMMC 中断完成，
     因此中断内绝不可忙等本机操作（会死锁），只允许"检查并快速失败"。

2. 文件系统级接管（策略）
   - STORAGE_Read 成功路径调用 emmc_msc_touch()（仅写时间戳标记，可在中断内调用），
     INQUIRY/READ CAPACITY/TUR 不调用 → 不触发接管；
   - 主循环每圈调用 emmc_media_poll()：满足条件后置 emmc_media_taken()=1，
     此后本机所有 FatFS 入口（diskio 层）快速返回 RES_NOTRDY / STA_NOINIT；
   - 接管前调用 emmc_fs_suspend_hook()（weak，offline_processor 覆盖实现：
     把写缓存刷盘，保证主机读到最新数据）；释放时调用 emmc_fs_resume_hook()（weak 空实现）；
   - USB 拔出/挂起（g_usb_conn_state == 0）立即释放；
   - CDC 文件下载会话期间调用 emmc_set_defer(1) 推迟接管，会话结束（STOP 或错误路径）
     调用 emmc_set_defer(0)，避免长写会话被主机访问打断。

接管状态机只在主循环上下文修改 s_media_taken，USB 中断只写 s_media_tick / s_media_req 标记。

---

## 3. 修改文件清单

### 新增文件

| 文件 | 说明 |
| --- | --- |
| Middlewares/FATFS/source/emmc_arbitrator.h | 仲裁模块 API：emmc_local_lock/unlock/busy、emmc_msc_touch、emmc_media_taken、emmc_media_poll、emmc_set_defer、EMMC_MEDIA_HOLD_MS=10000、weak 钩子声明 |
| Middlewares/FATFS/source/emmc_arbitrator.c | 仲裁状态机实现（见 2.3），extern g_usb_conn_state |
| Middlewares/USB/USB_APP/usbd_msc_storage_if.h | 导出 USBD_MSC_fops |
| Middlewares/USB/USB_APP/usbd_msc_storage_if.c | MSC 只读后端：Init(空操作)、GetCapacity(返回总扇区数)、IsReady(本机忙时返回未就绪)、IsWriteProtected(恒 1)、Read(中断内、独立 32B 对齐 512B 缓冲 s_msc_sector_buf、先 touch 再检查忙)、Write(-1)、GetMaxLun(0)、36 字节 INQUIRY（Vendor ALIENTEK / Product eMMC Disk） |

### 修改文件

| 文件 | 改动 |
| --- | --- |
| Middlewares/USB/USB_APP/usbd_conf.h | USBD_MAX_NUM_INTERFACES 1→3；新增 USE_USBD_COMPOSITE=1、USBD_CMPSIT_ACTIVATE_CDC/MSC=1、USBD_COMPOSITE_USE_IAD=1、USBD_CMPST_MAX_CONFDESC_SZ=300 |
| Middlewares/USB/USB_APP/usbd_desc.c | PID 0x5740→0x5741；配置字符串 "Composite Config"；设备类 0xEF/0x02/0x01；bcdDevice 0x0110 |
| Middlewares/USB/USB_APP/usbd_conf.c | 追加 HAL_PCDEx_SetTxFiFo(EP2, 0x10)、SetTxFiFo(EP3, 0x80) |
| User/usb_processor.c | usb_init() 改为复合注册序列（见下）；新增 include usbd_msc.h / usbd_msc_storage_if.h / emmc_arbitrator.h；下载会话 USB_DownloadFileStart 置 emmc_set_defer(1)，Stop 与 f_read 错误路径置 emmc_set_defer(0) |
| Middlewares/FATFS/source/diskio.c | include emmc_arbitrator.h；disk_initialize / disk_read / disk_write / disk_ioctl 入口加 emmc_media_taken() 门控（返回 STA_NOINIT / RES_NOTRDY），重试与 emmc_reinit 全过程包在 emmc_local_lock()/unlock() 内 |
| User/ida_config.c | include emmc_arbitrator.h；主循环 while(1) 内 sd_disk_poll() 之后调用 emmc_media_poll() |
| User/offline_processor.c | include emmc_arbitrator.h；HandleRecordStart / OfflineDatasRecord 开头加 emmc_media_taken() 早退（不置错误态，暂停记录写入，释放后继续）；strong 实现 emmc_fs_suspend_hook()（接管前 cache_flush 落盘） |
| Middlewares/USB/USB_APP/usbd_cdc_if.c | 复合模式下 USBD_CDC_SetTxBuffer / USBD_CDC_TransmitPacket 增加 ClassId 参数（新增 USBD_CDC_CLASS_ID=0，CDC 为最先注册的 class） |
| Middlewares/MALLOC/malloc.h | MEM1（AXI-SRAM）池上限 0x62000→0x60000（缩小 8KB）：新增 MSC/复合静态数据约 1.3KB 使 .ANY 区域溢出，缩小本就富裕的 mem1 堆池腾出空间 |
| Projects/MDK-ARM/atk_h7r3.uvprojx | 新增源文件：Middlewares/USB_CLASS 组加入 usbd_msc.c、usbd_msc_bot.c、usbd_msc_scsi.c、usbd_msc_data.c、usbd_composite_builder.c（CompositeBuilder/Src）；Middlewares/USB_APP 组加入 usbd_msc_storage_if.c；Middlewares/FATFS 组加入 emmc_arbitrator.c；IncludePath 追加 Class\MSC\Inc 与 Class\CompositeBuilder\Inc（Define 保持 USE_HAL_DRIVER,STM32H7R3xx,USE_USB_HS 不变） |

### 复合注册顺序（User/usb_processor.c:379，顺序硬约束）

```
USBD_Init(...);                                      // classId = 0
USBD_CDC_RegisterInterface(..., &USBD_CDC_fops);     // pUserData[0]
USBD_RegisterClassComposite(CDC, cdc_ep_addr);       // classId 0 → 1
USBD_MSC_RegisterStorage(..., &USBD_MSC_fops);       // pUserData[1]
USBD_RegisterClassComposite(MSC, msc_ep_addr);       // classId 1 → 2
USBD_Start(...);
```

原因：pUserData 按 pdev->classId 索引，必须先注册 fops 再注册 class。
端点数组顺序由 CompositeBuilder 固定为 [IN bulk, OUT bulk, (IN intr)]：
cdc_ep_addr = {0x81, 0x01, 0x82}，msc_ep_addr = {0x83, 0x02}。

---

## 4. 关键设计说明

1. MSC 回调在 USB 中断上下文执行，Read 内使用独立的 32B 对齐缓冲（DMA + DCache 要求），
   不可复用 diskio.c 的中转缓冲（分属不同上下文）。
2. GetCapacity 返回总扇区数，SCSI 层自行 -1 得最后 LBA；返回 -1 时主机得到 NOT_READY。
3. 本机 FatFS 调用点（离线记录、CDC 下载、读取配置文件等）无需逐个加锁，
   统一在 diskio 层门控 + 互斥，改动面最小。
4. PC 侧"弹出 U 盘"（START STOP UNIT）不会回调 storage fops，
   释放依赖 10s 无数据读自然超时；期间本机 eMMC 访问被暂停，需拔线或停止读取后恢复。
5. usb_init()（ida_config.c:507）先于 disk_initialize（:522）执行：
   USB 立即枚举，eMMC 未初始化完成前 MSC 报 NOT_READY，主机稍后自动重试，无影响。

---

## 5. 验证情况

- **编译验证通过**（Keil MDK V6.14，ARMCLANG，2026-10-17）：
  全量 Rebuild 0 Error / 0 Warning。
  Program Size: Code=153958, RO-data=187438, RW-data=524, ZI-data=9011836
  （ZI 含 mem2 HyperRAM 堆池；代码 + RO 在 0x90100000 外部 XSPI NOR，RAM 在 0x24000000 AXI-SRAM）
- 编译过程中发现并修复的两个问题：
  1. usbd_cdc_if.c：复合模式下 SetTxBuffer/TransmitPacket 需传 ClassId 参数；
  2. 链接 L6406E：新增静态数据使 RW_IRAM1 .ANY 区溢出 0x51c 字节 →
     mem1 堆池上限缩小 8KB（0x62000→0x60000，池仍有约 390KB，实际使用远小于此）。
- 已完成的静态审查：
  - 注册顺序、pUserData/classId 索引、EpAdd 索引顺序与 CompositeBuilder 源码一致；
  - inquiry 数组长度核对（曾发现 40 字节初始化超出 STANDARD_INQUIRY_DATA_LEN=36，已修正）；
  - diskio.c 门控/锁的所有返回路径已确认成对 lock/unlock；
  - uvprojx 中 usbd_composite_builder.c 路径已由误写的 Core\Src 修正为 Class\CompositeBuilder\Src；
  - FIFO 升序、总占用 748 words (2992B) 未超硬件限制。

## 6. 遗留风险与建议测试

1. 实机 USB 枚举：确认 Windows 同时出现 CDC 串口与 "eMMC Disk" 只读 U 盘；写入应被拒绝。
2. 仲裁压力测试：U 盘拷贝大文件的同时让本机做离线记录/CDC 下载，
   观察主机侧重试（拷贝短暂变慢）与本机记录暂停/恢复是否正常、数据是否损坏。
3. PC 弹出后 10s 内本机 eMMC 仍不可用（按设计），如不满足需求可改为解析 START STOP UNIT
   或缩短 EMMC_MEDIA_HOLD_MS。
4. CDC 文件下载期间主机读盘 → 因 defer 不接管，主机读到的文件内容可能滞后，
   下载结束后自动恢复正常；
5. 高负载下 SCSI NOT_READY 重试对 Windows 拷贝速度的影响需实测。
6. 编译时留意新增中文注释编码：相关源文件均为 UTF-8，MDK 已加 -Wno-invalid-source-encoding。
