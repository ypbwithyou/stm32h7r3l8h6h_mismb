/******************************************************************************
 
  
  * This software can only be used for  production/manufacturing equipments.
  * Without permission, it cannot be copied and used in any form or provided to 
    any third party.
  ------------------------------------------------------------------------
  ... ...
*******************************************************************************/

/******************************************************************************
* Include files.
******************************************************************************/
#include "link_list.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/******************************************************************************
* Macros.
******************************************************************************/

/******************************************************************************
* Variables.
******************************************************************************/

/******************************************************************************
* Functions.
******************************************************************************/

// ------------------- static API ------------------------------
// 移除链表头部第一个节点，不加锁
static int LinkListPopFrontUnlock(LinkList* link_list)
{
    if (link_list->node_num > 0)
    {
        ListNode* tmp = link_list->head;
        if (NULL != tmp)
        {
            if (link_list->head == link_list->end)
            {
                link_list->end = link_list->end->next;
            }
            link_list->head = link_list->head->next;
            
            if (0 < tmp->data_len)
            {
                free(tmp->data);
                tmp->data = NULL;
                tmp->data_len = 0;
            }
            free(tmp);
        }
//        else
//        {
//            LOG_ERROR("LinkListPopFrontUnlock. link_list head is NULL. node_num: %d", link_list->node_num);
//        }
        --link_list->node_num;
        //LOG_INFO("LinkListPopFrontUnlock. node_num: %d", link_list->node_num);
        return LL_OK;
    }
    return LL_ERROR;
}

// ------------------- export API ------------------------------
// 创建链表, 成功返回指针, 失败返回NULL
LinkList* LinkListCreate(void)
{
    LinkList* list = malloc(sizeof(LinkList));
    if (!list)
    {
//        LOG_ERROR("LinkListCreate. malloc(%d) error. No enough memory.", sizeof(LinkList));
        return NULL;
    }
    list->head = NULL;
    list->end = NULL;
    list->node_num = 0;
//    pthread_mutex_init(&(list->mutex), NULL);
    list->xMutex = xSemaphoreCreateMutex();
    return list;
}

// 释放链表
void LinkListDestroy(LinkList** link_list)
{
    if (NULL == *link_list)
    {
        // LOG_ERROR("LinkListDestroy. link_list is NULL.");
        return;
    }
    LinkList* list = *link_list;
    *link_list = NULL;
    xSemaphoreTake(list->xMutex, pdMS_TO_TICKS(10));
    while(list->node_num > 0)
    {
        LinkListPopFrontUnlock(list);
    }
    xSemaphoreGive(list->xMutex);
    vSemaphoreDelete(list->xMutex);
    free(list);
    // *link_list = NULL;
}

// 创建一个Node
static ListNode* CreatNode(const void* data, int data_len)
{
    if (0 >= data_len || NULL == data)
    {
//        LOG_ERROR("CreatNode. para is error: %d, %d.", data, data_len);
        return NULL;
    }
    ListNode* node = malloc(sizeof(ListNode));
    if (NULL == node)
    {
//        LOG_ERROR("CreatNode. malloc(%d) error. No enough memory.", sizeof(ListNode));
        return NULL;
    }
    // node->ptr = NULL;
    node->data = malloc(data_len);
    if (NULL == node->data)
    {
//        LOG_ERROR("CreatNode. malloc(%d) error. No enough memory.", data_len);
        free(node);
        return NULL;
    }
    // memset(node->data, 0x00, data_len);
    memcpy(node->data, data, data_len);
    node->next = NULL;
    node->data_len = data_len;
	node->offset = 0;
    return node;
}

// 添加Node 到LinkList结尾
static void AppendNode(LinkList* link_list, ListNode* node)
{
    xSemaphoreTake(link_list->xMutex, pdMS_TO_TICKS(10));
    if (link_list->end == NULL)
    {
        link_list->end = node;
        link_list->head = node;
    }
    else
    {
        link_list->end->next = node;
        link_list->end = node;
    }
    ++link_list->node_num;
    // LOG_INFO("LinkListPushBack. node_num: %d", link_list->node_num);
    xSemaphoreGive(link_list->xMutex);
}

