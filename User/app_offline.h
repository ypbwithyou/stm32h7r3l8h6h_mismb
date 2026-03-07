/******************************************************************************
  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to any 
    third party.
  ------------------------------------------------------------------------
 * @file        app_offline.h
 * @author      Iris.Zang
 * @version     V1.0
 * @date        2026-1-21
 * @brief       ADC collecting
 * @license     Copyright (c) 2020-2032
*******************************************************************************/

#ifndef __APP_OFFLINE_H
#define __APP_OFFLINE_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* 离线采集任务 */
void OfflineTask(void *pvParameters);

#endif /* __APP_OFFLINE_H */