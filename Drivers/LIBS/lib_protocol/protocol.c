/******************************************************************************
  Copyright (c) 2021-2023  (Xi'an) Testing & Control Technologies Co., Ltd.
  All rights reserved.

  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to 
    any third party.
  ------------------------------------------------------------------------

  *  Filename:      protocol.c
  *
  *  Description:   
  * 
  *  Author:        
  *
  *  $Revision:     1.0 $
  *
*******************************************************************************/


/******************************************************************************
* Include files.
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <arpa/inet.h>
//#include <pthread.h>

//#include "log.h"
#include "protocol.h"
#include "protocol_str.h"
#include "frame_reply.h"
#include "frame_analys.h"
#include "ErrCode.h"

/******************************************************************************
* Macros.
******************************************************************************/
#define swab64(x) ((uint64_t)(\
                            (((uint64_t)(x)& (uint64_t)0x00000000000000ffULL) << 56) | \
                            (((uint64_t)(x)& (uint64_t)0x000000000000ff00ULL) << 40) | \
                            (((uint64_t)(x)& (uint64_t)0x0000000000ff0000ULL) << 24) | \
                            (((uint64_t)(x)& (uint64_t)0x00000000ff000000ULL) << 8)  | \
                            (((uint64_t)(x)& (uint64_t)0x000000ff00000000ULL) >> 8)  | \
                            (((uint64_t)(x)& (uint64_t)0x0000ff0000000000ULL) >> 24) | \
                            (((uint64_t)(x)& (uint64_t)0x00ff000000000000ULL) >> 40) | \
                            (((uint64_t)(x)& (uint64_t)0xff00000000000000ULL) >> 56)))
typedef union d_long
{
    double      data_d;
    long int   data_l;
}d_long;
typedef union f_int
{
    float   data_f;
    int     data_i;
}f_int;
float HtoN_float(float value)
{
    f_int A;
    A.data_f = value;
//    A.data_i = htonl(A.data_i);
    return A.data_f;
}
float NtoH_float(float value)
{
    f_int A;
    A.data_f = value;
//    A.data_i = ntohl(A.data_i);
    return A.data_f;
}
double SW_double(double value)
{
    d_long A;
    A.data_d = value;
    A.data_l = swab64(A.data_l);
    return A.data_d;
}
/******************************************************************************
* Variables.
******************************************************************************/
typedef ProtocolStr * (*protocol_handler)(ProtocolStr *data_in);
static ProtocolStr * GetUserData_DevDiscover(ProtocolStr *data_in);
static ProtocolStr * GetUserData_DevConnect(ProtocolStr *data_in);
static ProtocolStr * GetUserData_ArmUpgrade(ProtocolStr *data_in);

static ProtocolStr * GetUserData_NoData(ProtocolStr *data_in);
static ProtocolStr * GetUserData_DspDispData(ProtocolStr *data_in);
static ProtocolStr * GetUserData_DevConfigUpdateDone(ProtocolStr *data_in);
static ProtocolStr * GetUserData_CspStartDone(ProtocolStr *data_in);
static ProtocolStr * GetUserData_CspArmDevCfg(ProtocolStr *data_in);
static ProtocolStr * GetUserData_Offline_SetConfig(ProtocolStr *data_in);
static ProtocolStr * GetUserData_Inertial_SetConfig(ProtocolStr *data_in);
static ProtocolStr * GetUserData_ArmUpdate(ProtocolStr *data_in);
static ProtocolStr * GetUserData_RecordBeginDone(ProtocolStr *data_in);
static ProtocolStr * GetUserData_RecordDspData(ProtocolStr *data_in);
static ProtocolStr * GetUserData_AnalogOutput(ProtocolStr *data_in);

static ProtocolStr * GetUserData_Contrl_DspReadTeds(ProtocolStr *data_in);
static ProtocolStr * GetUserData_File_Calibration_write(ProtocolStr *data_in);
static ProtocolStr * GetUserData_DVS_INIT_DSP2DSP(ProtocolStr *data_in);

static ProtocolStr * GetUserData_TransferDirectly(ProtocolStr *data_in);

typedef struct 
{
    uint16_t                frame_tag;              //frame id
    protocol_handler        process_handle;         //protocol handler
}ProtocoItems;

/*接收数据解包*/ 
static const ProtocoItems protocol_getdata[] = 
{
    {DVS_INIT_DETECT_OK,                GetUserData_DevDiscover},
    {DVS_INIT_CONNECT_OK,               GetUserData_DevConnect},
    {DVS_INIT_DEVCONFIG_UPDATE_Done,    GetUserData_DevConfigUpdateDone},
    {DVS_INIT_ARM_UPGRADE_DONE,         GetUserData_ArmUpgrade},  
    // {DVS_READ_TEDS_INFO,                GetUserData_DspRTeds},
    {DVS_CSP_START_Done,                GetUserData_CspStartDone},
    {DVS_CSP_ARM_CONFIG,                GetUserData_CspArmDevCfg},

    {DVS_DISPNEXT_OK,                   GetUserData_DspDispData},
    {DVS_ARM_UPDATE,                    GetUserData_ArmUpdate},

    {DVS_PREPROC_OUTPUT_UPDATE_DONE,    GetUserData_AnalogOutput},

    {DVS_FILE_RECORD_BEGIN_DONE,        GetUserData_RecordBeginDone},
    {DVS_FILE_RECORD_DSP_DATA,          GetUserData_RecordDspData},

    {DVS_CSP_OFFLINE_SETCONFIG,         GetUserData_Offline_SetConfig},
    {DVS_CSP_ARM_CONFIG_INERTIAL,       GetUserData_Inertial_SetConfig},

    {DVS_READ_TEDS_INFO,                GetUserData_Contrl_DspReadTeds},

    {DVS_FILE_CALIBRATION_WRITE,        GetUserData_File_Calibration_write},

    {DVS_INIT_DSP2DSP,                  GetUserData_DVS_INIT_DSP2DSP},
    
};

