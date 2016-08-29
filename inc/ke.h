
/*-------------------------------------------------------------
    包括内核数据结构和常量的定义

    NOTE: 这个文件中的定义要与 ke.inc 中的定义同步

    修改时间列表:
    2011-01-04 22:04:05 
    2011-01-05 22:53:22 
-------------------------------------------------------------*/


#ifndef     _OLIEX_KE_
#define     _OLIEX_KE_


#include    "type.h"


#define     KGDT_R0_CODE    0x8
#define     KGDT_R0_DATA    0x10
#define     KGDT_R3_CODE    (0x18 | 3)
#define     KGDT_R3_DATA    (0x20 | 3)
#define     KGDT_VIDEO      (0x28 | 3)
#define     KGDT_TSS        0x30

#define     KSEG0_BASE      0x80000000
#define     KIDT_BASE       0x80090400
#define     IDT_LENGTH      0x800

#define     IRQ0_CLOCK          0
#define     IRQ1_KEYBOARD       1
#define     IRQ2_CASCADE        2
#define     IRQ3_ETHER          3
#define     IRQ12_PS2MOUSE      12
#define     IRQ14_ATA_WINI      14

#define     EOI             0x20

#define     INT_VECTOR_SYSCALL  0x20
#define     INT_VECTOR_IRQ0     0x30
#define     INT_VECTOR_IRQ8     (INT_VECTOR_IRQ0 + 8)

#define     TIMER_FREQUENCY     1193182
#define     OLIEX_HZ            100

#define     PORT_INTM_CTRL      0x20
#define     PORT_INTM_CTRLMASK  0x21
#define     PORT_INTS_CTRL      0xa0
#define     PORT_INTS_CTRLMASK  0xa1
#define     PORT_8253_MODE      0x43
#define     PORT_8253_COUNTER0  0x40


typedef struct _LIST_ENTRY
{
    struct _LIST_ENTRY  *Flink;
    struct _LIST_ENTRY  *Blink;
} LIST_ENTRY, *PLIST_ENTRY;


typedef struct _SLIST_ENTRY
{
    struct _SLIST_ENTRY *Next;
} SLIST_ENTRY, *PSLIST_ENTRY;


typedef struct _SLIST_HEADER
{
    SLIST_ENTRY     Next;
    ushort          Depth;
    ushort          Sequence;
} _packed SLIST_HEADER, *PSLIST_HEADER;


#define     IsListEmpty(list)       (((list)->Flink) == (list))
#define     CONTAINING_RECORD(address, type, field) \
                ((type *)((pchar)(address) - (ulong)(&((type *)0)->field)))


/**********************************************************/


typedef struct  _MEMORY_RANGE
{
    ulong   Base;
    ulong   Size;
} MEMORY_RANGE, *PMEMORY_RANGE;


typedef struct  _LOADER_PARAMETER_BLOCK
{
    PMEMORY_RANGE   MemoryRangeArray;
    ulong           MemRangeCount;
    ulong           KernelPhysicalBase;
    ulong           KernelImageSize;
    ulong           KernelFileSize;
    ulong           TotalMemorySize;
} LOADER_PARAMETER_BLOCK, *PLOADER_PARAMETER_BLOCK;


/**********************************************************/


typedef struct  _KTRAP_FRAME
{
    struct _KTRAP_FRAME     *NextFrame;
    ulong    Reserved1;
    ulong    Reserved2;
    ulong    Reserved3;
    ulong    Reserved4;
    ulong    Reserved5;
    
    ulong    SegGs;
    ulong    SegFs;
    ulong    SegEs;
    ulong    SegDs;

    ulong    Edi;
    ulong    Esi;
    ulong    Ebp;
    ulong    KernelEsp;
    ulong    Ebx;
    ulong    Edx;
    ulong    Ecx;
    ulong    Eax;

    ulong    ErrCode;
    ulong    Eip;
    ulong    SegCs;
    ulong    Eflags;
    ulong    PrevEsp;
    ulong    PrevSegSs;
} KTRAP_FRAME, *PKTRAP_FRAME;


