#ifndef _DATATYPE_H_
#define _DATATYPE_H_

#include "./LIBS/lib_usb_protocol/usb_protocol.h"

typedef struct
{
    float ch_cfg_value;
    uint32_t ch_set_index;
} Dev_ch_cfg_index;

extern const Dev_ch_cfg_index g_HighPassFreq[];
extern const uint32_t g_HighPassFreqCount;

extern const Dev_ch_cfg_index g_LowPassFreq[];
extern const uint32_t g_LowPassFreqCount;

extern const Dev_ch_cfg_index g_ida_ch_rate[];
extern const uint32_t g_ida_ch_rate_count;

#endif
