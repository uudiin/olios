
/*-------------------------------------------------------------
    该文件中 extern 所有的全局变量并且声明所有的函数原型
    所有的.c原文件都应该包含这个头文件(global.c 除外)

    修改时间列表:
    2011-01-04 21:18:16 
    2011-01-05 23:57:11 
    2011-01-08 03:15:02 
    2011-01-09 00:24:51 
    2011-07-30 00:13:56 
    2011-08-16 00:33:38 
-------------------------------------------------------------*/

#ifndef     _OLIEX_PROTO_
#define     _OLIEX_PROTO_

#include    "type.h"
#include    "ke.h"


#define     ASSERT      // 开启 assert


//
// 导出全局变量
//

// global.c
extern  ulong   CurrentDisplayPosition;
extern  ulong   LoaderParameterBlock;
extern  ulong   KiTssBase;
extern  ulong   KiInterruptReEnter;
extern  ulong   KiSystemTicks;

extern  ulong   MmSystemRangeStart;
extern  MEMORY_DESC MxFreeDescriptor;
extern  ulong   MmNumberOfPhysicalPages;
extern  ulong   MmLowestPhysicalPage;
extern  ulong   MmHighestPhysicalPage;
extern  ulong   MmTotalFreePages;

extern  PMMPFN      MmPfnDatabase;
extern  ulong       MxPfnAllocation;
extern  ulong       MmNonPagedPoolStart;
extern  ulong       MmSystemPteBase;
extern  ulong       MmSystemPteEnd;
extern  PMMPTE      MmPteSystemPteBase;
extern  PMMPTE      MmPteSystemPteEnd;
extern  MMPTE       MmFirstFreeSystemPte;
extern  ulong       MmTotalFreeSystemPtes;
extern  pvoid       MiLowestSystemPteVirtualAddress;
extern  ulong       MmTotalSystemPtes;
extern  MMPTE       MmValidKernelPte;
extern  MMPTE       MmValidUserPte;
extern  ulong       MmNumberOfSystemPtes;
extern  LIST_ENTRY  MmNonPagedPoolFreeListHead[MI_MAX_FREE_LIST_HEADS];
extern  ulong       MmNumberOfFreeNonPagedPool;
extern  ulong       MiStartOfNonPagedPoolFrame;
extern  ulong       MiEndOfNonPagedPoolFrame;
extern  POOL_DESCRIPTOR     MmNonPagedPoolDescriptor;

extern  PKPROCESS       PsSystemProcess;
extern  LIST_ENTRY      PsProcessListHead;
extern  ulong           PsNumberOfProcess;
extern  ulong           PsNumberOfThread;
extern  LIST_ENTRY      PsReadyThreadListHead[PRIORITY_MAXIMUM];
extern  ulong           PsReadySummary;
extern  PKTHREAD        PsCurrentThread;
extern  PKTHREAD        PsIdleThread;
extern  pulong          PspCidTable;
extern  PKTHREAD        TestA;
extern  PKTHREAD        TestB;


// interrupt.asm
extern  TrapIdtStart;
extern  KiInterruptTemplate;
extern  KiInterruptDispatch;



/*****************************************************/


//
// 声明函数原型
//

// ke/sysentry.asm
void    _stdcall  KiIdleLoop(ulong);


// rtl/olsupa.asm
void    _stdcall  HalDisableIrq(ulong);
void    _stdcall  HalEnableIrq(ulong);

uchar   _stdcall  HalReadPortChar(ulong);
void    _stdcall  HalWritePortChar(ulong, uchar);
ushort  _stdcall  HalReadPortWord(ulong);
void    _stdcall  HalWritePortWord(ulong, ushort);
ulong   _stdcall  HalReadPortDword(ulong);
void    _stdcall  HalWritePortDword(ulong, ulong);

void    _stdcall  HalReadPort(ulong, pvoid, ulong);
void    _stdcall  HalWritePort(ulong, pvoid, ulong);
void    _stdcall  KiAcquireSpinLock(pulong);
void    _stdcall  KiReleaseSpinLock(pulong);
PSLIST_ENTRY  _stdcall  InterlockedPushEntrySList(PSLIST_HEADER, PSLIST_ENTRY);
PSLIST_ENTRY  _stdcall  InterlockedPopEntrySList(PSLIST_HEADER);
void    _stdcall  memcpy(pvoid, pvoid, ulong);
void    _stdcall  memset(pvoid, ulong, ulong);


// rtl/dispa.asm
void    _stdcall  DisplayInt(ulong);
void    _stdcall  DisplayColorString(pchar, ulong);
void    _stdcall  DisplayString(pchar);
void    _stdcall  DisplayReturn();
void    _stdcall  DisplayStackPtr();


// rtl/print.c
pchar   _stdcall  itoa(pchar, ulong);
void    _stdcall  print_var(pchar, ulong);
void    _stdcall  PrintInt(long);


// rtl/misc.c
void    _stdcall  spin(char*);

