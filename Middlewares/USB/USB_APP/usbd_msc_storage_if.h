/**
  ****************************************************************************************************
  * @file        usbd_msc_storage_if.h
  * @brief       USB MSC (只读U盘) 后端：把 eMMC 暴露给 USB 主机
  ****************************************************************************************************
  */

#ifndef __USBD_MSC_STORAGE_IF_H
#define __USBD_MSC_STORAGE_IF_H

#include "usbd_msc.h"

#ifdef __cplusplus
extern "C" {
#endif

extern USBD_StorageTypeDef USBD_MSC_fops;

#ifdef __cplusplus
}
#endif

#endif
