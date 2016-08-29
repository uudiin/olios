
;----------------------------------------------------------
; 支持函数库
;
; 最后修改日期：
; 2011-01-04
; 2011-01-14 22:36:53 
; 2011-01-16 23:21:21 
;----------------------------------------------------------


%include    "ke.inc"


[section .text]

global  HalDisableIrq
global  HalEnableIrq

global  HalReadPortChar
global  HalWritePortChar
global  HalReadPortWord
global  HalWritePortWord
global  HalReadPortDword
global  HalWritePortDword

global  HalReadPort
global  HalWritePort
global  KiAcquireSpinLock
global  KiReleaseSpinLock
global  InterlockedPushEntrySList
global  InterlockedPopEntrySlist
global  memcpy
global  memset

;------------------------------------
;------------------------------------
;------------------------------------




;------------------------------------
; 屏蔽指定的IRQ中断
; 参数：
; 1. IRQ (不是中断向量)
;------------------------------------

HalDisableIrq:
        mov     ecx, [esp + 4]
        pushfd
        cli
        mov     ah, 1
        rol     ah, cl
        cmp     cl, 8
        jge     _disable_8
        
    _disable_0:
        in      al, PORT_INTM_CTRLMASK
        or      al, ah
        out     PORT_INTM_CTRLMASK, al
        jmp     _end_disable
        
    _disable_8:
        in      al, PORT_INTS_CTRLMASK
        or      al, ah
        out     PORT_INTS_CTRLMASK, al
        
    _end_disable:
        popfd
        ret     4
; HalDisableIrq endp
;------------------------------------



;------------------------------------
; 打开指定的IRQ的中断
; 参数：
; 1. IRQ (不是中断向量)
;------------------------------------

HalEnableIrq:
        mov     ecx, [esp + 4]
        pushfd
        cli
        mov     ah, ~1
        rol     ah, cl
        cmp     cl, 8
        jge     _enable_8
        
    _enable_0:
        in      al, PORT_INTM_CTRLMASK
        and     al, ah
        out     PORT_INTM_CTRLMASK, al
        jmp     _end_enable
        
    _enable_8:
        in      al, PORT_INTS_CTRLMASK
        and     al, ah
        out     PORT_INTS_CTRLMASK, al
        
    _end_enable:
        popfd
        ret     4
; HalEnableIrq endp
;------------------------------------



;--------------------------------
; 读端口一个字节
; 参数：
; 1. PORT
;
; 返回值：读出的值
;--------------------------------
HalReadPortChar:
        mov     edx, [esp + 4]  ; PORT
        xor     eax, eax
        in      al, dx
        nop
        ret     4
; HalReadPortChar endp
;--------------------------------



;--------------------------------
; 写端口一个字节
; 参数：
; 1. PORT
; 2. 要写入的值
;--------------------------------

HalWritePortChar:
        mov     edx, [esp + 4]  ; PORT
        mov     eax, [esp + 8]  ; Value
        out     dx, al
        nop
        ret     8
; HalWritePortChar endp
;--------------------------------



;--------------------------------
; 读端口一个字
; 参数：
; 1. PORT
;
; 返回值：读出的值
;--------------------------------
HalReadPortWord:
        mov     edx, [esp + 4]  ; PORT
        xor     eax, eax
        in      ax, dx
        nop
        ret     4
; HalReadPortWord endp
;--------------------------------



;--------------------------------
; 写端口一个字
; 参数：
; 1. PORT
; 2. 要写入的值
;--------------------------------

HalWritePortWord:
        mov     edx, [esp + 4]  ; PORT
        mov     eax, [esp + 8]  ; Value
        out     dx, ax
        nop
        ret     8
; HalWritePortWord endp
;--------------------------------



;--------------------------------
; 读端口一个双字
; 参数：
; 1. PORT
;
; 返回值：读出的值
;--------------------------------
HalReadPortDword:
        mov     edx, [esp + 4]  ; PORT
        xor     eax, eax
        in      eax, dx
        nop
        ret     4
; HalReadPortDword endp
;--------------------------------



;--------------------------------
; 写端口一个双字
; 参数：
; 1. PORT
; 2. 要写入的值
;--------------------------------

HalWritePortDword:
        mov     edx, [esp + 4]  ; PORT
        mov     eax, [esp + 8]  ; Value
        out     dx, eax
        nop
        ret     8