#define     INTERRUPT_GATE  0x8e
#define     USER_INTERRUPT  0x60
#define     TRAP_IDT_COUNT  0x20
#define     IDT_BREAKPOINT  0x3
#define     IDT_OVERFLOW    0x4

typedef struct  _I386_GATE
{
    short   OffsetLow;
    short   Selector;
    char    Count;
    char    Attribute;
    short   OffsetHigh;
} _packed I386_GATE, *PI386_GATE;



/***********************************************************************/

#define     PAGE_SHIFT          12
#define     PAGE_SIZE           0x1000
#define     _1M                 (1024 * 1024)
#define     _16M                (16 * 1024 * 1024)
#define     PFN_MASK            0xfffff000
#define     PFN_DATA_BASE       (KSEG0_BASE + _16M)
#define     NOPAGED_POOL_SIZE   _16M
#define     PTE_BASE            0xc0000000
#define     PDE_BASE            ((PTE_BASE >> 10) + PTE_BASE)

// 定义系统PTE区域
#define     SYSTEM_PTES_BASE    0xf0000000
#define     SYSTEM_PTES_SIZE    0x08000000

#define     MiGetPteOffset(va)  ((((ulong)(va)) << 10) >> 22)

#define     MiGetPdeAddress(va) \
                ((PMMPTE)(((((ulong)(va)) >> 22) << 2) + PDE_BASE))
                
#define     MiGetPteAddress(va) \
                ((PMMPTE)(((((ulong)(va)) >> 12) << 2) + PTE_BASE))
                
#define     MiGetVirtualAddressMappedByPde(Pde) \
                ((pvoid)((ulong)(Pde) << 20))
                
#define     MiGetVirtualAddressMappedByPte(Pte) \
                ((pvoid)((ulong)(Pte) << 10))


#define     ROUND_TO_SIZE(L, Align) \
                (((L) + ((Align) - 1)) & ~((Align) - 1))
                
#define     BYTES_TO_PAGES(Size) \
                (((Size) >> PAGE_SHIFT) + (((Size) & (PAGE_SIZE - 1)) != 0))

#define     PAGE_ALIGNED(p)     (!(((ulong)(p)) & (PAGE_SIZE - 1)))


typedef struct  _MEMORY_DESC
{
    ulong   BasePage;
    ulong   PageCount;
} MEMORY_DESC, *PMEMORY_DESC;


#define     MM_PTE_VALID            0x1
#define     MM_PTE_WRITE            0x2
#define     MM_PTE_OWNER            0x4
#define     MM_PTE_WRITE_THROUGH    0x8
#define     MM_PTE_CACHE_DISABLE    0x10
#define     MM_PTE_ACCESS           0x20
#define     MM_PTE_DIRTY            0x40
#define     MM_PTE_LARGE_PAGE       0x80
#define     MM_PTE_GLOBAL           0x100
#define     MM_PTE_COPY_ON_WRITE    0x200
#define     MM_PTE_PROTOTYPE        0x400
#define     MM_PTE_TRANSITION       0x800

// x86 硬件PTE定义

typedef struct _HARDWARE_PTE
{
    ulong   Valid : 1;
    ulong   Write : 1;
    ulong   Owner : 1;
    ulong   WriteThrough : 1;
    ulong   CacheDisable : 1;
    ulong   Accessed : 1;
    ulong   Dirty : 1;
    ulong   LargePage : 1;
    ulong   Global : 1;
    ulong   CopyOnWrite : 1;
    ulong   ProtoType : 1;
    ulong   Reserved : 1;
    ulong   PageFrameNumber : 20;
} _packed HARDWARE_PTE, *PHARDWARE_PTE;


// 系统PTE格式定义
typedef struct _MMPTE_LIST
{
    ulong   Valid : 1;
    ulong   OneEntry : 1;
    ulong   Filler0 : 8;
    ulong   ProtoType : 1;
    ulong   Filler1 : 1;
    ulong   NextEntry : 20;
} _packed MMPTE_LIST, *PMMPTE_LIST;


