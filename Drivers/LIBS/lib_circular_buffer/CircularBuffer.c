#include "CircularBuffer.h"
#include <string.h>
#include <stdlib.h>
#include "./MALLOC/malloc.h"

// 初始化缓冲区
CircularBuffer* cb_init(int capacity) {
//    CircularBuffer *cb = (CircularBuffer*)malloc(sizeof(CircularBuffer));
//    if (!cb) return NULL;
//    
//    cb->buffer = (char*)malloc(capacity);
//    if (!cb->buffer) {
//        free(cb);
//        return NULL;
//    }
    
    CircularBuffer *cb = (CircularBuffer*)mymalloc(SRAMIN, sizeof(CircularBuffer));
    if (!cb) return NULL;
    
    cb->buffer = (char*)mymalloc(SRAMIN, capacity);
    if (!cb->buffer) {
        free(cb);
        return NULL;
    }
    
    cb->capacity = capacity;
    cb->size = 0;
    cb->read_pos = 0;
    cb->write_pos = 0;
    
    // 裸机环境下不需要互斥锁，直接返回
    return cb;
}

// 释放缓冲区
void cb_free(CircularBuffer *cb) {
    if (cb) {
        free(cb->buffer);
        free(cb);
    }
}

// 判断缓冲区是否为空
bool cb_is_empty(CircularBuffer *cb) {
    if (!cb) return true;
    return (cb->size == 0);
}

// 判断缓冲区是否已满
bool cb_is_full(CircularBuffer *cb) {
    if (!cb) return false;
    return (cb->size == cb->capacity);
}

// 获取缓冲区数据量
int cb_size(CircularBuffer *cb) {
    if (!cb) return 0;
    return cb->size;
}

// 获取缓冲区可用空间
int cb_available(CircularBuffer *cb) {
    if (!cb) return 0;
    return cb->capacity - cb->size;
}

// 清空缓冲区
void cb_clear(CircularBuffer *cb) {
    if (cb) {
        cb->size = 0;
        cb->read_pos = 0;
        cb->write_pos = 0;
    }
}

// 向缓冲区写入数据（支持覆盖旧数据）
int cb_write(CircularBuffer *cb, const char *data, int len) {
    if (!cb || !data || len <= 0) return 0;
    
    // 如果数据长度超过缓冲区容量，只保留最后capacity字节
    if (len > cb->capacity) {
        data = data + (len - cb->capacity);
        len = cb->capacity;
    }
    
    // 如果需要覆盖旧数据
    if (len > (cb->capacity - cb->size)) {
        // 计算需要丢弃的旧数据长度
        int overflow = len - (cb->capacity - cb->size);
        
        // 移动读指针，丢弃最旧的overflow字节
        cb->read_pos = (cb->read_pos + overflow) % cb->capacity;
        cb->size -= overflow;
        if (cb->size < 0) cb->size = 0;
    }
    
    // 写入数据
    int write_len = len;
    
    // 分两段写入（如果需要）
    int first_part = cb->capacity - cb->write_pos;
    if (write_len <= first_part) {
        // 不需要分段
        memcpy(cb->buffer + cb->write_pos, data, write_len);
        cb->write_pos = (cb->write_pos + write_len) % cb->capacity;
    } else {
        // 需要分段写入
        memcpy(cb->buffer + cb->write_pos, data, first_part);
        memcpy(cb->buffer, data + first_part, write_len - first_part);
        cb->write_pos = write_len - first_part;
    }
    
    cb->size += write_len;
    if (cb->size > cb->capacity) cb->size = cb->capacity;
    
    return write_len;
}

// 从缓冲区读取数据
int cb_read(CircularBuffer *cb, char *data, int len) {
    if (!cb || !data || len <= 0) return 0;
    
    if (cb->size == 0) {
        return 0;
    }
    
    // 确定实际读取长度
    int read_len = (len > cb->size) ? cb->size : len;
    
    // 分两段读取（如果需要）
    int first_part = cb->capacity - cb->read_pos;
    if (read_len <= first_part) {
        // 不需要分段
        memcpy(data, cb->buffer + cb->read_pos, read_len);
        cb->read_pos = (cb->read_pos + read_len) % cb->capacity;
    } else {
        // 需要分段读取
        memcpy(data, cb->buffer + cb->read_pos, first_part);
        memcpy(data + first_part, cb->buffer, read_len - first_part);
        cb->read_pos = read_len - first_part;
    }
    
    cb->size -= read_len;
    
    return read_len;
}

// 查看缓冲区数据（不移动读指针）
int cb_peek(CircularBuffer *cb, char *data, int len, int offset) {
    if (!cb || !data || len <= 0) return 0;
    
    if (offset >= cb->size) {
        return 0;
    }
    
    // 确定实际查看长度
    int peek_len = (len > (cb->size - offset)) ? (cb->size - offset) : len;
    
    // 计算起始位置
    int start_pos = (cb->read_pos + offset) % cb->capacity;
    
    // 分两段复制（如果需要）
    int first_part = cb->capacity - start_pos;
    if (peek_len <= first_part) {
        memcpy(data, cb->buffer + start_pos, peek_len);
    } else {
        memcpy(data, cb->buffer + start_pos, first_part);
        memcpy(data + first_part, cb->buffer, peek_len - first_part);
    }
    
    return peek_len;
}

// 获取缓冲区首字节（不移动读指针）
int cb_front(CircularBuffer *cb) {
    if (!cb) return -1;
    
    int front = -1;
    if (cb->size > 0) {
        front = (unsigned char)cb->buffer[cb->read_pos];
    }
    return front;
}