// 添加Node 到LinkList开头
static void AppendNodeFront(LinkList* link_list, ListNode* node)
{
    xSemaphoreTake(link_list->xMutex, pdMS_TO_TICKS(10));
    if (link_list->head == NULL)
    {
        link_list->end = node;
        link_list->head = node;
    }
    else
    {
        node->next = link_list->head;
        // link_list->end->next = node;
        link_list->head = node;
    }
    ++link_list->node_num;
    // LOG_INFO("LinkListPushBack. node_num: %d", link_list->node_num);
    xSemaphoreGive(link_list->xMutex);
}

// 添加节点到链表尾部
int LinkListPushBack(LinkList* link_list, const void* data, int data_len)
{
    if (NULL == link_list)
    {
//        LOG_ERROR("link_list is NULL.");
        return LL_ERROR;
    }
    ListNode* node = CreatNode(data, data_len);
    if (NULL == node)
    {
        return LL_ERROR;
    }
    // node->ptr = NULL;
    AppendNode(link_list, node);
    return LL_OK;
}

// 添加节点到链表头部
int LinkListPushFront(LinkList* link_list, const void* data, int data_len)
{
    if (NULL == link_list)
    {
//        LOG_ERROR("link_list is NULL.");
        return LL_ERROR;
    }
    ListNode* node = CreatNode(data, data_len);
    if (NULL == node)
    {
        return LL_ERROR;
    }
    // node->ptr = NULL;
    AppendNodeFront(link_list, node);
    return LL_OK;
}

// 创建一个ptr节点
static ListNode* CreatPtrNode(void* ptr)
{
    if (NULL == ptr)
    {
//        LOG_ERROR("CreatPtrNode. ptr is NULL.");
        return NULL;
    }
    ListNode* node = malloc(sizeof(ListNode));
    if (NULL == node)
    {
//        LOG_ERROR("CreatPtrNode. malloc(%d) error. No enough memory.", sizeof(ListNode));
        return NULL;
    }
    // node->ptr = NULL;
    node->data = ptr;
    node->next = NULL;
    node->data_len = 0;
    node->offset = 0;
    return node;
}

#if 1
// 添加指针节点到尾部, 内部不做释放指针释放
int LinkListPushBackPtr(LinkList* link_list, void* ptr)
{
    if (NULL == link_list)
    {
//        LOG_ERROR("link_list is NULL.");
        return LL_ERROR;
    }
    ListNode* node = CreatPtrNode(ptr);
    if (NULL == node)
    {
        return LL_ERROR;
    }
    AppendNode(link_list, node);
    return LL_OK;
}
#endif

// 移除链表头部第一个节点
int LinkListPopFront(LinkList* link_list)
{
    int ret;
    if (NULL == link_list)
    {
        return LL_ERROR;
    }
    xSemaphoreTake(link_list->xMutex, pdMS_TO_TICKS(10));
    ret = LinkListPopFrontUnlock(link_list);
    xSemaphoreGive(link_list->xMutex);
    return ret;
}

// 从链表头开始移除指定数量节点
int LinkListPopNodesFront(LinkList* link_list, int node_n)
{
    int ret;
    int num = 0;
    if ((NULL == link_list) || (node_n <= 0))
    {
        return LL_ERROR;
    }
    if (link_list->node_num < node_n) {
        num = link_list->node_num;
    }
    xSemaphoreTake(link_list->xMutex, pdMS_TO_TICKS(10));
    for (int i=0; i<num; i++) {
        LinkListPopFrontUnlock(link_list);
    }
    xSemaphoreGive(link_list->xMutex);
    return ret;
}

// 清空链表
int LinkListClear(LinkList* link_list)
{
    if (NULL == link_list)
    {
        // LOG_ERROR("link_list is NULL.");
        return LL_OK;
    }
    xSemaphoreTake(link_list->xMutex, pdMS_TO_TICKS(10));
    while(link_list->node_num > 0)
    {
        LinkListPopFrontUnlock(link_list);
    }
    xSemaphoreGive(link_list->xMutex);
    return LL_OK;
}

// 计算LinkList数据长度
int LinkListLength(LinkList* link_list)
{
    int length = 0;
    if (NULL == link_list)
    {
        return length;
    }
    ListNode* node = link_list->head;
    while(NULL != node)
    {
        length += (node->data_len - node->offset);
        node = node->next;
    }
    return length;
}

