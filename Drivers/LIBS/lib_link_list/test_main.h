/******************************************************************************
 
  
  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to any 
    third party.
  ------------------------------------------------------------------------
  文件名称: test_main.h
  文件标识: 
  摘    要: 
  当前版本: 1.0
  作    者: Jack.lei
  完成日期: 2019年3月27日
*******************************************************************************/
#ifndef UT_TEST_INCLUDE_TEST_MAIN_H_
#define UT_TEST_INCLUDE_TEST_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

#define OPEN_LOG    (1)
// #define OPEN_LOG    (0)

#define TEST_LOG_MAX_LEN    (1024)
#define TEST_UNUSED(x)      (void)(x)

void TestLog(const char* file, int line, const char* level, const char* fmt, ...);
#define TEST_INFO(...) do {TestLog(__FILE__, __LINE__, "INFO", ##__VA_ARGS__);} while(0)
#define TEST_ERROR(...) do {TestLog(__FILE__, __LINE__, "ERROR", ##__VA_ARGS__);} while(0)

#define TEST_RET_PASS       (0)
#define TEST_RET_FAIL       (1)

// 测试用例
typedef int (*TestCase)(void);

// 设置Test名称
void UT_SetTestName(const char* test_name);
// 添加UT用例
void UT_AddCase(const char* case_name, TestCase handle);

// 测试用例初始化
void TestCaseInit(void);

#ifdef __cplusplus
}
#endif

#endif // UT_TEST_INCLUDE_TEST_MAIN_H_
