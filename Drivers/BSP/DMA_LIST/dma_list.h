 
#ifndef __DMA_H
#define __DMA_H

#include "./SYSTEM/sys/sys.h"

/* 功能定义 */
#define DMA_MAX_NODE    10  /* DMA链表最大节点数量 */

/* 函数声明 */
void dma_init(uint32_t *bufaddr, uint32_t *bufsize, uint32_t bufnum);   /* 初始化DMA */
void dma_start_transfer(void);                                          /* 开启DMA传输 */

#endif
