/****************************************************************************** 
  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to 
    any third party.
  ------------------------------------------------------------------------
  ... ...
*******************************************************************************/
// #include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/syscall.h>
#include <sys/stat.h>
#include <ftw.h>

#include "sys_cmd.h"
#include "file_opt.h"
#include "time_extend.h"

#define CMD_MAX_LEN             (1024)
// #define BYTE_TO_MB              (1000 * 1000)
#define BYTE_TO_KB              (1000)
#define UNUSED(x)  (void)(x)

static int g_dir_size = 0;

// So 构造函数
static void SoConstruct(void) __attribute__((constructor));
// So 析构函数
static void SoDestruct(void) __attribute__((destructor));

// So 构造函数
static void SoConstruct(void)
{
    // pthread_mutex_init(&g_log_mutex, NULL);
}

// So 析构函数
static void SoDestruct(void)
{
    // pthread_mutex_destroy(&g_log_mutex); 
}

// 文件压缩并删除, dir 文件所在目录, bytes 文件大小，单位字节； async:1异步, 0同步
void FileZipOver(const char* dir, unsigned int bytes, int async)
{
    char cmd[CMD_MAX_LEN] = "";
    // memset(cmd, 0x00, sizeof(cmd));
    // snprintf(cmd, sizeof(cmd), "cd %s; find -type f -size +%uc | sort -nr | head -1 | xargs -I {} tar -czf {}.tar.gz {}", \
    //          dir, bytes); // 20231024
    // if (async)
    // {
    //     strcat(cmd, " &");
    // }
    // pox_system(cmd);
    // sleep(1);
    memset(cmd, 0x00, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "cd %s; find -type f -size +%uc | sort -nr | head -1 | xargs -I {} rm -rf {}", dir, bytes); // 20231024
    if (async)
    {
        strcat(cmd, " &");
    }
    pox_system(cmd);

}

// 文件压缩, dir 文件所在目录, pattern_str 文件名称, 支持shell通配符 async:1异步, 0同步
void FileZip(const char* dir, const char* pattern_str, int async)
{
    char time_str[100];
    GetTimeStrNow("%Y%m%d%H%M%S", time_str, sizeof(time_str), 0);
    char cmd[CMD_MAX_LEN] = "";
    snprintf(cmd, sizeof(cmd), "cd %s; ls %s 2>/dev/null | xargs -I {} mv {} %s_{}", dir, pattern_str, time_str);
    pox_system(cmd);
    memset(cmd, 0x00, sizeof(cmd));
    snprintf(cmd, sizeof(cmd), "cd %s; ls %s_%s 2>/dev/null | grep -v 'gz' | xargs -I {} tar -czf {}.tar.gz {} >/dev/null 2>&1 ; rm -rf %s_%s", \
             dir, time_str, pattern_str, time_str, pattern_str); // 20201217
    if (async)
    {
        strcat(cmd, " &");
    }
    pox_system(cmd);
}

// 创建目录
void MkdirDir(const char* dir)
{
    char cmd[1024] = "";
    snprintf(cmd, sizeof(cmd), "mkdir -p %s", dir);
    pox_system(cmd);
}

// 文件大小累加函数, 用于ftw函数回调
static int FileSum(const char *fpath, const struct stat *sbuff, int typeflag)
{
    UNUSED(fpath);
    UNUSED(typeflag);
    g_dir_size += sbuff->st_size;
    // if (0 == typeflag && strlen(fpath) >=3 && 
    //     '.' == fpath[strlen(fpath) - 3] && 
    //     'g' == fpath[strlen(fpath) - 2] && 
    //     'z' == fpath[strlen(fpath) - 1])
    // {
    //     ++g_dir_tar_num;
    // }
    return 0; 
}

// 获取指定目录下文件总大小
int GetDirSize(const char* dir)
{
    g_dir_size = 0;
    // g_dir_tar_num = 0;
    ftw(dir, &FileSum, 1);
    // g_dir_size /= BYTE_TO_KB;
    return g_dir_size;
}

