
/*-------------------------------------------------------------
    包含IDT表处理的代码和默认的中断处理例程

    修改时间列表:
    2011-01-04 22:46:35 
    2011-01-17 22:14:54 
    2011-01-19 22:07:41 
    2011-08-09 22:51:42 
-------------------------------------------------------------*/


#include    "ke.h"
#include    "proto.h"


/*-------------------------------------------------
    检查CPU类型及初始化中断描述符表
-------------------------------------------------*/

void
_stdcall
KiInitInterrupt()
{
    //KiCheckCpuType();
    
    memset((pvoid)KIDT_BASE, 0, IDT_LENGTH);

    KiInitTrap();

    KiSetIdtr();
    
    KiInit8259A();
}
/*-----------------------------------------------*/



/*-------------------------------------------------
    初始化中断描述符表的前20项
    把异常处理地址按中断描述符表项中的定义填充
-------------------------------------------------*/

void
_stdcall
KiInitTrap()
{
    PI386_GATE  InterruptGate = (PI386_GATE)KIDT_BASE;
    pulong      TrapHandleTable = (pulong)TrapIdtStart;
    ulong       TrapHandle;
    int         i;

    // 按中断表项填充 IDT

    for (i = 0; i <= TRAP_IDT_COUNT; i++)
    {
        TrapHandle = TrapHandleTable[i];

        InterruptGate->OffsetLow = TrapHandle & 0xffff;
        InterruptGate->Selector = KGDT_R0_CODE;
        InterruptGate->Count = 0;
        InterruptGate->Attribute = INTERRUPT_GATE;
        InterruptGate->OffsetHigh = (TrapHandle >> 16) & 0xffff;

        InterruptGate++;
    }

    // 设置断点中断和溢出中断可在用户模式下触发

    InterruptGate = (PI386_GATE)KIDT_BASE;
    InterruptGate += IDT_BREAKPOINT;
    InterruptGate->Attribute |= USER_INTERRUPT;
    InterruptGate++;
    InterruptGate->Attribute |= USER_INTERRUPT;
}
/*-----------------------------------------------*/



/*-------------------------------------------------
    默认的中断处理例程
    只是简单地显示出错误信息及出错的地址等

    NOTE： 为了保护现场，这个函数不清理栈

    输入参数： 很明显
-------------------------------------------------*/

void
KiDefaultTrapHandle(
    long    Vector,
    long    ErrorCode,
    long    Eip,
    long    Cs,
    long    Eflags
    )
{
    int     i;
    int     TextColor = 0x74;

    char*   ErrorMsg[] = {  "#DE Devide Error",
                            "#DB Debug Break",
                            "--- NMI Interrupt",
                            "#BP Breakpoint",
                            "#OF Overflow",
                            "#BR Bound Range Exceeded",
                            "#UD Invalid Opcode",
                            "#NM Device Not Available",
                            "#DF Double Fault",
                            "--- Coprocessor Segment Overrun",
                            "#TS Invalid TSS",
                            "#NP Segment Not Present",
                            "#SS Stack Segment Fault",
                            "#GP General Protection",
                            "#PF Page Fault",
                            "--- (Intel Reserved)",
                            "#MF x87 FPU Floating Point Error",
                            "#AC Alignment Check",
                            "#MC Machine Check",
                            "#XF SIMD Floating Point Exception"
                          };

    CurrentDisplayPosition = 0;
    for (i = 0; i < 80 * 5; i++)
        DisplayString(" ");

    CurrentDisplayPosition = 0;
    DisplayColorString("Exception ---> ", TextColor);
    DisplayColorString(ErrorMsg[Vector], TextColor);
    DisplayColorString("\n\n", TextColor);

    DisplayColorString("EFLAGS = ", TextColor);
    DisplayInt(Eflags);
    DisplayColorString("CS = ", TextColor);
    DisplayInt(Cs);
    DisplayColorString("EIP = ", TextColor);
    DisplayInt(Eip);

    if (ErrorCode != 0xffffffff)
    {
        DisplayColorString("ErrorCode = ", TextColor);
        DisplayInt(ErrorCode);
    }
}
/*-----------------------------------------------*/



