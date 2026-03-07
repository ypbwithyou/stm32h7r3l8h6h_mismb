/******************************************************************************
 
  
  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to 
    any third party.
  ------------------------------------------------------------------------
  ... ...
*******************************************************************************/
#include "test_main.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

// Test name
#define TEST_NAME    "test_link_list"
// 要测试的h文件
#include "link_list.h"

// 测试用例
int TestCase1(void)
{
    LinkList* link_list = LinkListCreate();
    if (NULL == link_list)
    {
        TEST_ERROR("LinkListCreate error. return NULL");
        return TEST_RET_FAIL;
    }
    char str_x[1024] = "";
    memset(str_x, 'x', sizeof(str_x));
    char str_y[1024] = "";
    memset(str_y, 'y', sizeof(str_y));
    int push_len = 512;
    char str_xy[1024] = "";
    char tmp[1024] = "";
    memset(str_xy, 'x', sizeof(str_xy));
    memset(str_xy + push_len, 'y', sizeof(str_xy) - push_len);
    if (LL_OK != LinkListPushBack(link_list, str_x, push_len))
    {
        TEST_ERROR("LinkListPushBack error. len: %d", push_len);
        return TEST_RET_FAIL;
    }
    if (LL_OK != LinkListPushBack(link_list, str_y, push_len))
    {
        TEST_ERROR("LinkListPushBack error. len: %d", push_len);
        return TEST_RET_FAIL;
    }
    int lenth = LinkListLength(link_list);
    if (push_len * 2 != lenth)
    {
        TEST_ERROR("LinkListLength error. len: %d, expect: %d", lenth, push_len * 2);
        return TEST_RET_FAIL;
    }
    int get_len = push_len*2;
    if (get_len != LinkListGetData(link_list, tmp, get_len))
    {
        TEST_ERROR("LinkListGetData error. len: %d", get_len);
        return TEST_RET_FAIL;
    }
    if (memcmp(tmp, str_xy, get_len) != 0)
    {
        TEST_ERROR("LinkListGetData data is wrong", get_len);
        return TEST_RET_FAIL;
    }
    LinkListDestroy(&link_list);
    return TEST_RET_PASS;
}

int TestCase2(void)
{
    int get_length;
    LinkList* link_list = LinkListCreate();
    if (NULL == link_list)
    {
        TEST_ERROR("LinkListCreate error. return NULL");
        return TEST_RET_FAIL;
    }
    char str_abc[] = "abcdefghijklmnopqrstuvwxyzasdbaqwiojxpjsdpjfpdsgdfgh";
    int len = strlen(str_abc) - (strlen(str_abc) % 3);
    for (size_t i = 0; i + 3 <= strlen(str_abc); i+=3)
    {
        LinkListPushBack(link_list, &str_abc[i], 3);
    }
    int lenth = LinkListLength(link_list);
    if (len != lenth)
    {
        TEST_ERROR("LinkListLength error. len: %d, expect: %d", lenth, len);
        return TEST_RET_FAIL;
    }
    char tmp[10] = "";
    for (int i = 0; i < len; ++i)
    {
        get_length = LinkListGetData(link_list, tmp, 1);
        if (1 != get_length)
        {
            TEST_ERROR("LinkListGetData error. len: %d, expect: %d", get_length, 1);
            LinkListDestroy(&link_list);
            return TEST_RET_FAIL;
        }
        if (tmp[0] != str_abc[i])
        {
            TEST_ERROR("LinkListGetData error. i: %d, str: %c, expect: %c", i, tmp[0], str_abc[i]);
            LinkListDestroy(&link_list);
            return TEST_RET_FAIL;
        }
        LinkListPopLength(link_list, 1);
    }
    LinkListDestroy(&link_list);
    return TEST_RET_PASS;
}

// 初始化
void TestCaseInit(void)
{
    // 设置test name
    UT_SetTestName(TEST_NAME);
    UT_AddCase("TestCase1", TestCase1);
    UT_AddCase("TestCase2", TestCase2);
}
