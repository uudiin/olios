
;-------------------------------------------------------------
; intel 指定的中断的默认异常处理函数定义
; 只是简单调用KiDefaultTrapHandle显示异常信息，并不做出处理
;
; 修改时间列表:
; 2011-01-04 22:58:47 
; 2011-01-05 22:52:56 
; 2011-01-09 00:05:22 
; 2011-01-16 23:21:35 
; 2011-01-17 20:19:00 
; 2011-01-22 02:31:55 
; 2011-08-16 00:26:49 
;-------------------------------------------------------------


%include    "ke.inc"


global  TrapIdtStart
global  KiSetIdtr
global  ret_to_thread
global  IoDelay
global  KiInterruptTemplate
global  KiInterruptDispatch



;---------------------------------------------
; 当一个硬件中断发生时，暂时屏蔽当前的IRQ中断
; edi -> KINTERRUPT
;---------------------------------------------

%macro  DISABLE_CURRENT_IRQ 0
        mov     ah, 1
        mov     cl, [edi + KINTERRUPT_VECTOR]
        sub     cl, INT_VECTOR_IRQ0
        rol     ah, cl
        cmp     cl, 8
        jge     _dis8259s
        
    _dis8259m:
        in      al, PORT_INTM_CTRLMASK
        or      al, ah
        out     PORT_INTM_CTRLMASK, al
        jmp     _end_dis
        
    _dis8259s:
        in      al, PORT_INTS_CTRLMASK
        or      al, ah
        out     PORT_INTS_CTRLMASK, al
        mov     al, EOI
        out     PORT_INTS_CTRL, al
        nop
        
    _end_dis:
        mov     al, EOI
        out     PORT_INTM_CTRL, al
        nop
%endmacro
;---------------------------------------------



;---------------------------------------------
; 打开当前的IRQ硬件中断
; edi -> KINTERRUPT
;---------------------------------------------

%macro  ENABLE_CURRENT_IRQ  0
        mov     ah, ~1
        mov     cl, [edi + KINTERRUPT_VECTOR]
        sub     cl, INT_VECTOR_IRQ0
        rol     ah, cl
        cmp     cl, 8
        jge     _en8259s
        
    _en8259m:
        in      al, PORT_INTM_CTRLMASK
        and     al, ah
        out     PORT_INTM_CTRLMASK, al
        jmp     _end_enable
        
    _en8259s:
        in      al, PORT_INTS_CTRLMASK
        and     al, ah
        out     PORT_INTS_CTRLMASK, al
        nop
        
    _end_enable:
%endmacro
;---------------------------------------------



;-------------------------------------------
;-------------------------------------------

KiInterruptTemplate:
        push    0       ; ErrorCode占位
        pushad
        push    ds
        push    es
        push    fs
        push    gs
        
        sub     esp, 18h    ; 为KTRAP_FRAME保留前面的保留域
        
        call    _self_locate
        
    _self_locate:
        pop     edi
        sub     edi, KINTERRUPT_OBJ_SIZE + (_self_locate - KiInterruptTemplate)
        jmp     [edi + KINTERRUPT_DISPATCH]
        times   48  db 90h
; KiInterruptTemplate endp
;-------------------------------------------



;-------------------------------------------
;-------------------------------------------
;-------------------------------------------



;--------------------------------------------------
; 硬件中断分发函数stub
; // FIXME
; 目前只适用于处理内核系统线程
;--------------------------------------------------

KiInterruptDispatch:
        inc     dword [KiInterruptReEnter]
        cmp     dword [KiInterruptReEnter], 0
        jnz     _reenter
        
        mov     esi, [PsCurrentThread]
        
        mov     eax, [esi + KTHREAD_KTRAP_FRAME]
        
        ; 保存上次的KTRAP_FRAME到当前的NextFrame
        
        mov     [esp + KTRAP_FRAME_NEXT], eax
        
        mov     eax, esp
        mov     [esi + KTHREAD_KTRAP_FRAME], eax
        
        ; 保存进入中断前线程所处的特权级
        
        mov     eax, [eax + KTRAP_FRAME_CS]
        and     eax, 3
        mov     [esi + KTHREAD_PREVIOUS_MODE], eax
        cmp     eax, KERNEL_MODE
        jz      _ke_int
        
        ; 从用户模式进入的中断，切换栈
        
        mov     eax, [esi + KTHREAD_KERNEL_STACK]
        mov     esp, eax        ; // FIXME
        jmp     _enter_service
        
    _ke_int:
    
    _enter_service:
    
        DISABLE_CURRENT_IRQ
        
        ; 置EOI，可以重新接收中断
        
        mov     al, EOI
        out     PORT_INTM_CTRL, al
        nop
        
        sti
        
        push    edi     ; edi -> KINTERRUPT
        call    [edi + KINTERRUPT_SERVICE]
        
        cli
        
        ENABLE_CURRENT_IRQ
        
    ret_to_thread:
    
        mov     edi, [PsCurrentThread]
        mov     eax, [edi + KTHREAD_KTRAP_FRAME]
        
        mov     eax, [eax + KTRAP_FRAME_CS]
        and     eax, 3
        cmp     eax, KERNEL_MODE
        jz      _ke_int_ret
        
        ; 返回到用户模式时把栈保存到TSS中
        
        mov     eax, [edi + KTHREAD_KERNEL_STACK]
        mov     ecx, [KiTssBase]
        mov     [ecx + TSS_ESP0], eax
        jmp     _reenter
        
    _ke_int_ret:
        mov     eax, [edi + KTHREAD_KTRAP_FRAME]
        
        ; 把KTRAP_FRAME的NextFrame指针保存到线程中
        
        mov     ecx, [eax + KTRAP_FRAME_NEXT]
        mov     [edi + KTHREAD_KTRAP_FRAME], ecx
        
        mov     esp, eax    ; 内核线程栈
        add     esp, 18h
        
    _reenter:
    
        dec     dword [KiInterruptReEnter]
        pop     gs
        pop     fs
        pop     es
        pop     ds
        popad
        add     esp, 4
        iretd