static Frame_Attribute g_FrameDataHead = {0};
/******************************************************************************
* Functions.
******************************************************************************/
static int FrameDataDecode(FrameReply* data_frame);
static int FrameDataCrcCheck(FrameReply* data_frame);

static void FrameHeadInit(void);
static void SetFrameHead_Attribute(void) __attribute__((constructor));
static void Destruct(void) __attribute__((destructor));

static FrameHeadInfo* GetFrameHeadInfo(Frame_Attribute* frame_head);
static Frame_Attribute GetFrameAttributie(FrameHeadInfo* data_in);
#if NtoH_or_HtoN
static void SwitchHtoN_FreamHead(FrameHeadInfo* data_in);
static void SwitchNtoH_FreamHead(FrameHeadInfo* data_in);
static void SwitchNtoH_UserDataHead(UserDataHeadInfo* data_in);
static void SwitchNtoH_UserData_TcpConnect(DeviceDetailInfo* data_in);
#endif

char* datas_buf = NULL;
SemaphoreHandle_t g_protocol_xMutex;

// So 析构函数
static void SetFrameHead_Attribute(void)
{
    FrameHeadInit();
    if (NULL == datas_buf) {
        datas_buf = (char*)calloc(sizeof(char), FRAME_USER_DATA_MAX_SIZE);
    }
    g_protocol_xMutex = xSemaphoreCreateMutex();
}

// So 析构函数
static void Destruct(void)
{
}

// 帧属性初始化
static void FrameHeadInit(void)
{
    g_FrameDataHead.protocol_version = PROTOCOL_VERSION;       // 协议版本
    g_FrameDataHead.frame_encrypt = ENCRYP_DISABLE;            // 加密标志
    g_FrameDataHead.check_enable = CHECK_DISABLE;              // CRC标志
    g_FrameDataHead.data_bit_wide = DATA_DWORD;                // 数据位宽
    g_FrameDataHead.data_align_type = DATA_RIGHT_ALIGN;        // 数据对齐方式
    g_FrameDataHead.frame_num = 0;                             // 帧序号
}

// 帧属性设置与更新
void SetFrameAttribute(Frame_Attribute *set_attribute)
{
    if (!set_attribute)
    {
        return;
    }
    memcpy(&g_FrameDataHead, set_attribute, sizeof(Frame_Attribute));
}

/* 包头数据打包 */
static FrameHeadInfo* GetFrameHeadInfo(Frame_Attribute* frame_head)
{
    FrameHeadInfo *head_data = NULL;

    /* internal functions */
    if (!frame_head) {
        return NULL;
    }
    head_data = (FrameHeadInfo *)calloc(1, sizeof(FrameHeadInfo));
    if (!head_data)
    {
        return NULL;
    }

    head_data->hd_head = FRAME_HEAD;
    head_data->hd_attribute_ver = frame_head->protocol_version & 0xff;
    head_data->hd_attribute_flag = (frame_head->frame_encrypt & 0xf) << 4 | (frame_head->check_enable & 0xf);
    head_data->hd_data_status = (frame_head->data_bit_wide & 0xf) << 4 | (frame_head->data_align_type & 0xf);
    head_data->hd_serial_num = frame_head->frame_num;
    return head_data;
}
// 包头数据解析
static Frame_Attribute GetFrameAttributie(FrameHeadInfo* data_in)
{
    /* 内部函数 */
    Frame_Attribute data_out;

    data_out.protocol_version = data_in->hd_attribute_ver;
    data_out.frame_encrypt = (data_in->hd_attribute_flag >> 4) & 0x000f;
    data_out.check_enable = data_in->hd_attribute_flag & 0x000f;
    data_out.data_bit_wide = data_in->hd_data_status >> 4 & 0x000f;
    data_out.data_align_type = data_in->hd_data_status & 0x000f;
    data_out.frame_num = data_in->hd_serial_num;
    return data_out;
}

#if NtoH_or_HtoN

/*******************************************************
 * 字节序转换：Host to Net
 * *****************************************************/
static void SwitchHtoN_FreamHead(FrameHeadInfo* data_in)
{
    if (!data_in) {
        LOG_ERROR("SwitchHtoN_FreamHead() data_in is NULL", NULL);
        return;
    }
    data_in->hd_head = htonl(data_in->hd_head);
    data_in->hd_serial_num = htons(data_in->hd_serial_num);
}

/*******************************************************
 * 字节序转换：Net to Host
 * *****************************************************/
