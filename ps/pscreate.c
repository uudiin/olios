
/*-------------------------------------------------------------
    进程线程创建
    
    修改时间列表:
    2011-01-19 23:30:10 
-------------------------------------------------------------*/


#include    "ke.h"
#include    "proto.h"


/*-------------------------------------------------------------
-------------------------------------------------------------*/
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    进程创建
-------------------------------------------------------------*/
/*
PKPROCESS
_stdcall
PsCreateProcess(
    PKPROCESS   ParentProcess,
    ulong       Flags,
    pchar       ProcessName
    )
{
    PKPROCESS   Process;
    
    Process = (PKPROCESS)ExAllocatePool(sizeof(KPROCESS));
    memset(Process, 0, sizeof(KPROCESS));
    Process->Pid = PsNumberOfProcess;       // FIXME
    Process->BasePriority = PRIORITY_NORMAL;
    Process->DirectoryTableBase = MmCreateProcessAddressSpace();
    InitializeListHead(&Process->ThreadListHead);
    
    InsertTailList(&PsProcessListHead, &Process->ProcessListEntry);
    PsNumberOfProcess++;
    
    return Process;
}*/
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    内核线程创建
-------------------------------------------------------------*/

PKTHREAD
_stdcall
PsCreateSystemThread(
    pvoid       ThreadContext,      // ignore
    pvoid       StartRoutine,
    pvoid       StartContext
    )
{
    PKTHREAD    Thread;
    PMMPTE      PointerPte;
    MMPTE       TempPte;
    PKPROCESS   Process;
    PKTRAP_FRAME    TrapFrame;
    
    Process = PsSystemProcess;
    
    Thread = (PKTHREAD)ExAllocatePool(sizeof(KTHREAD));
    memset(Thread, 0, sizeof(KTHREAD));
    Thread->Process = Process;
    InsertTailList(&Process->ThreadListHead, &Thread->ThreadListEntry);
    Thread->Priority = Process->BasePriority;
    Thread->BasePriority = Process->BasePriority;
    
    Thread->QuantumReset = THREAD_QUANTUM;
    Thread->Ticks = Thread->QuantumReset;
    Thread->Tid = PsInsertCidTable(Thread);
    Thread->StartAddress = StartRoutine;
    
    InitializeListHead(&Thread->SenderListHead);
    
    Process->ThreadCount++;
    PsNumberOfThread++;
    
    //
    // 为线程分配内核栈，分配物理页面，在系统PTE区域保留一个PTE并映射
    //
    
    TempPte = MmValidKernelPte;
    TempPte.Hard.PageFrameNumber = MxGetNextPage(1);
    PointerPte = MiReserveSystemPtes(1);
    *PointerPte = TempPte;
    Thread->KernelStack = MiGetVirtualAddressMappedByPte(PointerPte) + PAGE_SIZE;
    Thread->PreviousMode = KERNEL_MODE;
    
    //
    // 在栈中保留一部分空间   FIXME
    //
    
    // nothing;
    
    //
    // 初始化线程的TRAP_FRAME
    //
    
    TrapFrame = (PKTRAP_FRAME)(Thread->KernelStack) - 1;
    Thread->InterruptedStack = TrapFrame;
    
    TrapFrame->NextFrame = NULL;
    TrapFrame->SegGs = KGDT_VIDEO;
    TrapFrame->SegFs = KGDT_R3_DATA;
    TrapFrame->SegEs = KGDT_R3_DATA;
    TrapFrame->SegDs = KGDT_R3_DATA;
    TrapFrame->Edi = NULL;
    TrapFrame->Esi = NULL;
    TrapFrame->Ebp = NULL;
    TrapFrame->KernelEsp = NULL;
    TrapFrame->Ebx = NULL;
    TrapFrame->Edx = NULL;
    TrapFrame->Ecx = NULL;
    TrapFrame->Eax = NULL;
    TrapFrame->ErrCode = NULL;
    TrapFrame->Eip = (ulong)StartRoutine;
    TrapFrame->SegCs = KGDT_R0_CODE;
    TrapFrame->Eflags = EFLAGS_SYSTEM_THREAD;
    TrapFrame->PrevEsp = NULL;                      // 线程的返回地址 FIXME
    TrapFrame->PrevSegSs = (ulong)StartContext;     // 参数
    Thread->TrapFrame = TrapFrame;
    
    //
    // 链入就绪线程链表
    //
    
    KiReadyThread(Thread);
    
    return Thread;
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    在PspCidTable中找一个空位将进程或者线程结构存储起来,返回索引作为ID
-------------------------------------------------------------*/

ulong
_stdcall
PsInsertCidTable(
    pvoid   Object
    )
{
    ulong   i = TASK_ID_MAXIMUM;
    
    while (i < (CID_TABLE_SIZE / 4))
    {
        if (PspCidTable[i] == 0)
        {
            PspCidTable[i] = (ulong)Object;
            break;
        }
        
        i++;
    }
    
    if (i == (CID_TABLE_SIZE / 4))
    {
        KiBugCheck("PsInsertCidTable: have non enough CID to allocated\n");
    }
    
    return i;
}
/*-----------------------------------------------------------*/


















