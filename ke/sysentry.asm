
;------------------------------------------------------------------
; 内核模块入口
;
; 编译及连接方法
; nasm -f elf kernel.asm -o kernel.o
; ld -s -Ttext-segment 0x80100000 kernel.o -o zhoskrnl
;
; 最后修改时间：
; 2011-01-04 23:31:47 
; 2011-01-09 22:00:42 
; 2011-01-19 22:57:17 
; 2011-01-21 00:31:14 
; 2011-01-23 16:44:30 
; 2011-07-30 00:29:46 
;------------------------------------------------------------------



%include    "ke.inc"


extern  DisplayColorString
extern  DisplayString
extern  DisplayInt
extern  KiInitInterrupt
extern  IoPciEnum
extern  MmInitSystem
extern  PsInitSystem
extern  HalInitClock
extern  ret_to_thread
extern  KiIdleThread
extern  KeSetThreadPriority


[section .text]
[bits 32]

global  _start

global  KiIdleLoop



        ;
        ; 内核入口
        ;

    _start:
        cld
        xor     eax, eax
        mov     ax, KGDT_R3_DATA
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     ax, KGDT_R0_DATA
        mov     ss, ax
        mov     ax, KGDT_VIDEO
        mov     gs, ax

        ; 先把页目录和页表清零

        mov     edi, PAGE_DIR_BASE
        xor     eax, eax
        mov     ecx, INITIALIZE_PT_COUNT * 1000h + 1000h
        rep stosb

        ; 填充低地址在页目录中的映射

        mov     edi, PAGE_DIR_BASE
        mov     eax, PAGE_TABLE_BASE | 3
        mov     ecx, INITIALIZE_PT_COUNT

    _fill_user_pde:
        stosd
        add     eax, 1000h
        dec     ecx
        jnz     _fill_user_pde

        ; 填充内核地址在页目录中的映射

        mov     edi, PAGE_DIR_BASE + (KSEG0_BASE >> 20)
        mov     eax, PAGE_TABLE_BASE | 3
        mov     ecx, INITIALIZE_PT_COUNT

    _fill_kernel_pde:
        stosd
        add     eax, 1000h
        dec     ecx
        jnz     _fill_kernel_pde

        ; 填充系统空间和用户空间共用的页表

        mov     edi, PAGE_TABLE_BASE
        mov     eax, 3      ; 读/写/执行 存在

    _fill_pte:
        stosd
        add     eax, 1000h
        cmp     edi, PAGE_TABLE_BASE + INITIALIZE_PT_COUNT * 1000h
        jnz     _fill_pte

        ;
        ; 把页表以数组形式映射到虚拟地址空间,也就是自映射方案
        ; 这样就可以不用物理地址来访问页目录表和页表
        ;

        mov     eax, PAGE_DIR_BASE + (PAGE_TABLE >> 20)
        mov     dword [eax], PAGE_DIR_BASE | 3

        ;
        ; 打开分页机制,然后跳转到系统空间地址执行
        ;

        mov     eax, PAGE_DIR_BASE
        mov     cr3, eax

        mov     eax, cr0
        or      eax, 80000000h
        mov     cr0, eax
        jmp     _flush_inc      ; 刷掉CPU的预取队列

        ; 转入系统空间执行

    _flush_inc:
        mov     eax, _sys_addr_start
        jmp     eax


        ;
        ; 真正运行在系统地址空间的入口
        ;

    _sys_addr_start:

        push    13
        push    szKernelMsg
        call    DisplayColorString

        mov     dword [LoaderParameterBlock], LPB_BASE | KSEG0_BASE

        ;
        ; 重新定位GDTR到高地址,并把TSS和视频缓冲区定位到高内存区
        ;

        push    eax
        push    eax
        sgdt    [esp + 2]
        or      dword [esp + 4], KSEG0_BASE
        
        mov     ebx, [esp + 4]
        lea     eax, [ebx + KGDT_TSS]
        or      byte [eax + 7], KSEG0_BASE >> 24
        
        ; KGDT_VIDEO = 2bh
        lea     eax, [ebx + KGDT_VIDEO - 3]
        or      byte [eax + 7], KSEG0_BASE >> 24
        
        lgdt    [esp + 2]
        
        xor     eax, eax
        mov     eax, KGDT_R3_DATA
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     eax, KGDT_R0_DATA
        mov     ss, ax
        mov     eax, KGDT_VIDEO
        mov     gs, ax
        
        mov     eax, KGDT_TSS
        ltr     ax

        mov     eax, KTSS_BASE
        mov     [KiTssBase], eax
        

        ; 下面这个跳转强制使用刚加载的GDT

        jmp     KGDT_R0_CODE:_banlance_stack

    _banlance_stack:
        pop     eax
        pop     eax

        ; 把栈也定位到高内存区

        or      esp, KSEG0_BASE

        ; 初始化中断,IDT,CPU类型及8259A芯片

        call    KiInitInterrupt
        
        ; PCI设备枚举
        
        call    IoPciEnum

        ; 初始化内存管理

        push    dword [LoaderParameterBlock]
        call    MmInitSystem
        
        ;
        ; 初始化进程管理
        ;
        
        call    PsInitSystem
        
        ;;
        ;; 设置当前线程为空闲线程
        ;;
        
        mov     eax, [PsIdleThread]
        mov     [PsCurrentThread], eax
        
        ;
        ; 初始化硬件时钟中断
        ;
        
        call    HalInitClock

        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        push    13
        push    szPreSched
        call    DisplayColorString
        
        ;jmp     $
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        
        ;
        ; 转入空闲线程执行
        ;
        
        jmp     ret_to_thread
        
        ud2


;----------------------------------------------------------
; 空闲循环线程
; 一个参数，未使用
;----------------------------------------------------------

KiIdleLoop:
        ; 把空闲线程的优先级设为0，最低
        push    0
        push    dword [PsCurrentThread]
        call    KeSetThreadPriority
        
    _idle_loop:
        nop
        push    dword [esp + 4]
        call    KiIdleThread
        nop
        pause
        nop
        jmp     _idle_loop
        
        ret     4
; KiIdleLoop endp
;----------------------------------------------------------


    _pause_machine:
        pause
        jmp     _pause_machine



;-----------------------------------------------------------------------

        align   4

szKernelMsg     db  'there is the zhoskrnl in system memory', 0ah, 0
szPreSched      db  'before schedule thread', 0ah, 0
szPageTable     db  'page_table and self map is : ', 0
;-----------------------------------------------------------------------



