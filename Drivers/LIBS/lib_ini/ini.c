/******************************************************************************
  Copyright (c) 2018-2021  (Xi'an) Testing & Control Technologies Co., Ltd.
  All rights reserved.
  
  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to 
    any third party.
  ------------------------------------------------------------------------
  ... ...
*******************************************************************************/
#include "ini.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define DEFAULT_INI_DIR             "../ini/"

typedef struct KeyValue
{
    char*               key;
    char*               value;
    struct KeyValue*    kv_next;
}KeyValue;

typedef struct Sector
{
    struct KeyValue*    kv_head;
    char*               sec_name;
    struct Sector*      sec_next;
}Sector;

typedef struct Conf
{
    struct Sector*      sec_head;
    char*               file_name;
}Conf;

static Conf g_conf = {NULL, NULL};

static void Construct(void) __attribute__((constructor));
static void Destruct(void) __attribute__((destructor));

static int IsSpace(char ch);
static char* StrTrimLeft(char* str);
static char* StrTrimRight(char* str);
static void FreeKeyValue(KeyValue* kv);
static void FreeSector(Sector* sec);
static void FreeAll(void);
static KeyValue* CreatKeyValue(const char* key, const char* value);
static int AddKeyValue(Sector* sec, const char* key, const char* value);
static Sector* CreateSec(const char* sector);
static int AddSecKeyValue(const char* sector, const char* key, char* value);
static int SaveConfFile(void);

static int IsSpace(char ch)
{
    if (' ' == ch || '\t' == ch || '\n' == ch || '\r' == ch)
    {
        return 1;
    }
    return 0;
}

static char* StrTrimLeft(char* str)
{
    if (!str)
    {
        return NULL;
    }
    size_t i;
    for (i = 0; i < strlen(str); ++i)
    {
        if (!IsSpace(str[i]))
        {
            break;
        }
    }
    return (str + i);
}

static char* StrTrimRight(char* str)
{
    if (!str)
    {
        return NULL;
    }
    for (int i = strlen(str) - 1; i >= 0; --i)
    {
        if (!IsSpace(str[i]))
        {
            break;
        }
        str[i] = '\0';
    }
    return str;
}

char* StrTrim(char* str)
{
    if (!str)
    {
        return NULL;
    }
    return StrTrimRight(StrTrimLeft(str));
}

static void FreeKeyValue(KeyValue* kv)
{
    if (!kv)
    {
        return;
    }
    if (NULL != kv->key)
    {
        free(kv->key);
        kv->key = NULL;
    }
    if (NULL != kv->value)
    {
        free(kv->value);
        kv->value = NULL;
    }
    free(kv);
}

static void FreeSector(Sector* sec)
{
    if (!sec)
    {
        return;
    }
    if (NULL != sec->sec_name)
    {
        free(sec->sec_name);
        sec->sec_name = NULL;
    }
    KeyValue* kv = sec->kv_head;
    while (NULL != kv)
    {
        KeyValue* tmp = kv->kv_next;
        FreeKeyValue(kv);
        kv = tmp;
    }
    free(sec);
}

static void FreeAll(void)
{
    if (NULL != g_conf.file_name)
    {
        free(g_conf.file_name);
        g_conf.file_name = NULL;
    }
    Sector* sec = g_conf.sec_head;
    while (NULL != sec)
    {
        Sector* tmp = sec->sec_next;
        FreeSector(sec);
        sec = tmp;
    }
    g_conf.sec_head = NULL;
}

static KeyValue* CreatKeyValue(const char* key, const char* value)
{
    if (!key || !value)
    {
        return NULL;
    }
    KeyValue* kv = malloc(sizeof(KeyValue));
    if (NULL == kv)
    {
        return NULL;
    }
    kv->kv_next = NULL;
    kv->key = malloc(strlen(key) + 1);
    if (NULL == kv->key)
    {
        free(kv);
        return NULL;
    }
    kv->value = malloc(strlen(value) + 1);
    if (NULL == kv->value)
    {
        free(kv->key);
        free(kv);
        return NULL;
    }
    memset(kv->key, 0x00, strlen(key) + 1);
    memset(kv->value, 0x00, strlen(value) + 1);
    strcpy(kv->key, key);
    strcpy(kv->value, value);
    return kv;
}

