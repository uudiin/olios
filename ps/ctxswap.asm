
;-------------------------------------------------------------
; 线程切换相关
;
; 修改时间列表:
; 2011-02-26 00:20:41 
; 2011-08-16 00:05:30 
;-------------------------------------------------------------


%include    "ke.inc"


extern  DisplayInt


[section .text]

global  KiSwapThread
global  KiSwapContext


;---------------------------------------------------------
;---------------------------------------------------------
;---------------------------------------------------------




;---------------------------------------------------------
; 完成线程切换，原型如下：
;
;   void __stdcall
;   KiSwapThread(
;       PKTHREAD    OldThread,
;       PKTHREAD    NewThread
;       );
;
; 线程切换返回后，已经处于另一个线程的环境
; 若两个线程所在进程不同，则切换到新进程的地址空间
;---------------------------------------------------------

KiSwapThread:
        pushad
        sub     esp, 8
        pushfd
        xor     eax, eax
        mov     ax, cs
        push    eax
        
        mov     esi, [esp + 34h]
        mov     edi, [esp + 38h]
        call    KiSwapContext
        
        add     esp, 8
        popad
        ret     8


        ;pushfd      ; 占 KTRAP_FRAME中ErrCode的位置
        ;pushad
        ;mov     esi, [esp + 24h]
        ;mov     edi, [esp + 28h]
        ;call    KiSwapContext
        ;popad
        ;popfd
        ;ret     8
; KiSwapThread endp
;---------------------------------------------------------



;---------------------------------------------------------
; 完成线程环境切换
;
; 参数：
;   esi -> OldThread
;   edi -> NewThread
;---------------------------------------------------------

KiSwapContext:

        cli

        ; ErrorCode 占位
        
        push    eax
        
        pushad
        push    ds
        push    es
        push    fs
        push    gs
        sub     esp, 14h
        push    dword [esi + KTHREAD_KTRAP_FRAME]

        ; 保存原来线程的栈，恢复新线程的栈
        
        mov     [esi + KTHREAD_KERNEL_STACK], esp
        mov     esp, [edi + KTHREAD_KERNEL_STACK]
        
        ; 判断是否需要切换进程
        
        mov     ebp, [edi + KTHREAD_PROCESS]
        mov     eax, [esi + KTHREAD_PROCESS]
        cmp     ebp, eax
        jz      _same_process
        
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        push    eax
        call    DisplayInt
        push    ebp
        call    DisplayInt
        push    esi
        call    DisplayInt
        push    edi
        call    DisplayInt
        jmp     $
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        
        mov     eax, [ebp + KPROCESS_DIRECTORY_TABLE_BASE]
        mov     cr3, eax
        
    _same_process:
        
        ;mov     eax, [KiTssBase]
        ;mov     ecx, [edi + KTHREAD_KERNEL_STACK]
        ;mov     [eax + TSS_ESP0], ecx
        
        pop     dword [edi + KTHREAD_KTRAP_FRAME]
        add     esp, 14h
        pop     gs
        pop     fs
        pop     es
        pop     ds
        popad
        add     esp, 4
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        ; 这里由IDLE切换到A，A第一次运行，栈里的数据可能不对还有栈空间可能不够
        ; // FIXME KernelStack
        push    dword [esp]
        call    DisplayInt
        push    dword [esp + 4]
        call    DisplayInt
        push    dword [esp + 8]
        call    DisplayInt
        push    dword [esp + 0ch]
        call    DisplayInt
        jmp     $
        ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        iret
        
        ;xor     eax, eax
        ;ret
; KiSwapContext endp
;---------------------------------------------------------























