
/*-------------------------------------------------------------
    双向链表操作函数

    修改时间列表:
    2011-01-09 22:32:41 
-------------------------------------------------------------*/


#include    "ke.h"
#include    "proto.h"


/*-------------------------------------------------------------
-------------------------------------------------------------*/
/*-----------------------------------------------------------*/



//
// 初始化一个双向链表
//

void
_stdcall
InitializeListHead(
    PLIST_ENTRY ListHead
    )
{
    ListHead->Flink = ListHead;
    ListHead->Blink = ListHead;
}



//
// 将节点从链表中移除,移出后链表为空返回1
//

ulong
_stdcall
RemoveEntryList(
    PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Flink;

    Flink = Entry->Flink;
    Blink = Entry->Blink;
    Blink->Flink = Flink;
    Flink->Blink = Blink;
    
    return (Flink == Blink);
}



//
// 从链表头移出一个节点
//

PLIST_ENTRY
_stdcall
RemoveHeadList(
    PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Flink;
    Flink = Entry->Flink;
    ListHead->Flink = Flink;
    Flink->Blink = ListHead;
    return Entry;
}



//
// 从链表尾移出一个节点
//

PLIST_ENTRY
_stdcall
RemoveTailList(
    PLIST_ENTRY ListHead
    )
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Entry;

    Entry = ListHead->Blink;
    Blink = Entry->Blink;
    ListHead->Blink = Blink;
    Blink->Flink = ListHead;
    return Entry;
}



//
// 在链表尾插入一个节点
//

void
_stdcall
InsertTailList(
    PLIST_ENTRY ListHead,
    PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Blink;

    Blink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = Blink;
    Blink->Flink = Entry;
    ListHead->Blink = Entry;
}



//
// 在链表头插入一个节点
//

void
_stdcall
InsertHeadList(
    PLIST_ENTRY ListHead,
    PLIST_ENTRY Entry
    )
{
    PLIST_ENTRY Flink;

    Flink = ListHead->Flink;
    Entry->Flink = Flink;
    Entry->Blink = ListHead;
    Flink->Blink = Entry;
    ListHead->Flink = Entry;
}



