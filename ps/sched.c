
/*-------------------------------------------------------------
    线程调度
    
    修改时间列表:
    2011-01-19 23:33:44 
    2011-02-26 16:37:45 
-------------------------------------------------------------*/


#include    "ke.h"
#include    "proto.h"


ulong       ScheduleCount;


/*-------------------------------------------------------------
-------------------------------------------------------------*/
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    线程调度
    FIXME
    只有系统线程的调度，还没有考虑用户级的进程空间的不同
-------------------------------------------------------------*/
/*
void
_stdcall
PsSchedule()
{
    PKTHREAD        Thread;
    ulong           i = PRIORITY_NORMAL;
    ulong           GreatTicks = 0;
    PLIST_ENTRY     Entry;
    
//    for (i = PRIORITY_MAXIMUM - 1; i >= 0; i--)
//    {
        do
        {
            Entry = PsReadyThreadListHead[i].Flink;
                
            while (Entry != &PsReadyThreadListHead[i])
            {
                Thread = CONTAINING_RECORD(Entry, KTHREAD, ReadyListEntry);
                
                // Flags    FIXME
                if (Thread->Flags == 0 && Thread->Ticks > GreatTicks)
                {
                    GreatTicks = Thread->Ticks;
                    PsCurrentThread = Thread;
                }
                    
                Entry = Entry->Flink;
            }
                
            if (GreatTicks == 0)
            {
                Entry = PsReadyThreadListHead[i].Flink;
            
                while (Entry != &PsReadyThreadListHead[i])
                {
                    Thread = CONTAINING_RECORD(Entry, KTHREAD, ReadyListEntry);
                
                    if (Thread->Flags == 0)     // FIXME 同上
                    {
                        Thread->Ticks = Thread->Priority;
                    }
                
                    Entry = Entry->Flink;
                }
            }
        } while (GreatTicks == 0);
//    }
}*/
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    线程调度
-------------------------------------------------------------*/

