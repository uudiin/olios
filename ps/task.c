
/*-------------------------------------------------------------
    内核任务支持
    
    修改时间列表:
    2011-01-24 21:23:53 
    2011-02-26 22:56:00 
-------------------------------------------------------------*/


#include    "ke.h"
#include    "proto.h"


/*-------------------------------------------------------------
-------------------------------------------------------------*/
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    把当前系统线程注册成一个任务
-------------------------------------------------------------*/

ulong
_stdcall
KeRegisterTaskThread(
    ulong   TaskId
    )
{
    PKTHREAD    Thread;
    ulong       OldTid;
    
    if (TaskId >= TASK_ID_MAXIMUM)
        return FALSE;
    
    if (PspCidTable[TaskId])
        return FALSE;
        
    Thread = PsGetCurrentThread();
    OldTid = Thread->Tid;
    
    PspCidTable[TaskId] = (ulong)Thread;
    Thread->TaskId = TaskId;
    
    PspCidTable[OldTid] = NULL;
    Thread->Tid = TaskId;
    
    return TRUE;
}
/*-----------------------------------------------------------*/






