static int AddKeyValue(Sector* sec, const char* key, const char* value)
{
    if (!sec || !key || !value)
    {
        return INI_RET_ERROR;
    }
    if (NULL == sec->kv_head)
    {
        sec->kv_head = CreatKeyValue(key, value);
        if (NULL == sec->kv_head)
        {
//            LOG_ERROR("CreatKeyValue err. %s: %s", key, value);
            return INI_RET_ERROR;
        }
        else
        {
            return INI_RET_OK;
        }
    }
    KeyValue* kv = sec->kv_head;
    while (NULL != kv)
    {
        if (NULL != kv->key && strcmp(kv->key, key) == 0)
        {
            if (NULL != kv->value)
            {
                free(kv->value);
            }
            kv->value = malloc(strlen(value) + 1);
            if (NULL == kv->value)
            {
//                LOG_ERROR("malloc value %d err. %s: %s", strlen(value) + 1, key, value);
                return INI_RET_ERROR;
            }
            strcpy(kv->value, value);
            return INI_RET_OK;
        }
        if (NULL != kv->kv_next)
        {
            kv = (KeyValue*)kv->kv_next;
        }
        else 
        {
            break;
        }
    }
    kv->kv_next = CreatKeyValue(key, value);
    if (NULL == kv->kv_next)
    {
//        LOG_ERROR("CreatKeyValue err. %s: %s", key, value);
        return INI_RET_ERROR;
    }
    return INI_RET_OK;
}

static Sector* CreateSec(const char* sector)
{
    if (!sector)
    {
        return NULL;
    }
    Sector* sec = malloc(sizeof(Sector));
    if (NULL == sec)
    {
        return NULL;
    }
    sec->sec_next = NULL;
    sec->sec_name = malloc(strlen(sector) + 1);
    if (NULL == sec->sec_name)
    {
        free(sec);
        return NULL;
    }
    memset(sec->sec_name, 0x00, strlen(sector) + 1);
    strcpy(sec->sec_name, sector);
    sec->kv_head = NULL;
    return sec;
}

static int AddSecKeyValue(const char* sector, const char* key, char* value)
{
    if (!sector || !key || !value)
    {
        return INI_RET_ERROR;
    }
    if (NULL == g_conf.sec_head)
    {
        g_conf.sec_head = CreateSec(sector);
        if (NULL == g_conf.sec_head)
        {
            return INI_RET_ERROR;
        }
        return AddKeyValue(g_conf.sec_head, key, value);
    }
    Sector* sec = g_conf.sec_head;
    while(NULL != sec)
    {
        if (NULL != sec->sec_name && strcmp(sec->sec_name, sector) == 0)
        {
            return AddKeyValue(sec, key, value);
        }
        if (NULL != sec->sec_next)
        {
            sec = (Sector*)sec->sec_next;
        }
        else
        {
            break;
        }
    }
    sec->sec_next = CreateSec(sector);
    if (NULL == sec->sec_next)
    {
        return INI_RET_ERROR;
    }
    return AddKeyValue(sec->sec_next, key, value);
}

int LoadConfFile(const char* file)
{
    if (!file)
    {
        return INI_RET_ERROR;
    }
    FreeAll();
    FILE *fp;

    g_conf.file_name = malloc(strlen(file) + 1);
    if (NULL == g_conf.file_name)
    {
//        LOG_ERROR("malloc file_name %d error.", strlen(file) + 1);
        return INI_RET_ERROR;
    }
    strcpy(g_conf.file_name, file);

    fp = fopen(file, "r");
    if (NULL == fp)
    {
//        LOG_ERROR("fopen ini file error. file: %s", file);
        return INI_RET_ERROR;
    }

    char line_buff[1024];
    char sector_name[1024] = "";
    while (NULL != fgets(line_buff, sizeof(line_buff), fp))
    {   
        char* p = StrTrim(line_buff);
        if (strlen(p) <= 0 || '#' == p[0])
        {
            continue;
        }
        else if ('[' == p[0] && ']' == p[strlen(p) - 1])
        {
            memset(sector_name, 0x00, sizeof(sector_name));
            strncpy(sector_name, p+1, strlen(p) - 2);
            continue;
        }
        else
        {
            if (strlen(sector_name) == 0)
            {
                continue;
            }
            char* p2 = strchr(p, '=');
            if (NULL == p2)
            {
                continue;
            }
            *p2++ = '\0';
            p = StrTrimRight(p);
            p2 = StrTrimLeft(p2);
            if (INI_RET_ERROR == AddSecKeyValue(sector_name, p, p2))
            {
                fclose(fp);
                return INI_RET_ERROR;
            }
        }
    }
    fclose(fp);
    return INI_RET_OK;
}

