/******************************************************************************
  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to any 
    third party.
  ------------------------------------------------------------------------
 * @file        app_collector.h
 * @author      Iris.Zang
 * @version     V1.0
 * @date        2025-12-21
 * @brief       ADC collecting
 * @license     Copyright (c) 2020-2032
*******************************************************************************/

#ifndef __APP_COLLECTOR_H
#define __APP_COLLECTOR_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* ADC数据采集任务 */
void CollectorTask(void *pvParameters);

#endif /* __APP_COLLECTOR_H */