; HalWritePortDword endp
;--------------------------------



;--------------------------------
; 读端口n个字节
; 参数：
; 1. PORT
; 2. 缓冲区
; 3. 读的字节数, 以2对齐
;--------------------------------

HalReadPort:
        push    edi
    
        mov     edx, [esp + 8]      ; port
        mov     edi, [esp + 0ch]    ; buf
        mov     ecx, [esp + 10h]    ; number
        shr     ecx, 1
        cld
        rep insw
    
        pop     edi
        ret     0ch
; HalReadPort endp
;--------------------------------



;--------------------------------
; 写端口n个字节
; 参数：
; 1. PORT
; 2. 缓冲区
; 3. 写的字节数, 2的倍数
;--------------------------------

HalWritePort:
        push    esi

        mov     edx, [esp + 8]
        mov     esi, [esp + 0ch]
        mov     ecx, [esp + 10h]
        shr     ecx, 1
        cld
        rep outsw

        pop     esi
        ret     0ch
; HalWritePort endp
;--------------------------------



;------------------------------------
; 获取自旋锁
;------------------------------------

KiAcquireSpinLock:
        mov     ecx, [esp + 4]
        
    _asl10:
        lock bts  dword [ecx], 0
        jc      _asl20
        ret     4
        
    _asl20:
        test    dword [ecx], 1
        jz      _asl10
        pause
        jmp     _asl20
; KiAcquireSpinLock endp
;------------------------------------



;------------------------------------
; 释放自旋锁
;------------------------------------

KiReleaseSpinLock:
        mov     ecx, [esp + 4]
        lock and  byte [ecx], 0
        ret     4
; KiReleaseSpinLock endp
;------------------------------------



;------------------------------------
; 插入单链表
; 输入参数：
;   ListHead    链表头
;   ListEntry   链表节点
; 返回：   链表头之前的内容
;------------------------------------

InterlockedPushEntrySList:
        push    ebx
        push    ebp
        
        mov     ebp, [esp + 0ch]    ; ListHead
        mov     ebx, [esp + 10h]    ; ListEntry
        
        mov     edx, [ebp + 4]
        mov     eax, [ebp]
        
    _epsh10:
        mov     [ebx], eax
        lea     ecx, [edx + 10001h]
        
        ; lock cmpxchg8b  qword [ebp]
        
        db      0f0h, 00fh, 0c7h, 04dh, 000h
        
        jnz     _epsh10
        
        pop     ebp
        pop     ebx
        ret     8
; InterlockedPushEntrySList endp
;------------------------------------



;------------------------------------
; 移除单链表
; 输入参数：
;    ListHead    单链表头地址
; 返回：   从链表中移出的 entry
;------------------------------------

InterlockedPopEntrySList:
        push    ebx
        push    ebp

        mov     ebp, [esp + 0ch]

    _epop10:
        mov     edx, [ebp + 4]
        mov     eax, [ebp]
        or      eax, eax        ; 判断链表是否为空
        jz      _epop20
        lea     ecx, [edx - 1]
        mov     ebx, [eax]
        
        ; lock cmpxchg8b  qword [ebp]
        
        db      0f0h, 00fh, 0c7h, 04dh, 000h
        
        jnz     _epop10
        
    _epop20:
        pop     ebp
        pop     ebx
        ret     4
; InterlockedPopEntrySList endp
;------------------------------------



;------------------------------------
; memcpy(dst, src, size);
; stdcall调用方式
;------------------------------------

memcpy:
        push    ebp
        mov     ebp, esp
        pushad
        mov     esi, [ebp + 0ch]
        mov     edi, [ebp + 8]
        mov     ecx, [ebp + 10h]
        rep movsb
        popad
        mov     esp, ebp
        pop     ebp
        ret     0ch
; memcpy endp
;------------------------------------



;------------------------------------
; memset(dst, val, size);
; stdcall调用方式
;------------------------------------

memset:
        push    ebp
        mov     ebp, esp
        pushad
        mov     edi, [ebp + 8]
        mov     eax, [ebp + 0ch]
        mov     ecx, [ebp + 10h]
        rep stosb
        popad
        mov     esp, ebp
        pop     ebp
        ret     0ch
; memset endp
;------------------------------------