static int SaveConfFile(void)
{
    if (NULL == g_conf.file_name)
    {
        return INI_RET_ERROR;
    }
    FILE *fp;
    fp = fopen(g_conf.file_name, "w");
    if (NULL == fp)
    {
        return INI_RET_ERROR;
    }
    Sector* sec = g_conf.sec_head;
    while (NULL != sec)
    {
        if (NULL != sec->sec_name)
        {
            fprintf(fp, "[%s]\n", sec->sec_name);
            KeyValue* kv = sec->kv_head;
            while (NULL != kv)
            {
                if (NULL != kv->key && NULL != kv->value)
                {
                    fprintf(fp, "%s = %s\n", kv->key, kv->value);
                }
                kv = kv->kv_next;
            }
            fprintf(fp, "\n");
        }
        sec = sec->sec_next;
    }
    fclose(fp);
//    system("sync");
    return INI_RET_OK;
}

int GetSecStr(const char* sector, const char* key, char* value)
{
    if (!sector || !key || !value)
    {
        return INI_RET_ERROR;
    }
    Sector* sec = g_conf.sec_head;
    while (NULL != sec)
    {
        if (NULL != sec->sec_name && strcmp(sec->sec_name, sector) == 0)
        {
            KeyValue* kv = sec->kv_head;
            while (NULL != kv)
            {
                if (NULL != kv->key && strcmp(kv->key, key) == 0)
                {
                    if (NULL != kv->value)
                    {
                        strcpy(value, kv->value);
                        return INI_RET_OK;
                    }
                    else
                    {
                        return INI_RET_ERROR;
                    }
                }
                kv = kv->kv_next;
            }
            return INI_RET_ERROR;
        }
        sec = sec->sec_next;
    }
    return INI_RET_ERROR;
}

int SetSecStr(const char* sector, const char* key, char* value)
{
    if (!sector || !key || !value)
    {
        return INI_RET_ERROR;
    }
    if (INI_RET_ERROR == AddSecKeyValue(sector, key, value))
    {
//        LOG_ERROR("AddSecKeyValue err. %s %s: %s", sector, key, value);
        return INI_RET_ERROR;
    }
    return SaveConfFile();
}

static int CheckIntStr(const char* str)
{
    if (!str)
    {
        return INI_RET_ERROR;
    }
    if (strlen(str) == 0)
    {
        return INI_RET_ERROR;
    }
    for (size_t i = 0; i < strlen(str); ++i)
    {
        if (str[i] < '0' || str[i] > '9')
        {
            if (str[i] != '-')
            {
                return INI_RET_ERROR;
            }
        }
    }
    return INI_RET_OK;
}

static int CheckFloatStr(const char* str)
{
    if (!str)
    {
        return INI_RET_ERROR;
    }
    if (strlen(str) == 0)
    {
        return INI_RET_ERROR;
    }
    for (size_t i = 0; i < strlen(str); ++i)
    {
        if (str[i] < '0' || str[i] > '9')
        {
            if (str[i] != '.' && str[i] != '-')
            {
                return INI_RET_ERROR;
            }
        }
    }
    return INI_RET_OK;
}

int GetSecInt(const char* sector, const char* key, int* value)
{
    if (!sector || !key || !value)
    {
        return INI_RET_ERROR;
    }
    char tmp[100] = "";
    if (INI_RET_OK != GetSecStr(sector, key, tmp))
    {
        return INI_RET_ERROR;
    }
    if (INI_RET_OK != CheckIntStr(tmp))
    {
        return INI_RET_ERROR;
    }
    *value = atoi(tmp);
    return INI_RET_OK;
}

int SetSecInt(const char* sector, const char* key, int* value)
{
    if (!sector || !key || !value)
    {
        return INI_RET_ERROR;
    }
    char tmp[100] = "";
    snprintf(tmp, sizeof(tmp), "%d", *value);
    return SetSecStr(sector, key, tmp);
}

int GetSecFloat(const char* sector, const char* key, float* value)
{
    if (!sector || !key || !value)
    {
        return INI_RET_ERROR;
    }
    char tmp[100] = "";
    if (INI_RET_OK != GetSecStr(sector, key, tmp))
    {
        return INI_RET_ERROR;
    }
    if (INI_RET_OK != CheckFloatStr(tmp))
    {
        return INI_RET_ERROR;
    }
    *value = atof(tmp);
    return INI_RET_OK;
}

int SetSecFloat(const char* sector, const char* key, float* value)
{
    if (!sector || !key || !value)
    {
        return INI_RET_ERROR;
    }
    char tmp[100] = "";
    snprintf(tmp, sizeof(tmp), "%f", *value);
    return SetSecStr(sector, key, tmp);
}

