/******************************************************************************
 
  
  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to 
    any third party.
  ------------------------------------------------------------------------
  ... ...
*******************************************************************************/
#include "test_main.h"
#if OPEN_LOG
#include "log.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdarg.h>

typedef struct Cases
{
    TestCase test_handle;
    char test_name[100];
}Cases;

static char g_test_name[100] = "Unknown Test";
static Cases g_case[100];
static int g_case_count = 0;

void TestLog(const char* file, int line, const char* level, const char* fmt, ...)
{
    va_list ap;
    char msg[TEST_LOG_MAX_LEN];
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);
    printf("[%s][%s:%d] %s\n", level, file, line, msg);
}

// 设置Test名称
void UT_SetTestName(const char* test_name)
{
    snprintf(g_test_name, sizeof(g_test_name), "%s", test_name);
}

// 添加UT用例
void UT_AddCase(const char* case_name, TestCase handle)
{
    g_case[g_case_count].test_handle = handle;
    snprintf(g_case[g_case_count].test_name, sizeof(g_case[g_case_count].test_name), "%s", case_name);
    ++g_case_count;
}

int main(int argc, char** argv)
{
    int ret;
    TEST_UNUSED(argc);
    TEST_UNUSED(argv);
    TestCaseInit();
#if OPEN_LOG
    ConfigLogModule(NULL, g_test_name, LOG_LEVEL_INFO);
#endif
    TEST_INFO("%s start ---", g_test_name);
    for (int i = 0; i < g_case_count; ++i)
    {
        TEST_INFO("--- case: %s start", g_case[i].test_name);
        ret = g_case[i].test_handle();
        if (TEST_RET_PASS == ret)
        {
            TEST_INFO("--- case: %s pass", g_case[i].test_name);
        }
        else
        {
            TEST_ERROR("--- case: %s failed", g_case[i].test_name);
        }
    }
    TEST_INFO("%s exit ---", g_test_name);
    return TEST_RET_PASS;
}