// 帧头数据字节序转换：network to host
static void SwitchNtoH_FreamHead(FrameHeadInfo* data_in)
{
    /* 内部函数 */
    if (!data_in) {
        LOG_ERROR("SwitchNtoH_FreamHead() data_in is NULL", NULL);
        return;
    }
    data_in->hd_head = ntohl(data_in->hd_head);
    data_in->hd_serial_num = ntohs(data_in->hd_serial_num);
}
// 用户数据head字节序转换：network to host
static void SwitchNtoH_UserDataHead(UserDataHeadInfo* data_in)
{
    /* 内部函数 */
    if (!data_in) {
        LOG_ERROR("SwitchNtoH_UserDataHead() data_in is NULL", NULL);
        return;
    }
    data_in->nIsValidFlag = ntohl(data_in->nIsValidFlag);
    data_in->nEventID = ntohl(data_in->nEventID);
    data_in->nSourceType = ntohl(data_in->nSourceType);
    data_in->nDestinationID = ntohl(data_in->nDestinationID);
    data_in->nParameters0 = ntohl(data_in->nParameters0);
    data_in->nParameters1 = ntohl(data_in->nParameters1);
    data_in->nParameters2 = ntohl(data_in->nParameters2);
    data_in->nParameters3 = ntohl(data_in->nParameters3);
    data_in->nDataLength = ntohl(data_in->nDataLength);
    data_in->DataFlag = ntohl(data_in->DataFlag);
}
// 用户数据data字节序转换：network to host: 设备注册失败
static void SwitchNtoH_UserData_TcpConnect(DeviceDetailInfo* data_in)
{
    if (!data_in) {
        LOG_ERROR("SwitchNtoH_UserData_TcpConnect() data_in is NULL", NULL);
        return;
    }
    data_in->ID = ntohs(data_in->ID);
    data_in->Reserved3 = ntohs(data_in->Reserved3);
    data_in->DeviceType = ntohl(data_in->DeviceType);
    data_in->IsConnected = ntohl(data_in->IsConnected);
    data_in->SystemID = ntohl(data_in->SystemID);
    data_in->IsMaster = ntohl(data_in->IsMaster);
    data_in->MasterID = ntohs(data_in->MasterID);
    data_in->ClientCount = ntohs(data_in->ClientCount);
    for (uint16_t i=0; i<STR_128; i++) {
        data_in->ClientsID[i] = ntohs(data_in->ClientsID[i]);
    }
    data_in->AIChannelNum = ntohl(data_in->AIChannelNum);
    data_in->AOChannelNum = ntohl(data_in->AOChannelNum);
    data_in->DIChannelNum = ntohl(data_in->DIChannelNum);
    data_in->DOChannelNum = ntohl(data_in->DOChannelNum);
    data_in->TachoChannelNum = ntohl(data_in->TachoChannelNum);
    data_in->GPSNum = ntohl(data_in->GPSNum);
    data_in->HighestSamplingFreq = NtoH_float(data_in->HighestSamplingFreq);
    data_in->ModeType = ntohl(data_in->ModeType);
    data_in->Reserved1 = ntohl(data_in->Reserved1);
    data_in->Reserved2 = ntohl(data_in->Reserved2);
}
// 用户数据data字节序转换：network to host: CSP_START ： ChannelTableHeader
static void SwitchNtoH_UserData_CspHead(ChannelTableHeader* data_in)
{
    if (!data_in) {
        LOG_ERROR("SwitchNtoH_UserData_CspHead() data_in is NULL", NULL);
        return;
    }
    data_in->nTotalChannelNum = ntohl(data_in->nTotalChannelNum);
    data_in->nHardwareSampleRateIndex = ntohl(data_in->nHardwareSampleRateIndex);
    data_in->nFlagIntDiff = ntohl(data_in->nFlagIntDiff);
    data_in->nICPCurrentIndex = ntohl(data_in->nICPCurrentIndex);
    data_in->nReserved1 = ntohl(data_in->nReserved1);
    data_in->nReserved2 = ntohl(data_in->nReserved2);
}