#define     MM_EMPTY_PTE_LIST       ((ulong)0xfffff)
#define     MM_SYSPTE_TABLES_MAX    5

// PTE共用体
typedef struct _MMPTE
{
    union
    {
        ulong           Long;
        HARDWARE_PTE    Hard;
        MMPTE_LIST      List;
    };
} _packed MMPTE, *PMMPTE;


#define     MiPfnElement(i)             (&MmPfnDatabase[i])
#define     MiPfnElementToIndex(Pfn) \
                ((ulong)(((ulong)(Pfn) - (ulong)MmPfnDatabase) / sizeof(MMPFN)))


typedef struct _MMPFNENTRY
{
    ushort  Modified : 1;
    ushort  StartOfAllocation : 1;
    ushort  EndOfAllocation : 1;
    ushort  PrototypePte : 1;
    ushort  PageColor : 4;
    ushort  PageLocation : 3;
    ushort  RemoveRequested : 1;
    ushort  CacheAttribute : 2;
    ushort  Rom : 1;
    ushort  ParityError : 1;
} _packed MMPFNENTRY, *PMMPFNENTRY;


// PFN 结构定义
typedef struct _MMPFN
{
    union
    {
        ulong   WsIndex;
        ulong   Flink;
    };
    PMMPTE  PteAddress;
    union
    {
        ulong   Blink;
        ulong   ShareCount;
    } u2;
    union
    {
        ushort      ReferenceCount;
        MMPFNENTRY  ShortFlags;
    } u3;
    MMPTE   OriginalPte;
    ulong   u4;
} _packed MMPFN, *PMMPFN;


#define     MI_MAX_FREE_LIST_HEADS      4
#define     MI_FREE_POOL_SIGNATURE      0x12345678

// 非分页内存池中的内存页链表结构
typedef struct _MMFREE_POOL_ENTRY
{
    LIST_ENTRY  List;
    ulong       Size;
    ulong       Signature;
    struct _MMFREE_POOL_ENTRY   *Owner;
} MMFREE_POOL_ENTRY, *PMMFREE_POOL_ENTRY;


#define     POOL_SMALLEST_BLOCK         0x8
#define     POOL_BLOCK_SHIFT            3
#define     POOL_BUDDY_MAX \
                (PAGE_SIZE - (sizeof(POOL_HEADER) + POOL_SMALLEST_BLOCK))

#define     POOL_LIST_HEADS             (PAGE_SIZE / POOL_SMALLEST_BLOCK)
#define     DEFAULT_TAG                 0x65656565

typedef struct _POOL_DESCRIPTOR
{
    ulong       RunningAllocs;
    ulong       RunningDeAllocs;
    ulong       TotalPages;
    ulong       TotalBytes;
    ulong       LockAddress;
    ulong       PendingFrees;
    ulong       PendingFreeDepth;
    LIST_ENTRY  ListHeads[POOL_LIST_HEADS];
} _packed POOL_DESCRIPTOR, *PPOOL_DESCRIPTOR;

typedef struct _POOL_HEADER
{
    union
    {
        struct
        {
            ulong   PreviousSize : 9;
            ulong   PoolIndex : 7;
            ulong   BlockSize : 9;
            ulong   PoolType : 7;
        };
        ulong   Long;
    };
    ulong   PoolTag;
} _packed POOL_HEADER, *PPOOL_HEADER;

typedef struct _POOL_BLOCK
{
    uchar   Filler[1 << POOL_BLOCK_SHIFT];
} POOL_BLOCK, *PPOOL_BLOCK;


/**********************************************************/

#define     PRIORITY_LOWEST                 0
#define     PRIORITY_NORMAL                 8
#define     PRIORITY_MAXIMUM                32

#define     EFLAGS_SYSTEM_THREAD            0x202   // IOPL = 0, IF = 1(中断开)
#define     KERNEL_MODE                     0
#define     USER_MODE                       3

