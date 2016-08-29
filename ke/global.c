
/*-------------------------------------------------------------
    该文件中定义内核中的所有全局变量,不包括函数的定义
    和 proto.h 文件中的 extern 配合来完成在其它文件中全局变量的导入
    该文件只能包含头文件 type.h和ke.h 且该头文件中不能包含有其它变量的声明
    否则就会冲突

    修改时间列表：
    2011-01-04 21:14:34
    2011-01-08 03:14:51 
    2011-01-18 21:40:52 
-------------------------------------------------------------*/


#include    "type.h"
#include    "ke.h"

ulong   CurrentDisplayPosition = (80 * 2 + 0) * 2;  // 第2行，第0列
ulong   LoaderParameterBlock;
ulong   KiTssBase;
ulong   KiInterruptReEnter;
ulong   KiSystemTicks;

/***********************************************/

ulong       MmSystemRangeStart;
MEMORY_DESC MxFreeDescriptor;
ulong       MmNumberOfPhysicalPages;
ulong       MmLowestPhysicalPage;
ulong       MmHighestPhysicalPage;
ulong       MmTotalFreePages;

PMMPFN      MmPfnDatabase;
ulong       MxPfnAllocation;
ulong       MmNonPagedPoolStart;

// 系统PTE区域的起始和结束地址
ulong       MmSystemPteBase;
ulong       MmSystemPteEnd;

// 指示系统PTE区域的PTE的开始和结束PTE
PMMPTE      MmPteSystemPteBase;
PMMPTE      MmPteSystemPteEnd;

MMPTE       MmFirstFreeSystemPte;
ulong       MmTotalFreeSystemPtes;
pvoid       MiLowestSystemPteVirtualAddress;
ulong       MmTotalSystemPtes;
ulong       MmNumberOfSystemPtes;


MMPTE       MmValidKernelPte = { MM_PTE_VALID |
                                 MM_PTE_WRITE | 
                                 MM_PTE_DIRTY |
                                 MM_PTE_ACCESS };

MMPTE       MmValidUserPte = { MM_PTE_VALID |
                               MM_PTE_WRITE |
                               MM_PTE_OWNER |
                               MM_PTE_DIRTY |
                               MM_PTE_ACCESS };

LIST_ENTRY  MmNonPagedPoolFreeListHead[MI_MAX_FREE_LIST_HEADS];
ulong       MmNumberOfFreeNonPagedPool;
ulong       MiStartOfNonPagedPoolFrame;
ulong       MiEndOfNonPagedPoolFrame;

POOL_DESCRIPTOR     MmNonPagedPoolDescriptor;



/***********************************************/

PKPROCESS       PsSystemProcess;
LIST_ENTRY      PsProcessListHead;
ulong           PsNumberOfProcess;
ulong           PsNumberOfThread;
LIST_ENTRY      PsReadyThreadListHead[PRIORITY_MAXIMUM];
ulong           PsReadySummary;
PKTHREAD        PsCurrentThread;
PKTHREAD        PsIdleThread;
pulong          PspCidTable;

PKTHREAD        TestA;
PKTHREAD        TestB;

/***********************************************/