#endif
/***********************************************************************************************
* 函数名 : GetProtocolStrtoSend
* 函数功能描述 : 待发送数据组包
* 函数参数 : ProtocolStr* ，包含：data_index —— 帧ID, data_len —— 用户数据长度， data —— 用户数据指针
* 函数返回值 : ProtocolStr* 格式的包含包头、用户数据长度、用户数据、校验结果、包尾的完整数据包
* 作者 : Iris.Zang
* 函数创建日期 : 2022.2.4
* 函数修改记录 : 
* 注意：（1）组包时，先对用户数据进行加密，再讲加密后的用户数据追加至链表中，然后再计算CRC并追加相关数据至链表
*       （2）输入的data_in需提前完成数据的子继续转换，
* **********************************************************************************************/
ProtocolStr* GetProtocolStrtoSend(ProtocolStr* data_in, int destination)
{
    /* 内部函数 */
    uint32_t frame_end = FRAME_END;
    int ret = RET_OK;

    if (!data_in || data_in->data_len <= 0) {
//        LOG_ERROR("Missing data. data_in:%x data_l:%d.", data_in, data_in->data_len);
        return NULL;
    }
//    LOG_DEBUG("Preparing data with tag:0x%08x len:%d!", data_in->data_index, data_in->data_len);
//    LOG_DEBUG("GetProtocolStrtoSend step 0 ok!", NULL);

    /*step1:发送帧包头数据整理*/  
    FrameHeadInfo* head = GetFrameHeadInfo(&g_FrameDataHead);
    if (NULL == head)
    {
//        LOG_ERROR("GetProtocolStrtoSend GetFrameHeadInfo Err!", NULL);
        return NULL;
    }
    #if NtoH_or_HtoN
    if (DESTINATION_DSP_TO_PC == destination || DESTINATION_ARM_TO_PC == destination) {
        SwitchHtoN_FreamHead(head);
    }
    #endif
//    LOG_DEBUG("GetProtocolStrtoSend step 1 ok! destination:%d.", destination);
    
    /*step2:新建一个链表*/
    LinkList* l_list = LinkListCreate();
    if (!l_list)
    {
        free(head);
        return NULL;
    }
//    LOG_DEBUG("GetProtocolStrtoSend step 2 ok!", NULL);

    /*step3:追加包头数据至链表*/
    ret = LinkListPushBack(l_list, head, sizeof(FrameHeadInfo));
    if (ret)
    {
//        LOG_ERROR("Link the head data to the frame list Err!", NULL);
        LinkListDestroy(&l_list);
        free(head);
        return NULL;
    }
//    LOG_DEBUG("GetProtocolStrtoSend step 3 ok! list_len:%u", LinkListLength(l_list));
    free(head);

    /* step4.0: 追加用户数据长度至链表*/
    int32_t d_l = data_in->data_len;
    #if NtoH_or_HtoN
    d_l = (DESTINATION_DSP_TO_PC == destination || DESTINATION_ARM_TO_PC == destination)?(int)htonl(d_l):d_l;
    #endif
    ret = LinkListPushBack(l_list, &d_l, FREAM_LEN_LENGTH);
    if (ret)
    {
//        LOG_ERROR("Link the user data length to the frame list Err!", NULL);
        LinkListDestroy(&l_list);
        return NULL;
    }
//    LOG_DEBUG("GetProtocolStrtoSend step 4.0 ok! list_len:%u len:%d", LinkListLength(l_list), d_l);

    /*step4:追加用户数据至链表*/
    if (data_in->data_len != 0) {
        if (g_FrameDataHead.frame_encrypt != ENCRYP_DISABLE) {
            // 用户数据加密（只计算用户数据value部分，不包括tag和length）
            ret = DataEncode(g_FrameDataHead.frame_encrypt , data_in, &data_in);
            if (!ret) {
//                LOG_ERROR("DataEncode failed! tag: %04d", data_in->data_index);
                LinkListDestroy(&l_list);
                return NULL;
            }
//            LOG_DEBUG("GetProtocolStrtoSend step DataEncode ok!", NULL);
        }
    }
    if (LinkListPushBack(l_list, data_in->data, data_in->data_len)) {
//        LOG_ERROR("GetProtocolStrtoSend step 4 for user data error. ptr:%x len:%d", data_in->data, data_in->data_len);
        LinkListDestroy(&l_list);
        return NULL;
    }
//    LOG_DEBUG("GetProtocolStrtoSend step 4 ok! tag:%x data_len:%u list_len:%u", data_in->data_index, data_in->data_len, LinkListLength(l_list));

    // step4.5:用户数据校验及校验结果追加至链表
    uint32_t check_result = 0;
    check_result = (g_FrameDataHead.check_enable != CHECK_DISABLE && data_in->data_len != 0)?DataCrc(data_in):0;
    #if NtoH_or_HtoN
    check_result = (DESTINATION_DSP_TO_PC == destination || DESTINATION_ARM_TO_PC == destination)?htonl(check_result):check_result;
    #endif
    if (LinkListPushBack(l_list, &check_result, sizeof(check_result))) {
//        LOG_ERROR("GetProtocolStrtoSend step 4.5 for crc error.", NULL);
        LinkListDestroy(&l_list);
        return NULL;
    }
//    LOG_DEBUG("GetProtocolStrtoSend step 4.5 ok!", NULL);

    /*step5:追加帧尾至链表*/
    #if NtoH_or_HtoN
    frame_end = (DESTINATION_DSP_TO_PC == destination || DESTINATION_ARM_TO_PC == destination)?htonl(frame_end):frame_end;
    #endif
    ret = LinkListPushBack(l_list, &frame_end, sizeof(frame_end));
    if (ret) {
//        LOG_ERROR("Link the head data to the frame list Err!", NULL);
        LinkListDestroy(&l_list);
        return NULL;
    }
//    LOG_DEBUG("GetProtocolStrtoSend step 5 ok! list_len:%u", LinkListLength(l_list));

    /*step6:将链表数据转换为带长度的字符串输出*/
    ProtocolStr* ptr = GetProtocolStrFromLinkList(l_list);
    ptr->data_index = data_in->data_index;
//    LOG_DEBUG("GetProtocolStrtoSend step 6 ok! frame_len:%u list_len:%u", ptr->data_len, LinkListLength(l_list));

    /*step7:销毁链表及ProtocolStr*/
    LinkListDestroy(&l_list);
//    LOG_DEBUG("GetProtocolStrtoSend step 7 ok!", NULL);

    /*step8:返回带长度的字符串指针*/
    return ptr;
}
/***********************************************************************************************
*函数名 : ReceiveDataCheck()
*函数功能描述 : LinkList*型链表中获取整帧数据
*函数参数 : LinkList* 接收数据链表
*函数返回值 : ProtocolStr* 返回带长度的字符串数据（整帧数据）
*作者 : Iris.Zang
*函数创建日期 : 2022.2.12
*函数修改记录 : 
***********************************************************************************************/
// 解析一个帧, 入参:数据流, 链表形式.
ProtocolStr* GetFrameDataFromList(LinkList* data_receive)
{
    ProtocolStr* frame = NULL;
    // char datas_buf[FRAME_USER_DATA_MAX_SIZE];
    uint32_t frame_head = 0;
    uint32_t fream_check = 0;
    uint32_t frame_end = 0;
    uint32_t fream_data_len = 0;
    int frame_len = 0; 
    FrameStructInfo* fream_data;

    // lock
    if(xSemaphoreTake(g_protocol_xMutex, pdMS_TO_TICKS(10)) != 0)
    {
        perror("pthread_mutex_lock");
//        exit(EXIT_FAILURE);
    }
    if (NULL == data_receive || LinkListLength(data_receive) == 0) {
//        LOG_DEBUG("GetFrameDataFromList data_receive == NULL",NULL);
        xSemaphoreGive(g_protocol_xMutex);
        return NULL;
    }
    // LOG_DEBUG("GetFrameDataFromList data_receive != NULL", NULL);
    // memset(datas_buf, 0, FRAME_USER_DATA_MAX_SIZE);
    while ((uint32_t)LinkListLength(data_receive) >= FRAME_MIN_SIZE) {
        // step1. 从链表中获取最小长度的数据
        int len_val = LinkListGetData(data_receive, datas_buf, FRAME_MIN_SIZE);
        if (len_val != FRAME_MIN_SIZE) {
//            LOG_ERROR("LinkListGetData error! FRAME_MIN_SIZE: %d len_val: %d.", FRAME_MIN_SIZE, len_val);
            break;
        }
        fream_data = (FrameStructInfo*)datas_buf;
        frame_head = fream_data->fraem_head.hd_head;
        fream_data_len = fream_data->fream_data_length;
        #if NtoH_or_HtoN
        frame_head = ntohl(frame_head);
        fream_data_len = ntohl(fream_data_len);
        #endif
        // step2. 帧头标志校验
        if (FRAME_HEAD != frame_head) {   // 帧头标志校验，默认0X3C3C，校验失败则从链表POP一个字节后退出
//            LOG_DEBUG("GetFrameInfo, not find head: 0x%08X, got: 0x%08X", FRAME_HEAD, frame_head);
            LinkListPopLength(data_receive, 1);
            continue;
        }
        // step3. 帧用户数据长度校验
        if (fream_data_len > FRAME_USER_DATA_MAX_SIZE || fream_data_len < sizeof(UserDataHeadInfo)) {   
            // 用户数据长度校验，若用户数据长度大于最大长度或小于最小长度（用户数据头长度），则校验失败，从链表POP一个字节后退出
//            LOG_ERROR("GetFrameInfo, Userdata length err: %d. len_max:%d len_min:%d", fream_data_len, FRAME_USER_DATA_MAX_SIZE, sizeof(UserDataHeadInfo));
            LinkListPopLength(data_receive, 1);
            continue;
        } else { // 计算完整帧数据实际长度=帧头数据长度（12）+用户数据长度（4）+用户数据+校验位（2）+帧尾（2）
            frame_len = FRAME_MIN_SIZE + fream_data_len;
//            LOG_DEBUG("fream_data_len: %u frame_len:%u", fream_data_len, frame_len);
        }
        // step4. 获取整帧数据        
        int list_len = LinkListLength(data_receive);
        if (list_len < frame_len) {
            // 当前可能存在数据分包，即一帧数据被分成多个数据包传输，等待有足够的数据再处理
            break;
        }      
        if (LinkListGetData(data_receive, (char *)datas_buf, frame_len) != frame_len) {
//            LOG_DEBUG("LinkListGetData %d, error.", frame_len);
            break;
        } else {    // 获取帧尾标志数据，默认0X3E3E
            // LOG_HEX_DEBUG(datas_buf, frame_len, "get receive data ");
            memcpy(&frame_end, frame_len-FREAM_END_LENGTH+(char *)datas_buf, FREAM_END_LENGTH);
            #if NtoH_or_HtoN
            frame_end = ntohl(frame_end);
            #endif
            // 获取用户数据校验值
            memcpy(&fream_check, frame_len-FREAM_END_LENGTH-FREAM_CHECK_LENGTH+(char *)datas_buf, FREAM_CHECK_LENGTH);
            #if NtoH_or_HtoN
            fream_check = ntohl(fream_check);
            #endif
        }
        // step5. 帧尾标志校验
        if (FRAME_END != frame_end) {   // 校验失败则从链表POP一个字节后退出
//            LOG_ERROR("frame_end mismatch, expected: %04X, got: 0x%08X fream_d_l:0x%08x", FRAME_END, frame_end, fream_data_len);
            LinkListPopLength(data_receive, 1);
            continue;
        } 
//        else {
//            LOG_DEBUG("GetFrameDataFromList ok! head:0x%x len:%u end:0x%x", frame_head, frame_len, frame_end);
//        }
        // step6. 用户数据校验位校验
        ProtocolStr* user_data = CreatProtocolStr(fream_data_len);
        if (NULL == user_data) {
//            LOG_ERROR("CreatProtocolStr error.", NULL);
            break;
        } else {
            memcpy(user_data->data, datas_buf+FREAM_HEAD_LENGTH+FREAM_LEN_LENGTH, fream_data_len);
        }
        uint32_t user_check = DataCrc(user_data);
        if (fream_check != user_check) {
//            LOG_ERROR("fream_check mismatch, received: %d, got: %d.", fream_check, user_check);
            LinkListPopLength(data_receive, 1);
            FreeProtocolStr(&user_data);
            continue;
        } 
//        LOG_DEBUG("GetFrameDataFromList user data check OK ! fream_check:0x%08x 0x%08x.", fream_check, user_check);
        FreeProtocolStr(&user_data);
        // step7. 创建待返回结构，组织待返回数据，并POP链表中对应数据
        frame = CreatProtocolStr(frame_len);
        if (NULL == frame) {
//            LOG_ERROR("CreatProtocolStr err", NULL);
            break;
        }
        memcpy(frame->data, datas_buf, frame_len);
        frame->data_len = frame_len;
        if (LL_OK != LinkListPopLength(data_receive, frame_len)) {
//            LOG_ERROR("GetFrameDataFromList LinkListPopLength err! data_len:%d",frame->data_len);
            // break;
        }
        // unlock
		if(xSemaphoreGive(g_protocol_xMutex) != 0)
		{
			perror("pthread_mutex_unlock");
//			exit(EXIT_FAILURE);
		}
        return frame;
    }
    // unlock
    if(xSemaphoreGive(g_protocol_xMutex) != 0)
    {
        perror("pthread_mutex_unlock");
//        exit(EXIT_FAILURE);
    }
    return NULL;
}

