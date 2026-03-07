/******************************************************************************
 
  
  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to any 
    third party.
  ------------------------------------------------------------------------
  文件名称: file_opt.h
  文件标识: 
  摘    要: 
  当前版本: 1.0
  作    者: Jack.lei
  完成日期: 2019年3月26日
*******************************************************************************/
#ifndef LIB_FILE_OPT_INCLUDE_FILE_OPT_H_
#define LIB_FILE_OPT_INCLUDE_FILE_OPT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

// 文件压缩, dir 文件所在目录, pattern_str 文件名称, 支持shell通配符 async:1异步, 0同步
void FileZip(const char* dir, const char* pattern_str, int async);

void FileZipOver(const char* dir, unsigned int bytes, int async);

// 创建目录
void MkdirDir(const char* dir);

// 获取指定目录下文件总大小
int GetDirSize(const char* dir);

// 获取指定文件大小
int GetFileSize(const char* dir, const char* file);

// 删除指定目录下时间最早的gz文件
void DelOldestGzFile(const char* dir);

// 删除指定目录下时间最早的log文件
void DelOldestLogFile(const char* dir);

// 删除指定目录下时间最早的某类型文件
void DelOldestTypeFile(const char* dir, const char* pattern_str);

// 删除指定目录下文件名中不包含某字段的所有文件
void DelUnlawTypeFile(const char* dir, const char* pattern_str);

int file_exist(const char *path, int mode);
long fsize(FILE *fp);

#ifdef __cplusplus
}
#endif

#endif // LIB_FILE_OPT_INCLUDE_FILE_OPT_H_
