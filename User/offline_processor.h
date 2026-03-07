#ifndef __OFFLINE_PROCESSOR_H
#define __OFFLINE_PROCESSOR_H

#include <stdint.h>

// enum offline_status{
//     OFFLINE_IDLE = 0,       //
//     OFFLINE_INIT,
//     OFFLINE_CONFIGURED,
////    OFFLINE_SCHEDULE,
//    OFFLINE_RUN
//};
enum ScheduleActionType
{
    Customize_Action = 1000,
    Loop_Begin,
    Loop_End,

    Record_Start,
    Record_End,
    Output_Open,
    Output_Close,
    DigitOutput_Send,
    ACQ_Start,
    ACQ_Stop,
    Limit_TurnOn,
    Limit_TurnOff,
    Trigger_Record_Offline_Dsp,
};
enum offline_record_status
{
    RECORD_IDLE = 0,
    RECORD_RUN,
    RECORD_PAUSE,
    RECORD_STOP,
    RECORD_ERROR,

};
enum collect_cfg_status
{
    COLLECT_CFG_ERR = -1,
    COLLECT_CFG_INIT,
    COLLECT_CFG_OK,
};

void SysRunStatusInit(void);
void offline_processor(uint8_t mode);

#endif /* __OFFLINE_PROCESSOR_H */