/***********************************************************************************************
*函数名 : GetProtocolReplyFrameData
*函数功能描述 : ProtocolStr*型整帧数据解包
*函数参数 : ProtocolStr* 接收到的带长度的字符串（整帧字符串数据）
*函数返回值 : FrameReply* 返回应答，用户数据结构体，仅包含用户数据，不包含帧头、帧尾及帧校验信息
                frame_tag : nEventID
                Frame_Attribute frame_attribute;        // 数据属性
                frame_user_encrypt;        // 用户数据密文：指针和长度
                frame_user_decode;         // 用户数据明文：指针和长度
                frame_user_data;           // 可用用户数据：经过字节序转换的可用数据
*作者 : Iris.Zang
*函数创建日期 : 2022.2.4
*函数修改记录 : 解析数据时，先获取加密的用户数据，再对加密的数据进行CRC校验，CRC校验通过后再对加密的数据进行解密
***********************************************************************************************/
FrameReply* GetProtocolReplyFrameData(ProtocolStr* frame_data)
{
    int ret = RET_OK;
    uint32_t    frame_len;
    Frame_Attribute head_value;
    FrameHeadInfo data_head;

    if (!frame_data || frame_data->data == NULL) {
        return NULL;
    }
    // step1. 创建待返回数据结构
    FrameReply* data_out = CreatFrameReply();
    if (NULL == data_out) {
        return NULL;
    }

    // step2.0 获取包头数据
    memset(&data_head, 0, sizeof(data_head));
    memcpy(&data_head, frame_data->data, sizeof(FrameHeadInfo));
    // step2.1 包头数据字节序转换
    #if NtoH_or_HtoN
    SwitchNtoH_FreamHead(&data_head);
    #endif
    // step2.2 包头数据解析，获取包属性
    head_value = GetFrameAttributie(&data_head);     // 包属性解析
    memcpy(&data_out->frame_attribute, &head_value, sizeof(Frame_Attribute));

    // step3.0 获取用户数据长度
    memcpy(&frame_len, (char *)frame_data->data + FREAM_HEAD_LENGTH, FREAM_LEN_LENGTH);  
    #if NtoH_or_HtoN
    frame_len = ntohl(frame_len);
    #endif

    // step4.0 根据用户数据长度获取数据
    if (frame_len >= sizeof(UserDataHeadInfo)) {
        // step4.1 获取原始密文
        data_out->frame_user_encrypt = CreatProtocolStr(frame_len);
        if (!data_out->frame_user_encrypt) {
            FreeFrameReply(&data_out);
//            LOG_ERROR("CreatProtocolStr by frame_len:%d err.", frame_len);
            return NULL;
        }
        char* data_ptr = (char *)frame_data->data + FREAM_HEAD_LENGTH + FREAM_LEN_LENGTH;
        memcpy(data_out->frame_user_encrypt->data, data_ptr, frame_len);      // 获取密文
        data_out->frame_user_encrypt->data_len = frame_len;

        // step4.2 数据解密
        ret = FrameDataDecode(data_out);
        if (RET_OK != ret) {
            FreeFrameReply(&data_out);
            data_out = NULL;
            return data_out;
        }
        // step4.3 用户数据字节序转换
        UserDataHeadInfo *userdata_head = (UserDataHeadInfo*)(data_out->frame_user_decode->data);
        #if NtoH_or_HtoN
        SwitchNtoH_UserDataHead(userdata_head);
        #endif
        uint32_t frame_tag = userdata_head->nEventID;
        uint32_t frame_destination = userdata_head->nDestinationID;
        if (frame_len == sizeof(UserDataHeadInfo) && (userdata_head->nSourceType == SOURCE_TYPE_NO_DATA \
                        || userdata_head->nSourceType == SOURCE_TYPE_INFORM)) 
        { // 根据用户数据长度初步判断接收数据是否为标准帧（无数据帧或通知帧）
            // 若为标准帧则统一处理，不解析帧ID
            data_out->frame_user_data = GetUserData_NoData(data_out->frame_user_decode);
        } else {
            if (frame_destination == DESTINATION_DSP_TO_PC || frame_destination == DESTINATION_PC_TO_DSP) { 
                // 若该帧nDestinationID=0或6，即为DSP-->PC或PC-->DSP转发帧，则直接处理，无需解析
                data_out->frame_user_data = GetUserData_TransferDirectly(data_out->frame_user_decode);
            } else { // 其它帧则为携带数据，且所携带数据需要PS侧处理，根据nEventID选择处理函数
                for (unsigned char i = 0; i < sizeof(protocol_getdata)/sizeof(protocol_getdata[0]); i++) {
                    if(frame_tag == protocol_getdata[i].frame_tag) {
                        data_out->frame_user_data = protocol_getdata[i].process_handle(data_out->frame_user_decode);
                        break;
                    }
                }
            }
        }
    } 
    return data_out;
}
/*******************************************************
 * 接收数据包，用户数据解析，将已解密数据进行字节序转换后行程可识别的用户数据
 * *****************************************************/
