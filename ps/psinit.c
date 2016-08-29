
/*-------------------------------------------------------------
    进程管理初始化
    
    修改时间列表:
    2011-01-18 21:32:19 
    2011-01-19 22:18:02 
-------------------------------------------------------------*/


#include    "ke.h"
#include    "proto.h"


void    _stdcall    DelayThread(int);
void    _stdcall    ThreadTestA(ulong);
void    _stdcall    ThreadTestB(ulong);
void    _stdcall    ThreadTestC(ulong);


/*-------------------------------------------------------------
-------------------------------------------------------------*/
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    进程管理初始化
-------------------------------------------------------------*/

void
_stdcall
PsInitSystem()
{
    ulong   i;
    
    InitializeListHead(&PsProcessListHead);
    
    for (i = 0; i < PRIORITY_MAXIMUM; i++)
    {
        InitializeListHead(&PsReadyThreadListHead[i]);
    }
    
    PspCidTable = ExAllocatePool(CID_TABLE_SIZE);
    if (PspCidTable == 0)
    {
        KiBugCheck("PsInitSystem: have no non paged pool\n");
    }
    
    memset(PspCidTable, 0, CID_TABLE_SIZE);
    
    //
    // 初始化系统进程结构
    //
    
    PsSystemProcess = (PKPROCESS)ExAllocatePool(sizeof(KPROCESS));
    
    memset(PsSystemProcess, 0, sizeof(KPROCESS));
    
    PsSystemProcess->Pid = PsInsertCidTable(PsSystemProcess);
    PsSystemProcess->DirectoryTableBase = (ulong)(MiGetPteAddress(PDE_BASE)->Hard.PageFrameNumber);
    PsSystemProcess->BasePriority = PRIORITY_NORMAL;
    InitializeListHead(&PsSystemProcess->ThreadListHead);
    memcpy(PsSystemProcess->ImageName, "System", 6);        // FIXME
    
    InsertTailList(&PsProcessListHead, &PsSystemProcess->ProcessListEntry);
    PsNumberOfProcess++;
    
    //
    // 创建空闲线程
    //
    
    PsIdleThread = PsCreateSystemThread(0, &KiIdleLoop, 0);    // 这里要取地址
    
    PsCreateSystemThread(0, KeSystemTask, 0);
    //PsCreateSystemThread(0, IoAtapiTask, 0);
    
    //
    // 创建测试系统线程，这里只为调试用
    //
    
    TestA = PsCreateSystemThread(0, ThreadTestA, (pvoid)0xaaaaaaaa);
    TestB = PsCreateSystemThread(0, ThreadTestB, (pvoid)0xbbbbbbbb);
    PsCreateSystemThread(0, ThreadTestC, (pvoid)0xcccccccc);
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    由空闲循环线程循环调用
-------------------------------------------------------------*/

void
_stdcall
KiIdleThread(
    pvoid   Context
    )
{
    int i;
    
    DisplayColorString("I", 9);
    
    for (i = 0; i < 0x8000000; i++)
        __asm__ __volatile__("nop");
    
    return;
}
/*-----------------------------------------------------------*/



void _stdcall DelayThread(int Internal)
{
	int	i, j;

	for (i = 0; i < Internal; i++)
		for (j = 0; j < 0x30ff; j++)
		{
			__asm__ __volatile__("nop");
		}
}



int _stdcall GetTicks()
{
    MESSAGE     Msg;
    
    memset(&Msg, 0, sizeof(MESSAGE));
    Msg.Type = SYS_GET_TICKS;
    KeSendRecvMsg(BOTH, TASK_SYSTEM, &Msg);
    
//    DisplayColorString("GetTicks", 14);
    
    return Msg.Reserved;
}



void _stdcall PrintHarddiskInfo()
{
    MESSAGE     Msg;
    
    memset(&Msg, 0, sizeof(MESSAGE));
    Msg.Type = 1;
    KeSendRecvMsg(BOTH, TASK_ATA_WINI, &Msg);
    return;
}



void _stdcall ThreadTestA(ulong Context)
{
    PrintInt(Context);
    
    //PrintHarddiskInfo();
    
    while (TRUE)
    {
        DisplayColorString("a", 14);
        //PrintInt(GetTicks());
        DelayThread(0x8000);
    }
}



void _stdcall ThreadTestB(ulong Context)
{
    int     i;
    
    PrintInt(Context);
    
    while (TRUE)
    {
        DisplayColorString("b", 10);
        //PrintInt(GetTicks());
        for (i = 0; i < 0x60000000; i++)
        {
            __asm__ __volatile__("nop");
        }
    }
}



void _stdcall ThreadTestC(ulong Context)
{
    int     i;
    
    PrintInt(Context);
    
    while (TRUE)
    {
        DisplayColorString("c", 9);
        for (i = 0; i < 0x30000000; i++)
        {
            __asm__ __volatile__("nop");
        }
    }
}





