#define     CID_TABLE_SIZE                  (2 * PAGE_SIZE)     // 8K


typedef struct _DISPATCH_HEADER
{
    ulong       Reserved;
    ulong       SignalState;
    LIST_ENTRY  WaitListHead;
} DISPATCH_HEADER, *PDISPATCH_HEADER;


// 这个结构中各个域的位置不能随便改，必须和 ke.inc中的定义相对应

typedef struct _KPROCESS
{
    DISPATCH_HEADER Header;
    LIST_ENTRY      ThreadListHead;
    LIST_ENTRY      ReadyListHead;
    LIST_ENTRY      ProcessListEntry;
    ulong       DirectoryTableBase;
    ulong       Pid;
    ulong       BasePriority;
    ulong       ProcessLock;
    ulong       Flags;
    ulong       ReservedFlags;
    ulong       IopmOffset;
    ulong       KernelTime;
    ulong       UserTime;
    char        ImageName[16];
    ulong       ThreadCount;
} _packed KPROCESS, *PKPROCESS;



typedef struct  _MESSAGE
{
    ulong       SourceTid;
    ulong       Type;
    ulong       Flags;
    ulong       Reserved;
    ulong       Parameters[8];
} _packed MESSAGE, *PMESSAGE;


#define     THREAD_QUANTUM                  3
#define     QUANTUM_END_COUNT_MAX           3

// 这个结构中各个域的位置不能随便改，必须和 ke.inc中的定义相对应

typedef struct _KTHREAD
{
    DISPATCH_HEADER Header;
    PKTRAP_FRAME    TrapFrame;
    PKPROCESS       Process;
    LIST_ENTRY      ThreadListEntry;    // list in process
    ulong       Priority;
    ulong       Ticks;
    pvoid       KernelStack;
    ulong       Tid;
    ulong       ThreadLock;
    union
    {
        ulong       Flags;
        struct
        {
            ulong   Sending : 1;
            ulong   Receiving : 1;
            ulong   Waiting : 1;
            ulong   Reserved0 : 1;
            ulong   Reserved1 : 28;
        };
    };
    ulong       PreviousMode;
    ulong       State;
    ulong       Running;
    pvoid       StartAddress;
    ulong       ReservedFlags;
    LIST_ENTRY  ReadyListEntry;
    pvoid       InterruptedStack;       // ignore
    ulong       QuantumReset;
    ulong       QuantumEndCount;
    ulong       BasePriority;
    ulong       TaskId;
    PMESSAGE    Msg;
    ulong       ReceiveFrom;
    ulong       SendTo;
    ulong       HasInterruptMsg;
    LIST_ENTRY  SenderListHead;
    LIST_ENTRY  SenderListEntry;
} _packed KTHREAD, *PKTHREAD;


#define     TASK_SYSTEM             0
#define     TASK_KEYBOARD           1
#define     TASK_PS2                2
#define     TASK_ATA_WINI           3

#define     TASK_ID_MAXIMUM         32

#define     SYS_GET_TICKS           1

// 任务ID定义
#define     ANY                     0xffffff01
#define     INTERRUPT               0xffffff02

#define     HARDWARE_INTERRUPT      1

#define     SEND            1
#define     RECEIVE         2
#define     BOTH            3


/**********************************************************/

#define     INTERRUPT_TEMPLATE_SIZE     0x30


// 这个结构中各个域的位置不能随便改，必须和 ke.inc中的定义相对应

typedef struct _KINTERRUPT
{
    ulong       Size;
    LIST_ENTRY  List;
    pvoid       ServiceRoutine;
    pvoid       DispatchAddress;
    uchar       Vector;
    uchar       Irql;
    uchar       SynchronizeIrql;
    uchar       Connected;
    PKTHREAD    ServiceThread;
    char        DispatchCode[128];
} _packed KINTERRUPT, *PKINTERRUPT;

/**********************************************************/


#endif  // _OLIEX_KE_
