/* static ProtocolStr * GetUserData_DspCheck(ProtocolStr *data_in)
{
    if (NULL == data_in || NULL == data_in->data) {
//        LOG_ERROR("GetUserData_DspCheck() data_in == NULL", NULL);
        return NULL;
    }

    ProtocolStr* data_reply = CreatProtocolStr(data_in->data_len);
    if (!data_reply) {
//        LOG_ERROR("GetUserData_DspCheck() calloc err", NULL);
        return NULL;
    }

    memcpy(data_reply->data, data_in->data, data_in->data_len);
    return data_reply;
} */


// 设备发现应答包数据包解析
static ProtocolStr * GetUserData_DevDiscover(ProtocolStr *data_in)
{
    if (NULL == data_in || NULL == data_in->data) {
        return NULL;
    }
    if (data_in->data_len != sizeof(UdpMsgRecv)) {
        return NULL;
    }
    ProtocolStr* data_reply = CreatProtocolStr(data_in->data_len);
    if (!data_reply) {
        return NULL;
    }
    memcpy(data_reply->data, data_in->data, data_in->data_len);
    return data_reply;
}
// 设备注册应答包数据解析
static ProtocolStr * GetUserData_DevConnect(ProtocolStr *data_in) 
{
    if (NULL == data_in || NULL == data_in->data) {
        return NULL;
    }

    ProtocolStr* data_reply = CreatProtocolStr(data_in->data_len);
    if (!data_reply) {
        return NULL;
    }
 
    memcpy(data_reply->data, data_in->data, data_in->data_len);
    data_reply->data_index = data_in->data_index;
    return data_reply;
}

static ProtocolStr * GetUserData_NoData(ProtocolStr *data_in) 
{
    if (NULL == data_in || NULL == data_in->data) {
        return NULL;
    }

    if (data_in->data_len != sizeof(UserDataHeadInfo)) {
        return NULL;
    }
    
    ProtocolStr* data_reply = CreatProtocolStr(data_in->data_len);
    if (!data_reply) {
        return NULL;
    }
    memcpy(data_reply->data, data_in->data, data_in->data_len);
    data_reply->data_index = data_in->data_index;
    return data_reply;
}

