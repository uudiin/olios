

;----------------------------------------------------------
; 内核定义头文件
;
; NOTE: 这个文件中的定义要与 ke.h　文件中的定义同步
;
; 最后修改日期：
; 2011-01-04 22:13:28 
; 2011-01-17 20:19:44 
; 2011-01-21 00:15:15 
; 2011-08-16 00:26:14 
;----------------------------------------------------------


%include    "common.inc"


KGDT_R0_CODE        equ     8h
KGDT_R0_DATA        equ     10h
KGDT_R3_CODE        equ     (18h + 3)   ; Ring3可访问
KGDT_R3_DATA        equ     (20h + 3)
KGDT_VIDEO          equ     (28h + 3)
KGDT_TSS            equ     30h



PAGE_TABLE          equ     0c0000000h
KIDT_BASE           equ     (IDT_BASE | KSEG0_BASE)
KTSS_BASE           equ     (TSS_BASE | KSEG0_BASE)

KERNEL_MODE         equ     0
USER_MODE           equ     3


;
; 下面的定义是相应的域在结构中的偏移,必须与结构定义相对应
;

KINTERRUPT_OBJ_SIZE     equ     (4 * 7)     ; KINTERRUPT中DispatchCode中前面的大小
KINTERRUPT_DISPATCH     equ     (4 * 4)     ; DispatchAddress的偏移
KINTERRUPT_SERVICE      equ     (4 * 3)     ; ServiceRoutine的偏移
KINTERRUPT_VECTOR       equ     (4 * 5)     ; Vector, one byte

KTHREAD_KTRAP_FRAME     equ     (4 * 4)
KTHREAD_PROCESS         equ     (4 * 5)
KTHREAD_KERNEL_STACK    equ     (4 * 10)
KTHREAD_PREVIOUS_MODE   equ     (4 * 14)
;KTHREAD_INTERRUPTED_STACK   equ (4 * 21)

KPROCESS_DIRECTORY_TABLE_BASE   equ     (4 * 10)

KTRAP_FRAME_NEXT        equ     0
KTRAP_FRAME_CS          equ     (4 * 20)

TSS_ESP0                equ     (4 * 1)



;
; 中断的相关定义
;

EOI                     equ     20h
INT_VECTOR_IRQ0         equ     30h
PORT_INTM_CTRL          equ     20h
PORT_INTM_CTRLMASK      equ     21h
PORT_INTS_CTRL          equ     0a0h
PORT_INTS_CTRLMASK      equ     0a1h



;
; 汇编文件中导入的全局变量
;

extern  CurrentDisplayPosition
extern  LoaderParameterBlock
extern  KiTssBase
extern  KiInterruptReEnter
extern  KiDefaultTrapHandle
extern  MmAccessFault
extern  PsCurrentThread
extern  PsIdleThread




