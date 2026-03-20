#include "dataType.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

const Dev_ch_cfg_index g_HighPassFreq[] =
    {
        {0.05f, HIGH_PASS_FREQ_005hz},
        {0.5f, HIGH_PASS_FREQ_05hz},
        {1.0f, HIGH_PASS_FREQ_1hz},
        {10.0f, HIGH_PASS_FREQ_10hz},
};
const uint32_t g_HighPassFreqCount = ARRAY_SIZE(g_HighPassFreq);

const Dev_ch_cfg_index g_LowPassFreq[] =
    {
        {0.0f, LOW_PASS_FREQ_110kHz},
        {20.0f, LOW_PASS_FREQ_20Hz},
        {1000.0f, LOW_PASS_FREQ_1kHz},
        {10000.0f, LOW_PASS_FREQ_10kHz},
        {20000.0f, LOW_PASS_FREQ_20kHz},
};
const uint32_t g_LowPassFreqCount = ARRAY_SIZE(g_LowPassFreq);

const Dev_ch_cfg_index g_ida_ch_rate[] =
    {
        {50.0f, SAMPLE_RATE_INDEX_50HZ},
        {100.0f, SAMPLE_RATE_INDEX_100H},
        {200.0f, SAMPLE_RATE_INDEX_200H},
        {400.0f, SAMPLE_RATE_INDEX_400H},
        {512.0f, SAMPLE_RATE_INDEX_512HZ},
        {800.0f, SAMPLE_RATE_INDEX_800HZ},
        {1024.0f, SAMPLE_RATE_INDEX_1024HZ},
        {1600.0f, SAMPLE_RATE_INDEX_1600HZ},
        {2048.0f, SAMPLE_RATE_INDEX_2048HZ},
        {3200.0f, SAMPLE_RATE_INDEX_3200HZ},
        {4096.0f, SAMPLE_RATE_INDEX_4096HZ},
        {6400.0f, SAMPLE_RATE_INDEX_6400HZ},
        {8192.0f, SAMPLE_RATE_INDEX_8192HZ},
        {12800.0f, SAMPLE_RATE_INDEX_12800HZ},
        {16384.0f, SAMPLE_RATE_INDEX_16384HZ},
        {25600.0f, SAMPLE_RATE_INDEX_25600HZ},
        {32768.0f, SAMPLE_RATE_INDEX_32768HZ},
        {51200.0f, SAMPLE_RATE_INDEX_51200HZ},
        {65536.0f, SAMPLE_RATE_INDEX_65536HZ},
        {102400.0f, SAMPLE_RATE_INDEX_102400HZ},
        // {131072.0f, SAMPLE_RATE_INDEX_131072HZ},
        // {204800.0f, SAMPLE_RATE_INDEX_204800HZ},
        // {256000.0f, SAMPLE_RATE_INDEX_256000HZ},
};
const uint32_t g_ida_ch_rate_count = ARRAY_SIZE(g_ida_ch_rate);

// 主卡设备信息
DeviceInfo g_dev_info;

// 子卡设备信息
SubDevicelnfo g_SubDevicelnfo[SUBDEV_NUM_MAX];