#ifdef  ASSERT
void    _stdcall  assertion_failure(char*, char*, char*, int);
#define     assert(exp) \
                if (exp) ; \
                else assertion_failure(#exp, __FILE__, __BASE_FILE__, __LINE__);
#else
#define     assert(exp)
#endif

void    _stdcall  DelayBreak();
void    _stdcall  KiBugCheck(pchar);


// rtl/olsup.c
void    _stdcall  InitializeListHead(PLIST_ENTRY);
ulong   _stdcall  RemoveEntryList(PLIST_ENTRY);
PLIST_ENTRY _stdcall  RemoveHeadList(PLIST_ENTRY);
PLIST_ENTRY _stdcall  RemoveTailList(PLIST_ENTRY);
void    _stdcall  InsertTailList(PLIST_ENTRY, PLIST_ENTRY);
void    _stdcall  InsertHeadList(PLIST_ENTRY, PLIST_ENTRY);


// ke/interrupt.asm
void    _stdcall  KiSetIdtr();
void    _stdcall  ret_to_r3();
void    _stdcall  IoDelay();


// ke/int.c
void    _stdcall  KiInitInterrupt();
void    _stdcall  KiInitTrap();
void    KiDefaultTrapHandle(long, long, long, long, long);  // 这个函数不清栈
void    _stdcall  KiInit8259A();
void    _stdcall  KeConnectInterrupt(ulong, pvoid, PKTHREAD);
void    _stdcall  IoWaitInterrupt();


// ke/clock.c
void    _stdcall  HalInitClock();
void    _stdcall  HalIrq0ClockService(PKINTERRUPT);


// ke/ipc.c
ulong   _stdcall  KeSendRecvMsg(ulong, ulong, PMESSAGE);
ulong   _stdcall  KiSendMsg(PKTHREAD, ulong, PMESSAGE);
ulong   _stdcall  KiRecvMsg(PKTHREAD, ulong, PMESSAGE);
ulong   _stdcall  KeIsDeadLock(PKTHREAD, PKTHREAD);
void    _stdcall  IoInformInterrupt(PKINTERRUPT);


// ke/systask.c
void    _stdcall  KeSystemTask(ulong);


// mm/mminit.c
void    _stdcall  MmInitSystem(PLOADER_PARAMETER_BLOCK);
ulong   _stdcall  MxGetNextPage(ulong);


// mm/allocpage.c
void    _stdcall  MiInitNonPagedPool();
pvoid   _stdcall  MiAllocatePoolPages(ulong);
ulong   _stdcall  MiFreePoolPages(pvoid);


// mm/syspte.c
void    _stdcall  MiInitSystemPtes(PMMPTE, ulong);
PMMPTE  _stdcall  MiReserveSystemPtes(ulong);
void    _stdcall  MiReleaseSystemPtes(PMMPTE, ulong);


// mm/pool.c
void    _stdcall  MmInitializeNonPagedPool();
pvoid   _stdcall  ExAllocatePoolWithTag(ulong, ulong);
void    _stdcall  ExFreePoolWithTag(pvoid, ulong);
pvoid   _stdcall  ExAllocatePool(ulong);
void    _stdcall  ExFreePool(pvoid);


// mm/pagefault.c
void    _stdcall  MmAccessFault(ulong, pvoid, ulong, PKTRAP_FRAME);


// mm/mmsup.c
pvoid   _stdcall  MmMapIoSpace(pvoid, ulong);
void    _stdcall  MmUnmapIoSpace(pvoid, ulong);
ulong   _stdcall  MmCreateProcessAddressSpace();


// ps/psinit.c
void    _stdcall  PsInitSystem();
void    _stdcall  KiIdleThread(pvoid);


// ps/pscreate.c
PKPROCESS   _stdcall  PsCreateProcess(PKPROCESS, ulong, pchar);
PKTHREAD    _stdcall  PsCreateSystemThread(pvoid, pvoid, pvoid);
ulong   _stdcall  PsInsertCidTable(pvoid);


// ps/sched.c
void    _stdcall  PsSchedule();
void    _stdcall  KiReadyThread(PKTHREAD);
PKTHREAD    _stdcall    KiFindReadyThread();
void    _stdcall  KeSetThreadPriority(PKTHREAD, ulong);
void    _stdcall  KiComputeNewPriority(PKTHREAD);
void    _stdcall  KeSwapThread(PKTHREAD, PKTHREAD);


// ps/pssup.c
pvoid   _stdcall  PsLookupProcessThreadById(ulong);
PKTHREAD    _stdcall    PsGetCurrentThread();
PKPROCESS   _stdcall    PsGetCurrentProcess();
ulong   _stdcall  PsGetCurrentThreadId();
ulong   _stdcall  PsGetCurrentProcessId();
ulong   _stdcall  PsGetCurrentTaskId();


// ps/ctxswap.asm
void    _stdcall  KiSwapThread(PKTHREAD, PKTHREAD);
void    _stdcall  KiSwapContext();


// io/ata/atapi.c
void    _stdcall  IoAtapiTask(ulong);


// io/pci/pci.c
void    _stdcall  IoPciEnum();


#endif  // _OLIEX_PROTO_

