/*-------------------------------------------------
    第二阶段的初始化中断
-------------------------------------------------*/

void
_stdcall
KiInit8259A()
{
    HalWritePortChar(PORT_INTM_CTRL, 0x11);
    IoDelay();
    
    HalWritePortChar(PORT_INTS_CTRL, 0x11);
    IoDelay();
    
    //
    // IRQ0和IRQ8分别对应于中断向量INT_VECTOR_IRQ0和INT_VECTOR_IRQ8
    //
    
    HalWritePortChar(PORT_INTM_CTRLMASK, INT_VECTOR_IRQ0);
    IoDelay();
    
    HalWritePortChar(PORT_INTS_CTRLMASK, INT_VECTOR_IRQ8);
    IoDelay();
    
    HalWritePortChar(PORT_INTM_CTRLMASK, 0x4);  // IRQ2对应从8259
    IoDelay();
    
    HalWritePortChar(PORT_INTS_CTRLMASK, 0x2);  // 对应主8259的IRQ2
    IoDelay();
    
    HalWritePortChar(PORT_INTM_CTRLMASK, 0x1);
    IoDelay();
    
    HalWritePortChar(PORT_INTS_CTRLMASK, 0x1);
    IoDelay();
    
    //
    // 屏蔽主8259和从8259的所有中断
    //
    
    HalWritePortChar(PORT_INTM_CTRLMASK, 0xff);
    IoDelay();
    
    HalWritePortChar(PORT_INTS_CTRLMASK, 0xff);
    IoDelay();
}
/*-----------------------------------------------*/



/*-------------------------------------------------
    连接一个硬件中断
-------------------------------------------------*/

void
_stdcall
KeConnectInterrupt(
    ulong       IrqNumber,
    pvoid       ServiceRoutine,
    PKTHREAD    ServiceThread
    )
{
    PKINTERRUPT     Interrupt;
    PI386_GATE      InterruptGate = (PI386_GATE)KIDT_BASE;
    
    Interrupt = ExAllocatePool(sizeof(KINTERRUPT));
    if (Interrupt == 0)
    {
        KiBugCheck("KeConnectInterrupt: have no memory\n");
    }
    
    memset(Interrupt, 0, sizeof(KINTERRUPT));
    Interrupt->ServiceRoutine = ServiceRoutine;
    
    // 这里KiInterruptDispatch和下面的KiInterruptTemplate对汇编标号的引用需要取地址
    Interrupt->DispatchAddress = &KiInterruptDispatch;
    Interrupt->Vector = INT_VECTOR_IRQ0 + IrqNumber;
    Interrupt->Connected = TRUE;
    Interrupt->ServiceThread = ServiceThread;
    
    memcpy(Interrupt->DispatchCode, &KiInterruptTemplate, INTERRUPT_TEMPLATE_SIZE);
    
    InterruptGate += (INT_VECTOR_IRQ0 + IrqNumber);
    
    InterruptGate->OffsetLow = (ulong)(Interrupt->DispatchCode) & 0xffff;
    InterruptGate->Selector = KGDT_R0_CODE;
    InterruptGate->Count = 0;
    InterruptGate->Attribute = INTERRUPT_GATE;
    InterruptGate->OffsetHigh = ((ulong)(Interrupt->DispatchCode) >> 16) & 0xffff;
    
    HalEnableIrq(IrqNumber);
}
    
/*-----------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/

void
_stdcall
IoWaitInterrupt()
{
    MESSAGE     Msg;
    
    KeSendRecvMsg(RECEIVE, INTERRUPT, &Msg);
}
/*-----------------------------------------------------------*/