static ProtocolStr * GetUserData_ArmUpgrade(ProtocolStr *data_in)
{
    UserDataHeadInfo data_head;
    if (NULL == data_in || NULL == data_in->data || data_in->data_len<=0) {
        return NULL;
    }
    memcpy(&data_head, data_in->data, sizeof(UserDataHeadInfo));
    ProtocolStr* data_reply = CreatProtocolStr(data_in->data_len);
    if (!data_reply) {
        return NULL;
    }
    data_reply->data_index = data_head.nEventID;
    memcpy(data_reply->data, data_in->data, data_in->data_len);
    return data_reply;    
}
static ProtocolStr * GetUserData_ArmUpdate(ProtocolStr *data_in)
{
    UserDataHeadInfo data_head;
    if (NULL == data_in || NULL == data_in->data || data_in->data_len<=0) {
        return NULL;
    }
    memcpy(&data_head, data_in->data, sizeof(UserDataHeadInfo));
    ProtocolStr* data_reply = CreatProtocolStr(data_in->data_len);
    if (!data_reply) {
        return NULL;
    }
    data_reply->data_index = data_head.nEventID;
    memcpy(data_reply->data, data_in->data, data_in->data_len);
    return data_reply;    
}
static ProtocolStr * GetUserData_AnalogOutput(ProtocolStr *data_in)
{
    UserDataHeadInfo data_head;
    if (NULL == data_in || NULL == data_in->data || data_in->data_len<=0) {
        return NULL;
    }
    memcpy(&data_head, data_in->data, sizeof(UserDataHeadInfo));
    ProtocolStr* data_reply = CreatProtocolStr(data_in->data_len);
    if (!data_reply) {
        return NULL;
    }
    data_reply->data_index = data_head.nEventID;
    memcpy(data_reply->data, data_in->data, data_in->data_len);
    return data_reply;    
}
static ProtocolStr * GetUserData_RecordBeginDone(ProtocolStr *data_in)
{
    UserDataHeadInfo data_head;
    if (NULL == data_in || NULL == data_in->data || data_in->data_len<=0) {
        return NULL;
    }
    memcpy(&data_head, data_in->data, sizeof(UserDataHeadInfo));
    ProtocolStr* data_reply = CreatProtocolStr(data_in->data_len);
    if (!data_reply) {
        return NULL;
    }
    data_reply->data_index = data_head.nEventID;
    memcpy(data_reply->data, data_in->data, data_in->data_len);
    return data_reply;    
}

static ProtocolStr * GetUserData_RecordDspData(ProtocolStr *data_in)
{
    UserDataHeadInfo data_head;
    if (NULL == data_in || NULL == data_in->data || data_in->data_len<=0) {
        return NULL;
    }
    memcpy(&data_head, data_in->data, sizeof(UserDataHeadInfo));
    ProtocolStr* data_reply = CreatProtocolStr(data_in->data_len);
    if (!data_reply) {
        return NULL;
    }
    data_reply->data_index = data_head.nEventID;
    memcpy(data_reply->data, data_in->data, data_in->data_len);
    return data_reply;    
}
static ProtocolStr * GetUserData_DspDispData(ProtocolStr *data_in)
{
    if (NULL == data_in || NULL == data_in->data || data_in->data_len <= 0) {
        return NULL;
    }
    UserDataHeadInfo *userdata_head = (UserDataHeadInfo*)(data_in->data);
    // 待返回结构创建
    ProtocolStr * data_out = CreatProtocolStr(data_in->data_len);
    if (NULL == data_out || NULL == data_out->data || data_out->data_len <= 0) {
        return NULL;
    }
    memcpy(data_out->data, data_in->data, data_in->data_len);
    data_out->data_index = userdata_head->nEventID;
    return data_out;    
}
static ProtocolStr * GetUserData_TransferDirectly(ProtocolStr *data_in)
{
    if (NULL == data_in || NULL == data_in->data || data_in->data_len <= 0) {
        return NULL;
    }
    UserDataHeadInfo *userdata_head = (UserDataHeadInfo*)(data_in->data);
    // 待返回结构创建
    ProtocolStr * data_out = CreatProtocolStr(data_in->data_len);
    if (NULL == data_out || NULL == data_out->data || data_out->data_len <= 0) {
        return NULL;
    }
    memcpy(data_out->data, data_in->data, data_in->data_len);
    data_out->data_index = userdata_head->nEventID;
    return data_out;    
}

static ProtocolStr * GetUserData_DevConfigUpdateDone(ProtocolStr *data_in) 
{
    UserData_connect data_config;
    if (NULL == data_in || NULL == data_in->data || data_in->data_len <= 0) {
        return NULL;
    }
    if (data_in->data_len != sizeof(UserData_connect)) {
        return NULL;
    }
    memcpy(&data_config, data_in->data, data_in->data_len);
    #if NtoH_or_HtoN
    // 字节序转换
    SwitchNtoH_UserData_TcpConnect(&data_config.data_detail);
    #endif
    // 待返回结构创建
    ProtocolStr * data_out = CreatProtocolStr(data_in->data_len);
    if (NULL == data_out || NULL == data_out->data || data_out->data_len <= 0) {
//        LOG_ERROR("GetUserData_DevConfigUpdateDone() creat data_out err %u.", data_out->data_len);
        return NULL;
    }
    memcpy(data_out->data, &data_config, sizeof(data_config));
    data_out->data_index = data_config.data_head.nEventID;
    return data_out;
}

