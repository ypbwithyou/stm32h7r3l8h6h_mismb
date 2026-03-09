
#include "usb_protocol.h"
#include "usbd_cdc_if.h"
#include "./LIBS/lib_dwt/lib_dwt_timestamp.h"
#include "./MALLOC/malloc.h"

// 打包函数
// 参数：
//   user_data: 用户实际数据
//   user_data_len: 用户实际数据长度
//   head_info: 用户数据头信息
//   frame_head: 帧头信息
//   packet_len: 输出参数，打包后的数据总长度
// 返回值：打包后的数据缓冲区（需要调用者释放），失败返回NULL
uint8_t *pack_data(
    const uint8_t *user_data,
    uint32_t user_data_len,
    const UserDataHeadInfo *head_info,
    const FrameHeadInfo *frame_head,
    uint32_t *packet_len)
{
    if (!head_info || !frame_head)
    {
        return NULL;
    }

    uint8_t *packet = (uint8_t *)mymalloc(SRAMEX, 2 * 1024 * 1024);

    // 计算用户数据部分总长度（用户数据头 + 实际用户数据）
    uint32_t user_part_len = sizeof(UserDataHeadInfo) + user_data_len;

    // 确保用户数据部分是4字节的倍数
    uint32_t padding = (4 - (user_part_len % 4)) % 4;
    uint32_t aligned_user_len = user_part_len + padding;

    // 计算整个数据包长度
    uint32_t total_len = sizeof(FrameHeadInfo) + // 帧头
                         sizeof(uint32_t) +      // 帧长度字段
                         aligned_user_len +      // 用户数据部分（对齐后）
                         sizeof(uint32_t) +      // CRC32校验
                         sizeof(uint32_t);       // 帧尾
  
    uint8_t *ptr = packet;

    // 1. 复制帧头
    memcpy(ptr, frame_head, sizeof(FrameHeadInfo));
    ptr += sizeof(FrameHeadInfo);

    // 2. 写入帧长度（用户数据部分长度）
    uint32_t frame_len = aligned_user_len;
    memcpy(ptr, &frame_len, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    // 3. 复制用户数据头
    memcpy(ptr, head_info, sizeof(UserDataHeadInfo));
    ptr += sizeof(UserDataHeadInfo);

    // 4. 复制用户实际数据
    if (user_data_len > 0)
    {
        memcpy(ptr, user_data, user_data_len);
        ptr += user_data_len;
    }

    // 5. 填充字节（如果需要）
    if (padding > 0)
    {
        memset(ptr, 0, padding);
        ptr += padding;
    }

    // 6. 计算CRC32（从帧头开始到用户数据结束）
    //    uint32_t crc_len = sizeof(FrameHeadInfo) + sizeof(uint32_t) + aligned_user_len;
    //    uint32_t crc = calculate_crc32(packet, crc_len);
    //    memcpy(ptr, &crc, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    // 7. 写入帧尾
    uint32_t frame_tail = FRAME_END;
    memcpy(ptr, &frame_tail, sizeof(uint32_t));

    *packet_len = total_len;

    // 8. 发送
    usbd_cdc_transmit(packet, total_len);

    myfree(SRAMEX, packet);

    return NULL;
}

// 解包函数
// 参数：
//   packet: 接收到的数据包
//   packet_len: 数据包长度
//   out_user_data: 输出参数，指向解包后的用户数据（如果存在）
//   out_head_info: 输出参数，用户数据头信息
//   out_frame_head: 输出参数，帧头信息
//   out_user_data_len: 输出参数，用户数据实际长度
// 返回值：0-成功，其他-错误码
int unpack_data(
    const uint8_t *packet,
    uint32_t packet_len,
    uint8_t **out_user_data,
    UserDataHeadInfo *out_head_info,
    FrameHeadInfo *out_frame_head,
    uint32_t *out_user_data_len)
{
    if (!packet || !out_user_data || !out_head_info || !out_frame_head || !out_user_data_len)
    {
        return -1;
    }

    // 检查最小长度
    uint32_t min_len = sizeof(FrameHeadInfo) + sizeof(uint32_t) +
                       sizeof(UserDataHeadInfo) + sizeof(uint32_t) + sizeof(uint32_t);
    if (packet_len < min_len)
    {
        return -2; // 数据包太短
    }

    const uint8_t *ptr = packet;

    // 1. 解析帧头
    memcpy(out_frame_head, ptr, sizeof(FrameHeadInfo));
    ptr += sizeof(FrameHeadInfo);

    // 检查帧头标识
    if (out_frame_head->hd_head != 0x3c3c3c3c)
    {
        return -3; // 帧头标识错误
    }

    // 2. 解析帧长度
    uint32_t frame_len;
    memcpy(&frame_len, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    // 3. 检查帧长度是否合理
    if (frame_len < sizeof(UserDataHeadInfo) ||
        packet_len < (sizeof(FrameHeadInfo) + sizeof(uint32_t) + frame_len + 2 * sizeof(uint32_t)))
    {
        return -4; // 帧长度错误
    }

    // 4. 解析用户数据头
    memcpy(out_head_info, ptr, sizeof(UserDataHeadInfo));
    ptr += sizeof(UserDataHeadInfo);

    // 检查用户数据头标志
    if (out_head_info->nIsValidFlag != 0x12345678)
    {
        return -5; // 用户数据头无效
    }

    // 5. 计算用户实际数据长度
    uint32_t actual_data_len = frame_len - sizeof(UserDataHeadInfo);
    *out_user_data_len = 0;
    *out_user_data = NULL;

    // 6. 如果有用户数据，复制出来
    if (out_head_info->nSourceType == 1 && out_head_info->nDataLength > 0)
    {
        if (out_head_info->nDataLength <= actual_data_len)
        {
            *out_user_data_len = out_head_info->nDataLength;
            //            *out_user_data = (uint8_t*)malloc(out_head_info->nDataLength);
            //            if (*out_user_data) {
            //                memcpy(*out_user_data, ptr, out_head_info->nDataLength);
            //            }
            *out_user_data = (uint8_t *)ptr;
        }
        else
        {
            return -6; // 数据长度不一致
        }
    }

    // 跳过用户数据部分
    ptr += actual_data_len;

    // 7. 验证CRC32
    uint32_t received_crc;
    memcpy(&received_crc, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    // 计算CRC32（从帧头开始到用户数据结束）
    //    uint32_t crc_len = sizeof(FrameHeadInfo) + sizeof(uint32_t) + frame_len;
    //    uint32_t calculated_crc = calculate_crc32(packet, crc_len);
    //
    //    if (received_crc != calculated_crc) {
    //        if (*out_user_data) {
    //            free(*out_user_data);
    //            *out_user_data = NULL;
    //        }
    //        return -7;  // CRC校验失败
    //    }

    // 8. 验证帧尾
    uint32_t frame_tail;
    memcpy(&frame_tail, ptr, sizeof(uint32_t));

    if (frame_tail != 0x3E3E3E3E)
    {
        //        if (*out_user_data) {
        //            free(*out_user_data);
        //            *out_user_data = NULL;
        //        }
        return -8; // 帧尾错误
    }

    return 0; // 成功
}

// 释放解包时分配的用户数据内存
void free_unpacked_user_data(uint8_t *user_data)
{
    if (user_data)
    {
        free(user_data);
    }
}

// 创建默认帧头（示例函数）
FrameHeadInfo create_default_frame_head(uint16_t serial_num)
{
    FrameHeadInfo head = {0};
    head.hd_head = 0x3c3c3c3c;
    head.hd_attribute_ver = 0x10;  // 版本1.0
    head.hd_attribute_flag = 0x01; // 默认标志
    head.hd_data_status = 0x00;    // 默认数据状态
    head.hd_serial_num = serial_num;
    return head;
}

// 创建用户数据头（示例函数）
UserDataHeadInfo create_user_data_head(
    uint32_t event_id,
    uint32_t source_type,
    uint32_t dest_id,
    uint32_t data_len)
{
    UserDataHeadInfo head = {0};
    head.nIsValidFlag = 0x12345678;
    head.nEventID = event_id;
    head.nSourceType = source_type;
    head.nDestinationID = dest_id;
    head.nDataLength = data_len;
    head.nNanoSecond = dwt_get_ns();
    return head;
}

// 使用示例
int main_example()
{
    // 1. 准备数据
    const char *test_data = "Hello, this is a test message!";
    uint32_t data_len = strlen(test_data) + 1; // 包括字符串结束符

    FrameHeadInfo frame_head = create_default_frame_head(1);
    UserDataHeadInfo user_head = create_user_data_head(1001, 1, 0, data_len);

    // 2. 打包
    uint32_t packet_len = 0;
    uint8_t *packet = pack_data((uint8_t *)test_data, data_len, &user_head, &frame_head, &packet_len);

    if (!packet)
    {
        printf("Pack failed!\n");
        return -1;
    }

    printf("Pack success! Packet length: %u\n", packet_len);

    // 3. 解包
    uint8_t *unpacked_data = NULL;
    UserDataHeadInfo unpacked_head;
    FrameHeadInfo unpacked_frame_head;
    uint32_t unpacked_data_len = 0;

    int result = unpack_data(packet, packet_len, &unpacked_data,
                             &unpacked_head, &unpacked_frame_head,
                             &unpacked_data_len);

    if (result == 0)
    {
        printf("Unpack success!\n");
        printf("Event ID: %u\n", unpacked_head.nEventID);
        printf("Source type: %u\n", unpacked_head.nSourceType);

        if (unpacked_data)
        {
            printf("User data: %s\n", unpacked_data);
            free_unpacked_user_data(unpacked_data);
        }
    }
    else
    {
        printf("Unpack failed with error code: %d\n", result);
    }

    // 4. 清理
    free(packet);

    return 0;
}