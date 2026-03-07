#ifndef __LIB_CIRCULAR_BUFFER_H
#define __LIB_CIRCULAR_BUFFER_H

#include <string.h>
#include <stdbool.h>

/*FreeRTOS*********************************************************************************************/

// 循环缓冲区结构体
typedef struct {
    char *buffer;           // 缓冲区指针
    int capacity;           // 缓冲区总容量
    int size;              // 当前数据量
    int read_pos;          // 读位置
    int write_pos;         // 写位置
} CircularBuffer;

/******************************************************************************************************/
// 初始化缓冲区
CircularBuffer* cb_init(int capacity);
// 释放缓冲区
void cb_free(CircularBuffer *cb);
// 判断缓冲区是否为空（线程安全）
bool cb_is_empty(CircularBuffer *cb);
// 判断缓冲区是否已满（线程安全）
bool cb_is_full(CircularBuffer *cb);
// 获取缓冲区数据量（线程安全）
int cb_size(CircularBuffer *cb);
// 获取缓冲区可用空间（线程安全）
int cb_available(CircularBuffer *cb);
// 清空缓冲区（线程安全）
void cb_clear(CircularBuffer *cb);
// 向缓冲区写入数据（线程安全，支持覆盖旧数据）
int cb_write(CircularBuffer *cb, const char *data, int len);
// 从缓冲区读取数据（线程安全，支持指定读取字节数）
int cb_read(CircularBuffer *cb, char *data, int len);
// 查看缓冲区数据（不移动读指针，线程安全）
int cb_peek(CircularBuffer *cb, char *data, int len, int offset);
// 获取缓冲区首字节（不移动读指针，线程安全）
int cb_front(CircularBuffer *cb);

#endif // __LIB_CIRCULAR_BUFFER_H