int GetSecDouble(const char* sector, const char* key, double* value)
{
    float tmp;
    if(INI_RET_OK != GetSecFloat(sector, key, &tmp))
    {
        return INI_RET_ERROR;
    }
    *value = tmp;
    return INI_RET_OK;
}

int SetSecDouble(const char* sector, const char* key, double* value)
{
    float tmp = *value;
    return SetSecFloat(sector, key, &tmp);
}

// --------------------

int GetThresholdStr(const char* dir_name, const char* file_name, char* value)
{
    if (!file_name || !value)
    {
        return INI_RET_ERROR;
    }
    FILE *fp;
    char file[1024] = "";
    snprintf(file, sizeof(file), "%s/%s", dir_name == NULL? DEFAULT_INI_DIR: dir_name, file_name);
    fp = fopen(file, "r");
    if (NULL == fp)
    {
        // LOG_ERROR("fopen file error. file: %s", file);
        return INI_RET_ERROR;
    }
    char line_buff[1024];
    if (NULL != fgets(line_buff, sizeof(line_buff), fp))
    {
        char* p = StrTrim(line_buff);
        sprintf(value, "%s", p);
        fclose(fp);
        return INI_RET_OK;
    }
    // LOG_ERROR("fgets file error. file: %s", file);
    fclose(fp);
    return INI_RET_ERROR;
}

int SetThresholdStr(const char* dir_name, const char* file_name, char* value)
{
    char read_value[1024] = "";
    if (INI_RET_OK == GetThresholdStr(dir_name, file_name, read_value))
    {
        if (strcmp(read_value, value) == 0)
        {
            return INI_RET_OK;
        }
    }

    if (!file_name || !value)
    {
        return INI_RET_ERROR;
    }
    FILE *fp;
    char file[1024] = "";
    snprintf(file, sizeof(file), "%s/%s", dir_name == NULL? DEFAULT_INI_DIR: dir_name, file_name);
    fp = fopen(file, "w");
    if (NULL == fp)
    {
//        LOG_ERROR("fopen file error. file: %s", file);
        return INI_RET_ERROR;
    }
    fprintf(fp, "%s\n", value);
    fclose(fp);
//    system("sync");
    return INI_RET_OK;
}

int GetThresholdInt(const char* dir_name, const char* file_name, int* value)
{
    if (!file_name || !value)
    {
        // LOG_ERROR("%d, %d, %d", dir_name, file_name, value);
        return INI_RET_ERROR;
    }
    char tmp[100] = "";
    if (INI_RET_OK != GetThresholdStr(dir_name, file_name, tmp))
    {
        // LOG_ERROR("GetThresholdStr error.", NULL);
        return INI_RET_ERROR;
    }
    if (INI_RET_OK != CheckIntStr(tmp))
    {
        // LOG_ERROR("CheckIntStr error. %s", tmp);
        return INI_RET_ERROR;
    }
    *value = atoi(tmp);
    return INI_RET_OK;
}

int SetThresholdInt(const char* dir_name, const char* file_name, int* value)
{
    if (!file_name || !value)
    {
        return INI_RET_ERROR;
    }
    char tmp[100] = "";
    snprintf(tmp, sizeof(tmp), "%d", *value);
    return SetThresholdStr(dir_name, file_name, tmp);
}

int GetThresholdFloat(const char* dir_name, const char* file_name, float* value)
{
    if (!file_name || !value)
    {
        return INI_RET_ERROR;
    }
    char tmp[100] = "";
    if (INI_RET_OK != GetThresholdStr(dir_name, file_name, tmp))
    {
        return INI_RET_ERROR;
    }
    if (INI_RET_OK != CheckFloatStr(tmp))
    {
        return INI_RET_ERROR;
    }
    *value = atof(tmp);
    return INI_RET_OK;
}

int SetThresholdFloat(const char* dir_name, const char* file_name, float* value)
{
    if (!file_name || !value)
    {
        return INI_RET_ERROR;
    }
    char tmp[100] = "";
    snprintf(tmp, sizeof(tmp), "%f", *value);
    return SetThresholdStr(dir_name, file_name, tmp);
}

int GetThresholdDouble(const char* dir_name, const char* file_name, double* value)
{
    float tmp;
    if(INI_RET_OK != GetThresholdFloat(dir_name, file_name, &tmp))
    {
        return INI_RET_ERROR;
    }
    *value = tmp;
    return INI_RET_OK;
}

int SetThresholdDouble(const char* dir_name, const char* file_name, double* value)
{
    float tmp = *value;
    return SetThresholdFloat(dir_name, file_name, &tmp);
}