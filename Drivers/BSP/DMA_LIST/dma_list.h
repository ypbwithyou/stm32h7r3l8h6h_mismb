#ifndef __DMA_H
#define __DMA_H

#include "./SYSTEM/sys/sys.h"
#include "./BSP/ADS8319/ads8319.h"

void dma_list_rtx_init(void);
HAL_StatusTypeDef dma_ads8319_start_frame(void);
uint8_t dma_ads8319_frame_busy(void);
void dma_ads8319_watchdog_kick(void);
void dma_ads8319_get_stats(uint32_t *start_req,
                           uint32_t *start_ok,
                           uint32_t *start_fail,
                           uint32_t *cplt_irq,
                           uint32_t *timeouts,
                           uint32_t *errors);
int32_t dma_ads8319_get_last_start_status(void);
uint32_t dma_ads8319_get_start_busy_count(void);
void dma_start_transfer(void);
void dma_list_data_init(void);
uint8_t dma_get_ready_data(uint32_t *node_index);

#endif
