


/****************************************************************************** 
  Copyright (c) 2019-2021  (Xi'an) Testing & Control Technologies Co., Ltd. 
  All rights reserved. 
 
  * This software can only be used for  production/manufacturing equipments. 
  * Without permission, it cannot be copied and used in any form or provided to  
    any third party. 
  ------------------------------------------------------------------------ 
 
  *  Filename:      common_processor.h 
  * 
  *  Description:    
  *  
  *  Author:        
  * 
  *  $Revision:     1.0 $ 
  * 
*******************************************************************************/ 
#ifndef __APP_COMMON_H_ 
#define __APP_COMMON_H_ 
 
 
/****************************************************************************** 
* Include files. 
******************************************************************************/ 
#include <stdint.h> 
#include "ida_config.h"
 
#ifdef __cplusplus 
extern "C" { 
#endif 
/****************************************************************************** 
* Macros. 
******************************************************************************/ 

// 升级功能ON
#define UPGRADE_ON  (1)

#define RET_OK      (0) 
#define RET_ERROR   (-1) 
#define RET_HTTP_ERROR          (-2) 
#define RET_HTTP_OK             (-3) 
#define RET_ERROR_FTP_DOWNLOAD  (-4) 
#define RET_ERROR_CHECK_MD5     (-5) 
#define RET_NO_ACK              (-6) 
// 判断结果  超时 
#define RET_TIMEOUT_ERROR       (-7) 
 
#define UN_USED(x)  (void)(x) 
 
#define DELAY_500MS             (500000) 
#define DELAY_100MS             (100000) 
#define DELAY_50MS              (50000) 
#define DELAY_10MS              (10000) 
#define DELAY_5MS               (5000) 
#define DELAY_1MS               (1000) 
 
#define SIZE_STR_20             (20) 
#define SIZE_STR_50             (50) 
#define SIZE_STR_100            (100) 
#define SIZE_STR_256            (256) 
#define SIZE_STR_1024           (1024) 
#define SIZE_STR_2048           (2048) 

#define ON                      (1)
#define OFF                     (0)
 
// 测试数据 
// #define USE_TEST_DATA           0 
/****************************************************************************** 
* Variables. 
******************************************************************************/ 
 
/****************************************************************************** 
* Functions. 
******************************************************************************/ 
 
#ifdef __cplusplus 
} 
#endif 
 
#endif //__APP_COMMON_H_ 
 