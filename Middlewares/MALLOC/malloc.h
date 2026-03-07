/**
 ****************************************************************************************************
 * @file        malloc.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2024-05-21
 * @brief       内存管理驱动代码
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 * 
 * 实验平台:正点原子 H7R3开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 * 
 ****************************************************************************************************
 */

#ifndef __MALLOC_H
#define __MALLOC_H

#include "./SYSTEM/sys/sys.h"

/**
 * 定义所使用HyperRAM的规格
 * 
 * 0: HyperRAM容量为8MB（默认）
 * 1: HyperRAM容量为32MB
 */
#define CONFIG_HYPERRAM_HIGH_SPEC 0

/* 内存池编号定义 */
#define SRAMIN      0       /* AXI-SRAM1~4内存池,AXI-SRAM1~4共456KB */
#define SRAMEX      1       /* XSPI2 HyperRAM内存池,XSPI2 HyperRAM共32MB */
#define SRAM12      2       /* AHB-SRAM1/2内存池,AHB-SRAM1~2,共32KB */
#define SRAMDTCM    3       /* DTCM内存池,DTCM共64KB,此部分内存仅CPU和HPDMA(通过AHB)可以访问!!!! */
#define SRAMITCM    4       /* ITCM内存池,DTCM共64KB,此部分内存仅CPU和HPDMA(通过AHB)可以访问!!!! */

#define SRAMBANK    5       /* 定义支持的SRAM块数. */


/* 内存管理表类型定义,需保证(((1 << (sizeof(MT_TYPE) * 8)) - 1) * MEMx_BLOCK_SIZE) >= MEMx_MAX_SIZE */
#define MT_TYPE     uint32_t

/* mem1内存参数设定.mem1是H7R内部的AXI-SRAM1~4 */
#define MEM1_BLOCK_SIZE         (64)                                                                                                /* 内存块大小为64字节 */
#define MEM1_MAX_SIZE           ((0x00062000 / (MEM1_BLOCK_SIZE + sizeof(MT_TYPE))) * MEM1_BLOCK_SIZE)                              /* AXI-SRAM1~4最大空闲0x00072000字节 */
#define MEM1_ALLOC_TABLE_SIZE   (MEM1_MAX_SIZE / MEM1_BLOCK_SIZE)                                                                   /* 内存表大小 */

/* mem2内存参数设定.mem2是H7R外部的XSPI2 HyperRAM */
#define MEM2_BLOCK_SIZE         (64)                                                                                                /* 内存块大小为64字节 */
#if (CONFIG_HYPERRAM_HIGH_SPEC == 0)
#define MEM2_MAX_SIZE           ((0x00800000 / (MEM2_BLOCK_SIZE + sizeof(MT_TYPE))) * MEM2_BLOCK_SIZE)                              /* XSPI2 HyperRAM空闲空间 */
#else
#define MEM2_MAX_SIZE           ((0x02000000 / (MEM2_BLOCK_SIZE + sizeof(MT_TYPE))) * MEM2_BLOCK_SIZE)                              /* XSPI2 HyperRAM空闲空间 */
#endif
#define MEM2_ALLOC_TABLE_SIZE   (MEM2_MAX_SIZE / MEM2_BLOCK_SIZE)                                                                   /* 内存表大小 */

/* mem3内存参数设定.mem3是H7R内部的AHB-SRAM1~2 */
#define MEM3_BLOCK_SIZE         (64)                                                                                                /* 内存块大小为64字节 */
#define MEM3_MAX_SIZE           ((0x00008000 / (MEM3_BLOCK_SIZE + sizeof(MT_TYPE))) * MEM3_BLOCK_SIZE)                              /* AHB-SRAM1~2空闲0x00008000字节 */
#define MEM3_ALLOC_TABLE_SIZE   (MEM3_MAX_SIZE / MEM3_BLOCK_SIZE)                                                                   /* 内存表大小 */

/* mem4内存参数设定.mem4是H7R内部的DTCM */
#define MEM4_BLOCK_SIZE         (64)                                                                                                /* 内存块大小为64字节 */
#define MEM4_MAX_SIZE           ((0x00010000 / (MEM4_BLOCK_SIZE + sizeof(MT_TYPE))) * MEM4_BLOCK_SIZE)                              /* DTCM空闲0x00010000字节 */
#define MEM4_ALLOC_TABLE_SIZE   (MEM4_MAX_SIZE / MEM4_BLOCK_SIZE)                                                                   /* 内存表大小 */

/* mem5内存参数设定.mem5是H7R内部的ITCM */
#define MEM5_BLOCK_SIZE         (64)                                                                                                /* 内存块大小为64字节 */
#define MEM5_MAX_SIZE           ((0x00010000 / (MEM5_BLOCK_SIZE + sizeof(MT_TYPE))) * MEM5_BLOCK_SIZE)                              /* ITCM空闲0x00010000字节 */
#define MEM5_ALLOC_TABLE_SIZE   (MEM5_MAX_SIZE / MEM5_BLOCK_SIZE)                                                                   /* 内存表大小 */

/* 内存管理控制器 */
struct _m_mallco_dev
{
    void (*init)(uint8_t);          /* 初始化 */
    uint16_t (*perused)(uint8_t);   /* 内存使用率 */
    uint8_t *membase[SRAMBANK];     /* 内存池 管理SRAMBANK个区域的内存 */
    MT_TYPE *memmap[SRAMBANK];      /* 内存管理状态表 */
    uint8_t  memrdy[SRAMBANK];      /* 内存管理是否就绪 */
};

extern struct _m_mallco_dev mallco_dev; /* 在mallco.c里面定义 */


/* 用户调用函数 */
void my_mem_init(uint8_t memx);                          /* 内存管理初始化函数(外/内部调用) */
uint16_t my_mem_perused(uint8_t memx) ;                  /* 获得内存使用率(外/内部调用) */
void my_mem_set(void *s, uint8_t c, uint32_t count);     /* 内存设置函数 */
void my_mem_copy(void *des, void *src, uint32_t n);      /* 内存拷贝函数 */

void myfree(uint8_t memx, void *ptr);                    /* 内存释放(外部调用) */
void *mymalloc(uint8_t memx, uint32_t size);             /* 内存分配(外部调用) */
void *myrealloc(uint8_t memx, void *ptr, uint32_t size); /* 重新分配内存(外部调用) */

#endif













