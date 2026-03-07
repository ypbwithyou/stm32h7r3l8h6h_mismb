/******************************************************************************
 
  
  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to any 
    third party.
  ------------------------------------------------------------------------
  文件名称: link_list.h
  文件标识: 
  摘    要: 
  当前版本: 1.0
  作    者: Jack.lei
  完成日期: 2018年12月05日
*******************************************************************************/
#ifndef LIB_LINK_LIST_INCLUDE_LINK_LIST_H_
#define LIB_LINK_LIST_INCLUDE_LINK_LIST_H_

/******************************************************************************
* Include files.
******************************************************************************/
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/******************************************************************************
* Macros.
******************************************************************************/
#define LL_OK               (0)
#define LL_ERROR            (-1)

typedef struct ListNode
{
    void*               data;
    int                 data_len;
    int                 offset;
    // void*               ptr;
    struct ListNode*    next;
}ListNode;

typedef struct LinkList
{
    ListNode*   head;
    ListNode*   end;
    int         node_num;
    SemaphoreHandle_t xMutex;
}LinkList;

/******************************************************************************
* Variables.
******************************************************************************/

/******************************************************************************
* Functions.
******************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

// 创建链表, 成功返回指针, 失败返回NULL
LinkList* LinkListCreate(void);

// 释放链表
void LinkListDestroy(LinkList** link_list);

// 添加节点到链表尾部
int LinkListPushBack(LinkList* link_list, const void* data, int data_len);
// 添加指针节点到尾部, 内部不做指针释放
int LinkListPushBackPtr(LinkList* link_list, void* ptr);

// 计算LinkList数据长度
int LinkListLength(LinkList* link_list);

// 获取指定长度的数据
int LinkListGetData(LinkList* link_list, char* data, int data_len);

// Pop指定长度的数据
int LinkListPopLength(LinkList* link_list, int pop_len);

// 添加节点到链表头部
int LinkListPushFront(LinkList* link_list, const void* data, int data_len);

// 移除链表尾部最后一个节点
// int LinkListPopBack(void* data, int data_len);

// 弹出head的data内容
int LinkListPopFrontData(LinkList* link_list, void* data, int data_len);

// 移除链表头部第一个节点
int LinkListPopFront(LinkList* link_list);

// 从链表头开始移除指定数量节点
int LinkListPopNodesFront(LinkList* link_list, int node_n);

// 移动链表头节点到尾节点位置
int LinkListMoveHeadToEnd(LinkList* link_list);

// 清空链表
int LinkListClear(LinkList* link_list);

// 从链表中移除指定节点
int LinkListRemoveNode(LinkList *link_list, ListNode *data_node);

#ifdef __cplusplus
}
#endif

#endif // LIB_LINK_LIST_INCLUDE_LINK_LIST_H_