void
_stdcall
PsSchedule()
{
    PKTHREAD    CurrentThread;
    
    CurrentThread = PsCurrentThread;
    
    //
    // 若是因为时限用光而发生的调度，重新把该线程加入就绪队列
    //
    
    if (CurrentThread->Flags == 0)
    {
        CurrentThread->Ticks = CurrentThread->QuantumReset;
        
        CurrentThread->QuantumEndCount++;
        
        KiReadyThread(CurrentThread);
    }
    else
    {
        CurrentThread->QuantumEndCount = 0;
    }
        
    KiComputeNewPriority(CurrentThread);
    
    PsCurrentThread = KiFindReadyThread();
    
    assert(PsCurrentThread != 0);
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    将线程插入就绪线程队列,链入队列头
-------------------------------------------------------------*/

void
_stdcall
KiReadyThread(
    PKTHREAD    Thread
    )
{
    ulong   Priority;
    
    assert(Thread->Flags == 0);
    
    Priority = Thread->Priority;
    
    //
    // 0 priority only for idle thread
    //
    
    InsertTailList(&PsReadyThreadListHead[Priority], &Thread->ReadyListEntry);
    
    PsReadySummary |= (1 << Priority);
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    从就绪线程队列头中基于优先级选出第一个可运行线程
-------------------------------------------------------------*/

PKTHREAD
_stdcall
KiFindReadyThread()
{
    PLIST_ENTRY     Entry;
    ulong           HighPriority;
    ulong           PrioritySet;
    PKTHREAD        Thread = 0;
    
    PrioritySet = PsReadySummary;
    
    //
    // 找到就绪线程的最高优先级,这里一条bsr就搞定的
    //
    
    HighPriority = PRIORITY_MAXIMUM - 1;
    
    while (((PrioritySet & (1 << HighPriority)) == 0) && HighPriority)
    {
        HighPriority--;
    }
    
    if (HighPriority == 0)
        return PsIdleThread;
    
    //
    // 取出第一个线程作为就绪线程
    //
    
    if (PrioritySet)
    {
        Entry = PsReadyThreadListHead[HighPriority].Flink;
        
        assert(Entry != &PsReadyThreadListHead[HighPriority]);
        
        Thread = CONTAINING_RECORD(Entry, KTHREAD, ReadyListEntry);
        if (RemoveEntryList(&Thread->ReadyListEntry))
        {
            // 返回真，表明移出后链表为空
            PsReadySummary ^= (1 << HighPriority);
        }
    }
    
    return Thread;
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    设置线程优先级
-------------------------------------------------------------*/

void
_stdcall
KeSetThreadPriority(
    PKTHREAD    Thread,
    ulong       NewPriority
    )
{
    ulong   OldPriority;
    
    if (NewPriority > PRIORITY_MAXIMUM)
        return;
    
    OldPriority = Thread->Priority;
    
    Thread->Priority = NewPriority;
    
    //
    // 若是就绪线程，链入新的就绪队列
    //
    
    if (Thread->Flags == 0)
    {
        if (RemoveEntryList(&Thread->ReadyListEntry))
            PsReadySummary ^= (1 << OldPriority);
            
        KiReadyThread(Thread);
    }
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    计算线程的新优先级
-------------------------------------------------------------*/
/*
void
_stdcall
KiComputeNewPriority(
    PKTHREAD    Thread
    )
{
    ulong   OldPriority;
    ulong   NewPriority;
    
    //
    // 不能改变空闲线程的优先级
    //
    
    if (Thread == PsIdleThread)
        return;
    
    OldPriority = Thread->Priority;

//    PrintInt(OldPriority);
    
    //
    // 若线程在规定的时间片个数中一直运行，则降低其优先级，直到0然后再上升到基本优先级
    //
    
    if (Thread->QuantumEndCount >= QUANTUM_END_COUNT_MAX)
    {
        Thread->QuantumEndCount = 0;
        
        assert(Thread->Flags == 0)
        
        if (Thread->Flags == 0)
        {
            if (RemoveEntryList(&Thread->ReadyListEntry))
            {
            //    DisplayColorString("1", 18);
                PrintInt(PsReadySummary);
                DisplayString(" ");
                // 返回真，表明移出后链表为空
                PsReadySummary ^= (1 << OldPriority);
            }
        
            if (OldPriority == PRIORITY_LOWEST)
            {
                NewPriority = Thread->BasePriority;
            }
            else
            {
                NewPriority = OldPriority - 1;
            }
        
            //
            // 设置新优先级后加入就绪队列
            //
        
            Thread->Priority = NewPriority;
            KiReadyThread(Thread);
        }
    }
}*/
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/

void
_stdcall
KiComputeNewPriority(
    PKTHREAD    Thread
    )
{
    ulong   NewPriority;
    
    if (Thread == PsIdleThread)
        return;
        
    if (Thread->QuantumEndCount >= QUANTUM_END_COUNT_MAX)
    {
        Thread->QuantumEndCount = 0;
        
        NewPriority = Thread->Priority - 1;
        if (NewPriority == PRIORITY_LOWEST)
        {
            NewPriority = Thread->BasePriority;
        }
            
        /*****************************************/
        /*
        if (Thread == TestA)
        {
            DisplayColorString("A", 19);
            PrintInt(NewPriority);
        }
        else if (Thread == TestB)
        {
            DisplayColorString("B", 19);
            PrintInt(NewPriority);
        }
        else if (Thread == PsIdleThread)
        {
            DisplayColorString("Idle", 19);
            PrintInt(NewPriority);
        }
        */
        /*****************************************/
        
        KeSetThreadPriority(Thread, NewPriority);
    }
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    从就绪线程队列中基于优先级选出一个合适的可运行线程
-------------------------------------------------------------*/
/*
PKTHREAD
_stdcall
KiSelectNextThread()
{
    PKTHREAD    Thread;
    
    Thread = KiSelectReadyThread(0);
    if (Thread == 0)
    {
        Thread = PsIdleThread;
    }
    
    return Thread;
}*/
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    搜索就绪线程队列，范围是从指定的优先级到最高优先级，找到一个可运行的线程
-------------------------------------------------------------*/
/*
PKTHREAD
_stdcall
KiSelectReadyThread(
    ulong   LowPriority
    )
{
    ulong       HighPriority;
    PLIST_ENTRY Entry;
    ulong       PrioritySet;
    PKTHREAD    Thread = NULL;
    
    PrioritySet = PsReadySummary >> LowPriority;
    
    if (PrioritySet)
    {
        HighPriority = PRIORITY_MAXIMUM - 1;
    
        while ((PrioritySet & (1 << HighPriority)) == 0)
        {
            HighPriority--;
        }
        
        HighPriority += LowPriority;
        
        Entry = PsReadyThreadListHead[HighPriority].Flink;
        Thread = CONTAINING_RECORD(Entry, KTHREAD, ReadyListEntry);
    }
    
    return Thread;
}*/
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    切换线程环境
-------------------------------------------------------------*/
/*
ulong
_stdcall
KeSwapThread(
    PKTHREAD    OldThread,
    PKTHREAD    NewThread
    )
{
    assert(OldThread != NewThread);
    
    PsCurrentThread = NewThread;
    
    KiSwapThread(OldThread, NewThread);
    
    return TRUE;
}*/
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    切换线程环境
    这里做了简单处理，延时等到下次时钟中断到来的时候再做切换 FIXME
-------------------------------------------------------------*/

void
_stdcall
KeSwapThread(
    PKTHREAD    OldThread,
    PKTHREAD    NewThread
    )
{
    PKTHREAD    Thread;
    
    DisplayColorString("Swap", 11);
    
    Thread = KiFindReadyThread();
    if (Thread == PsIdleThread)
        return;

    assert(PsCurrentThread != Thread);
    KiSwapThread(PsCurrentThread, Thread);
    PsCurrentThread = Thread;
    return;
}
/*-----------------------------------------------------------*/











