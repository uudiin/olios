
;------------------------------------------------------------
; 用于LOADER和内核之间共用的定义
;
; 最后修改日期：
; 2011-01-04 22:04:29 
;------------------------------------------------------------



;
; 系统段，包括如下：
;
; 表         范围        长度
; GDT   90000 - 903ff   400
; IDT   90400 - 90bff   800
; TSS   90c00 - 90fff   400
; LPB   91000 - 91fff   1000        ; 传给内核的参数块
; ARD   92000 - 92400   400         ; 内存地址范围描述符数组
; PD    93000 - 93fff   1000        ; 初始页目录
; PT    94000 - 97fff   4000        ; 页表，共四页
;

SYS_DESC_SEG        equ     9000h
GDT_BASE            equ     90000h
GDT_LENGTH          equ     3ffh
IDT_BASE            equ     90400h
IDT_LENGTH          equ     7ffh
TSS_BASE            equ     90c00h
LPB_SEG             equ     9100h
LPB_BASE            equ     91000h
ARD_SEG             equ     9200h
ARD_BASE            equ     92000h
PAGE_DIR_BASE       equ     93000h
PAGE_TABLE_BASE     equ     94000h
INITIALIZE_PT_COUNT equ     4           ; 最初初始化 4个对等页表


MINI_MEMORY         equ     1000000h    ; 内存至少16M
KRNL_PBYSICAL_BASE  equ     100000h     ; 内核加载的地址(物理地址)
KSEG0_BASE          equ     80000000h   ; 内核模式与用户模式分界地址


;--------------------------------------------------------
; 保护模式下的宏定义
;--------------------------------------------------------

PG_P        equ     1   ; 页存在属性位
PG_RWR      equ     0   ; R/W 属性位值, 读/执行
PG_RWW      equ     2   ; R/W 属性位值, 读/写/执行
PG_USS      equ     0   ; U/S 属性位值, 系统级
PG_USU      equ     4   ; U/S 属性位值, 用户级


; Base,Limit,Attribute

%macro DESCRIPTOR 3
        dw  %2 & 0ffffh                 ; 偏移低16位
        dw  %1 & 0ffffh                 ; 基地址低16位
        db  (%1 >> 16) & 0ffh           ; 基地址17～24位
        dw  ((%2 >> 8) & 0f00h) | (%3 & 0f0ffh)
        db  (%1 >> 24) & 0ffh           ; 基地址高8位
%endmacro


; Selector,Offset,DCount,Attribute

%macro GATE 4
        dw  %2 & 0ffffh
        dw  %1
        db  %3 & 1fh
        db  %4
        dw  ((%2 >> 16) & 0ffffh)
%endmacro


; InterruptHandle, DPL

%macro IDTENTRY 2
        dw  (%1 & 0ffffh)
        dw  SelectorCodeR0
        db  0
        db  (8eh | ((%2 & 3) << 5))
        dw  ((%1 >> 16) & 0ffffh)
%endmacro