// 获取指定长度的数据, 返回值: 实际获取到的数据长度
int LinkListGetData(LinkList* link_list, char* data, int data_len)
{
    int length = 0;
    int node_len = 0;
    if (NULL == link_list)
    {
        return length;
    }
    ListNode* node = link_list->head;
    while(length < data_len)
    {
        if (NULL == node)
        {
            return length;
        }
        node_len = node->data_len - node->offset;
        if (0 == node_len)
        {
            return length;
        }
        if (length + node_len <= data_len)
        {
            memcpy(data + length, (char*)node->data + node->offset, node_len);
            length += node_len;
        }
        else
        {
            memcpy(data + length, (char*)node->data + node->offset, data_len - length);
            length = data_len;
        }
        node = node->next;
    }
    return length;
}

// 弹出head的data内容
int LinkListPopFrontData(LinkList* link_list, void* data, int data_len)
{
    if (NULL == link_list)
    {
        // LOG_ERROR("link_list is NULL.");
        return LL_ERROR;
    }
    xSemaphoreTake(link_list->xMutex, pdMS_TO_TICKS(10));
    if (NULL == link_list->head)
    {
        xSemaphoreGive(link_list->xMutex);
        return LL_ERROR;
    }
    if (link_list->head->data_len != data_len)
    {
//        LOG_ERROR("LinkListPopFrontData %d != %d", link_list->head->data_len, data_len);
        xSemaphoreGive(link_list->xMutex);
        return LL_ERROR;
    }
    memcpy(data, link_list->head->data, link_list->head->data_len);
    LinkListPopFrontUnlock(link_list);
    xSemaphoreGive(link_list->xMutex);
    return LL_OK;
}

// Pop指定长度的数据 
int LinkListPopLength(LinkList* link_list, int pop_len)
{
    // int length = 0;
    int node_len = 0;
    if (NULL == link_list)
    {
        return LL_ERROR;
    }
    ListNode* node = NULL;
    while(NULL != link_list->head && 0 < pop_len)
    {
        node = link_list->head;
        node_len = node->data_len - node->offset;
        if (0 == node_len)
        {
            return LL_ERROR;
        }
        if (node_len <= pop_len)
        {
            LinkListPopFront(link_list);
            pop_len -= node_len;
            continue;
        }
        else
        {
            node->offset += pop_len;
            return LL_OK;
        }
    }
    if (0 < pop_len)
    {
        return LL_ERROR;
    }
    return LL_OK;
}

// 移动链表头节点到尾节点位置
int LinkListMoveHeadToEnd(LinkList* link_list)
{
    if (NULL == link_list)
    {
        // LOG_ERROR("link_list is NULL.");
        return LL_ERROR;
    }
    xSemaphoreTake(link_list->xMutex, pdMS_TO_TICKS(10));
    if (link_list->head == link_list->end)
    {
        xSemaphoreGive(link_list->xMutex);
        return LL_OK;
    }
    link_list->end->next = link_list->head;
    link_list->head = link_list->head->next;
    link_list->end = link_list->end->next;
    link_list->end->next = NULL;
    xSemaphoreGive(link_list->xMutex);
    return LL_OK;
}

static void LinkListFreeNode(ListNode *data_node)
{
    if (!data_node) {
        return;
    }

    if (0 < data_node->data_len){
        free(data_node->data);
    }

    free(data_node);
}

// 从链表中移除指定节点
int LinkListRemoveNode(LinkList *link_list, ListNode *data_node)
{
    ListNode *node = NULL, *tmp = NULL;

    if (!link_list || !data_node) {
        return LL_ERROR;
    }

    if (!link_list->head) {
        return LL_ERROR;
    }

    xSemaphoreTake(link_list->xMutex, pdMS_TO_TICKS(10));
    node = link_list->head;
    if (node == data_node) {
        /* 移除头节点 */
        if (link_list->head == link_list->end){
            link_list->end = link_list->end->next;
        }

        link_list->head = link_list->head->next;
        link_list->node_num--;
        LinkListFreeNode(data_node);
        xSemaphoreGive(link_list->xMutex);
        return LL_OK;
    }

    while (node) {
        tmp = node->next;
        if (tmp == data_node) {
            if (tmp == link_list->end) {
                link_list->end = node;
            }

            node->next = tmp->next;
            link_list->node_num--;
            LinkListFreeNode(data_node);
            xSemaphoreGive(link_list->xMutex);
            return LL_OK;
        }

        node = node->next;
    }
    xSemaphoreGive(link_list->xMutex);
    return LL_ERROR;
}