static ProtocolStr * GetUserData_CspStartDone(ProtocolStr *data_in) 
{
    int32_t data_l_mix = 0;
    char *ptr_i = NULL;
    ProtocolStr* data_out = NULL;
    int err_flag = 0;

    if (NULL == data_in || NULL == data_in->data || data_in->data_len <= 0) {
        return NULL;
    }
    // step 1. 接收数据长度校验，应该包含至少一个通道的配置参数
    data_l_mix = data_in->data_len-sizeof(UserDataHeadInfo)-sizeof(ChannelTableHeader)-sizeof(DeviceDetailInfo)-sizeof(DSAGlobalParam);
    UserDataHeadInfo* data_h = (UserDataHeadInfo *)data_in->data;
    if (data_l_mix<=0 || ((unsigned int)data_l_mix<(sizeof(SignalDataSource)+sizeof(ChannelTableHeader)))) {
        err_flag = DAQcfgError_UserDataLength;
    }
    // step 2. 用户数据头部字节序转换
    ptr_i = data_in->data;
    ptr_i += sizeof(UserDataHeadInfo);
    if (0 == err_flag) {
        // step 3. 通道配置头部字节序转换
        #if NtoH_or_HtoN
        SwitchNtoH_UserData_CspHead((ChannelTableHeader *)ptr_i);
        #endif
        data_out = CreatProtocolStr(data_in->data_len);
        if (NULL == data_out) {
            return NULL;
        }
        memcpy(data_out->data, data_in->data, data_in->data_len);
        data_out->data_index = data_in->data_index;
        return data_out;
    }
    data_out = CreatProtocolStr(sizeof(UserDataHeadInfo));
    if (NULL == data_out) {
        return NULL;
    } 
    memcpy(data_out->data, data_in->data, sizeof(UserDataHeadInfo));
    UserDataHeadInfo* head_value = (UserDataHeadInfo*)data_out->data;
    head_value->nParameters0 = err_flag;
    head_value->nParameters1 = data_h->nEventID;
    data_out->data_index = data_in->data_index;
    return data_out;  
}
static ProtocolStr * GetUserData_CspArmDevCfg(ProtocolStr *data_in) 
{
    if (NULL == data_in || NULL == data_in->data || data_in->data_len <= 0) {
        return NULL;
    }
    UserDataHeadInfo *userdata_head = (UserDataHeadInfo*)(data_in->data);
    // 待返回结构创建
    ProtocolStr * data_out = CreatProtocolStr(data_in->data_len);
    if (NULL == data_out || NULL == data_out->data || data_out->data_len <= 0) {
        return NULL;
    }
    memcpy(data_out->data, data_in->data, data_in->data_len);
    data_out->data_index = userdata_head->nEventID;
    return data_out;   
}
static ProtocolStr * GetUserData_Offline_SetConfig(ProtocolStr *data_in) 
{
    if (NULL == data_in || NULL == data_in->data || data_in->data_len <= 0) {
        return NULL;
    }
    UserDataHeadInfo *userdata_head = (UserDataHeadInfo*)(data_in->data);
    // 待返回结构创建
    ProtocolStr * data_out = CreatProtocolStr(data_in->data_len);
    if (NULL == data_out || NULL == data_out->data || data_out->data_len <= 0) {
        return NULL;
    }
    memcpy(data_out->data, data_in->data, data_in->data_len);
    data_out->data_index = userdata_head->nEventID;
    return data_out;   
}

static ProtocolStr * GetUserData_Inertial_SetConfig(ProtocolStr *data_in)
{
    UserDataHeadInfo data_head;
    if (NULL == data_in || NULL == data_in->data || data_in->data_len<=0) {
        return NULL;
    }
    memcpy(&data_head, data_in->data, sizeof(UserDataHeadInfo));
    ProtocolStr* data_reply = CreatProtocolStr(data_in->data_len);
    if (!data_reply) {
        return NULL;
    }
    data_reply->data_index = data_head.nEventID;
    memcpy(data_reply->data, data_in->data, data_in->data_len);
    return data_reply;      
}

ProtocolStr * GetUserData_File_Calibration_write(ProtocolStr *data_in)
{
    if (NULL == data_in || NULL == data_in->data) {
        return NULL;
    }

    ProtocolStr* data_reply = CreatProtocolStr(data_in->data_len);
    if (!data_reply) {
        return NULL;
    }
 
    memcpy(data_reply->data, data_in->data, data_in->data_len);
    data_reply->data_index = data_in->data_index;
    return data_reply;
}

 ProtocolStr * GetUserData_DVS_INIT_DSP2DSP(ProtocolStr *data_in)
 {
    if (NULL == data_in || NULL == data_in->data) {
        return NULL;
    }

    ProtocolStr* data_reply = CreatProtocolStr(data_in->data_len);
    if (!data_reply) {
        return NULL;
    }
 
    memcpy(data_reply->data, data_in->data, data_in->data_len);
    data_reply->data_index = data_in->data_index;
    return data_reply;
 }

ProtocolStr * GetUserData_Contrl_DspReadTeds(ProtocolStr *data_in)
{
    if (NULL == data_in || NULL == data_in->data) {
        return NULL;
    }

    ProtocolStr* data_reply = CreatProtocolStr(data_in->data_len);
    if (!data_reply) {
        return NULL;
    }
 
    memcpy(data_reply->data, data_in->data, data_in->data_len);
    data_reply->data_index = data_in->data_index;
    return data_reply;
}

// 用户数据解密
static int FrameDataDecode(FrameReply* data_frame)
{
    /* 内部函数 */
    if (!data_frame) {
        return RET_ERROR;
    }
    if (!data_frame->frame_attribute.frame_encrypt) {
        data_frame->frame_user_decode = data_frame->frame_user_encrypt;
        return RET_OK;
    } else {
        data_frame->frame_user_decode = CreatProtocolStr(data_frame->frame_user_encrypt->data_len);
        if (!data_frame->frame_user_decode) {
            return RET_ERROR;
        }
        int ret = DataDecode(data_frame->frame_attribute.frame_encrypt, data_frame->frame_user_encrypt, &data_frame->frame_user_decode);
        if (RET_OK != ret) {
            return RET_ERROR;
        }
    }
    return RET_OK;
}

// 用户数据CRC校验
static int FrameDataCrcCheck(FrameReply* data_frame)
{
    /* 内部函数 */
    if (!data_frame) {
        return RET_ERROR;
    }
    if (!data_frame->frame_attribute.check_enable)    // 校验标志位无效
    {
        return RET_OK;
    }
    else    // 校验标志有效
    {        
        uint16_t data_crc;
        int frame_len = 0;
        memcpy(&data_crc, (char *)data_frame->frame_user_decode->data + FREAM_HEAD_LENGTH + FREAM_LEN_LENGTH + frame_len, FREAM_CHECK_LENGTH);
        if (data_crc != DataCrc(data_frame->frame_user_decode))
        {            
            return RET_ERROR;
        }
    }
    return  RET_OK;
}