// 获取指定文件大小
int GetFileSize(const char* dir, const char* file)
{
    FILE *fp;
    int flen;
    char file_name[CMD_MAX_LEN] = "";
    snprintf(file_name, sizeof(file_name), "%s/%s", dir, file);
    fp = fopen(file_name, "r");
    if (!fp)
    {
        return 0;
    }
    flen = ftell(fp);
    fclose(fp);
    return flen;
}

// 删除指定目录下时间最早的gz文件
void DelOldestGzFile(const char* dir)
{
    char cmd[CMD_MAX_LEN] = "";
    snprintf(cmd, CMD_MAX_LEN, "ls -a %s/*.gz 2>/dev/null | sort -nk 1 -t_ | sed -n 1p | xargs -n1 rm -rf", dir);
    pox_system(cmd);
}

// 删除指定目录下时间最早的log文件
void DelOldestLogFile(const char* dir)
{
    char cmd[CMD_MAX_LEN] = "";
    snprintf(cmd, CMD_MAX_LEN, "ls -a %s/*.log 2>/dev/null | sort -nk 1 -t_ | sed -n 1p | xargs -n1 rm -rf", dir);
    pox_system(cmd);
}

// 删除指定目录下文件名中包含pattern_str的且创建时间最长文件
void DelOldestTypeFile(const char* dir, const char* pattern_str)
{
    if (!dir || !pattern_str)
    {
        return;
    }
    char cmd[CMD_MAX_LEN] = "";
    // ZipRedisLog(0);
    // snprintf(cmd, CMD_MAX_LEN, "cd %s; ls -t | sort -nk 4 -t_ | sed -n 1p | xargs -n1 rm -rf", dir);
    // snprintf(cmd, CMD_MAX_LEN, "find %s/* | grep %s | sort -nk 4 -t_ | sed -n 1p | xargs -n1 rm -rf", dir, pattern_str);
    snprintf(cmd, CMD_MAX_LEN, "cd %s; ls | grep %s | sort -nk 4 -t_ | sed -n 1p | xargs -n1 rm -rf", dir, pattern_str);
    // snprintf(cmd, CMD_MAX_LEN, "ls -a %s/%s 2>/dev/null | sort -nk 1 -t_ | sed -n 1p | xargs -n1 rm -rf", dir, pattern_str);
    // printf("%s\n", cmd);
    pox_system(cmd);
}

// 删除指定目录下文件名中不包含pattern_str字符串的文件及文件夹
void DelUnlawTypeFile(const char* dir, const char* pattern_str)
{
    if (!dir || !pattern_str)
    {
        return;
    }
    char cmd[CMD_MAX_LEN] = "";
    // snprintf(cmd, CMD_MAX_LEN, "cd %s; ls | grep -v %s | xargs rm -rf", dir, pattern_str);
    // snprintf(cmd, CMD_MAX_LEN, "find %s/* | grep -v %s | xargs rm -rf", dir, pattern_str);
    snprintf(cmd, CMD_MAX_LEN, "cd %s; ls | grep -v %s | xargs rm -rf", dir, pattern_str);
    // printf("***%s\n", cmd);
    pox_system(cmd);
}
/** mode
 * R_OK, 4 只判断是否有读权限，对应宏定义里面的00 只存在
 * W_OK, 2 只判断是否有写权限，对应宏定义里面的02 写权限
 * X_OK, 1 判断是否有执行权限，对应宏定义里面的04 读权限
 * F_OK, 0 只判断是否存在，对应宏定义里面的05     读和写权限
 */
int file_exist(const char *path, int mode)
{
    return (access(path, mode) == 0);
}
long fsize(FILE *fp)
{
    long n;
    fpos_t fpos; //当前位置
    fgetpos(fp, &fpos); //获取当前位置
    fseek(fp, 0, SEEK_END);
    n = ftell(fp);
    fsetpos(fp,&fpos); //恢复之前的位置
    return n;
}