; KiInterruptDispatch endp
;--------------------------------------------------





;--------------------------------------
; IDT中前32项异常入口宏
; 这些异常处理函数在压入错误码后都应该调用该宏
; 保存寄存器到栈中，形成 KTRAP_FRAME结构
; 输出:  ebp -> KTRAP_FRAME
;--------------------------------------

%macro  TRAP_ENTER  0
        mov     word [esp + 2], 0   ; ErrCode高位清0
        pushad
        push    ds
        push    es
        push    fs
        push    gs
        
        sub     esp, 18h
        
        mov     ax, ss
        mov     ds, ax
        mov     es, ax
        mov     ebp, esp
%endmacro
;--------------------------------------



;----------------------------------------
; IDT中前32项异常处理完毕宏
; 这些异常处理函数在处理完异常后应该调用该宏
; 从栈中的 KTRAP_FRAME结构中弹出各寄存器的值
; 输出:  ebp -> KTRAP_FRAME
;----------------------------------------

%macro  TRAP_LEAVE  0
        add     esp, 18h
        pop     gs
        pop     fs
        pop     es
        pop     ds
        popad
        iretd
%endmacro
;----------------------------------------



;
; 这个表用于初始化IDT
;

    align   4

TrapIdtStart    dd      $ + 4
dd      Trap00DivideError
dd      Trap01DebugBreak
dd      Trap02Nmi
dd      Trap03BreakPoint
dd      Trap04Overflow
dd      Trap05BoundCheck
dd      Trap06InvalidOpcode
dd      Trap07CoprocessorNotAvailable
dd      Trap08DoubleFault
dd      Trap09CoprocessorSegOverrun
dd      Trap0aInvalidTss
dd      Trap0bSegmentNotPresent
dd      Trap0cStackException
dd      Trap0dGeneralProtection
dd      Trap0ePageFault
dd      0
dd      Trap10CoprocessorError
dd      Trap11AlignCheck
dd      Trap12MachineCheck
dd      Trap13SimdTrap
times   31 - 19     dd  0
dd      KiSystemService
TRAP_IDT_LENGTH     equ     $ - TrapIdtStart



;-------------------------------------------------
; 设置IDTR寄存器
;-------------------------------------------------

KiSetIdtr:
    push    KIDT_BASE
    push    IDT_LENGTH << 16
    lidt    [esp + 2]
    pop     eax
    pop     eax
    ret
; KiSetIdtr endp
;-------------------------------------------------



;-------------------------------------------------
; IDT中前20项的默认异常处理例程
; 这里压入中断号和错误码(有错误码的不压)后
; 都调用 KiDefaultTrapHandle 来显示异常信息
;-------------------------------------------------
;                               error code

Trap00DivideError:
        push    0ffffffffh
        push    0
        jmp     _exception

Trap01DebugBreak:
        push    0ffffffffh
        push    1
        jmp     _exception

Trap02Nmi:
        push    0ffffffffh
        push    2
        jmp     _exception

Trap03BreakPoint:
        push    0ffffffffh
        push    3
        jmp     _exception

Trap04Overflow:
        push    0ffffffffh
        push    4
        jmp     _exception

Trap05BoundCheck:
        push    0ffffffffh
        push    5
        jmp     _exception

Trap06InvalidOpcode:
        push    0ffffffffh
        push    6
        jmp     _exception

Trap07CoprocessorNotAvailable:
        push    0ffffffffh
        push    7
        jmp     _exception

Trap08DoubleFault:              ; **
        push    8
        jmp     _exception

Trap09CoprocessorSegOverrun:
        push    0ffffffffh
        push    9
        jmp     _exception

Trap0aInvalidTss:               ; **
        push    0ah
        jmp     _exception

Trap0bSegmentNotPresent:        ; **
        push    0bh
        jmp     _exception

Trap0cStackException:           ; **
        push    0ch
        jmp     _exception

Trap0dGeneralProtection:        ; **
        push    0dh
        jmp     _exception

Trap0ePageFault:                ; **
        TRAP_ENTER
        mov     edi, cr2
        push    ebp
        mov     eax, [ebp + 50h]    ; SegCs, previous mode
        push    eax
        push    edi
        mov     eax, [ebp + 48h]    ; ErrorCode
        push    eax
        call    MmAccessFault
        jmp     _halt_machine

Trap10CoprocessorError:
        push    0ffffffffh
        push    10h
        jmp     _exception

Trap11AlignCheck:               ; **
        push    11h
        jmp     _exception

Trap12MachineCheck:
        push    0ffffffffh
        push    12h
        jmp     _exception

Trap13SimdTrap:
        push    0ffffffffh
        push    13h
        jmp     _exception


    _exception:
        call    KiDefaultTrapHandle
        add     esp, 8

    _halt_machine:
        hlt
        jmp     _halt_machine


;------------------------------------------------

;--------------------------
; IO操作延迟
;--------------------------
IoDelay:
        nop
        nop
        nop
        ret
; IoDelay endp
;--------------------------

;------------------------------------------------

KiSystemService:
        nop
        nop





















