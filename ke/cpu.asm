
;-------------------------------------------------------------
; CPU 初始化及相关例程
;
; 修改时间列表:
; 2011-08-09 22:43:08 
;-------------------------------------------------------------


%include    "ke.inc"


global  KiCheckCpuType





;-------------------------------------------
;-------------------------------------------
;-------------------------------------------



;-------------------------------------------
; 检测CPU类型，假定至少为386
;-------------------------------------------
KiCheckCpuType:
        pushfd
        pop     eax
        mov     ecx, eax
        xor     eax, 40000h     ; flip AC bit in EFLAGS
        push    eax
        popfd
        pushfd
        pop     eax
        xor     eax, ecx
        and     eax, 40000h
        je      _is386          ; 不能设置 AC位说明是386
        
        ;
        ; 到这里至少为486
        ;
        
        mov     eax, ecx
        xor     eax, 200000h    ; ID bit
        push    eax
        popfd
        pushfd
        pop     eax
        xor     eax, ecx
        push    ecx
        popfd
        and     eax, 200000h
        je      _is486
        
        ;
        ; 到这里至少为奔腾
        ;
        
        xor     eax, eax
        cpuid
        mov     [KiCpuId], eax
        mov     esi, KiCpuVendor
        mov     [esi], ebx
        mov     [esi + 4], edx
        mov     [esi + 8], ecx
        
        or      eax, eax
        je      _is486
        
        mov     eax, 1
        cpuid
        mov     ecx, eax
        and     ah, 0fh
        mov     Family, ah
        and     ch, 30h
        shr     ch, 4
        mov     ProcessorType, ch
        and     al, 0f0h
        shr     al, 4
        mov     Model, al
        and     cl, 0fh
        mov     SteppingID, cl
        mov     Feature, edx
        
    _is486:
        mov     eax, cr0
        and     eax, 80000011h      ; save  PG, PE, ET
        or      eax, 50022h         ; set   AM, WP, NE MP
        jmp     _check_x87
        
    _is386:
        push    ecx
        popfd
        mov     eax, cr0
        and     eax, 80000011h      ; save  PG, PE, ET
        or      eax, 2              ; set   MP
        
    _check_x87:
        mov     cr0, eax
        call    KiCpuCheckX87
        
; KiCheckCpuType endp
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
        mov     [ecx + 4], eax
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


