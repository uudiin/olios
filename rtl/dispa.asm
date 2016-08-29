
;----------------------------------------------------------
; 显示支持函数
;
; 最后修改日期：
; 2011-01-03
;----------------------------------------------------------


%include    "ke.inc"


[section .text]

global  DisplayInt
global  DisplayColorString
global  DisplayString
global  DisplayReturn
global  DisplayStackPtr


;---------------------------------------------------
; 在屏幕当前位置显示一位的十六进制数字
;
; 采用栈传递参数，stdcall 调用方式
;
; 输入参数：
; 1.    要显示的整数，只使用低四位，来显示一位的十六进制
;---------------------------------------------------

DisplayOneHex:
        push    ebp
        mov     ebp, esp
        pushad

        mov     eax, [ebp + 8]
        and     eax, 0fh

        ; > 9, 显示 A - F

        cmp     al, 9
        ja      _disp_hex

        ; 得到相对于字符‘0’的ASCII码

        add     al, '0'
        jmp     _can_disp

    _disp_hex:

        ; 得到相对于字符‘A’的ASCII码

        sub     al, 0ah
        add     al, 'A'

    _can_disp:
        mov     ah, 0ch         ; 颜色
        mov     edi, [CurrentDisplayPosition]
        mov     [gs:edi], ax
        add     edi, 2

        mov     [CurrentDisplayPosition], edi

        popad
        mov     esp, ebp
        pop     ebp
        ret     4
; DisplayOneHex endp
;---------------------------------------------------



;---------------------------------------------------
; 在屏幕当前位置显示一个十六进制数字
;
; 采用栈传递参数，stdcall 调用方式
;
; 输入参数：
; 1.    要显示的整数
;---------------------------------------------------

DisplayInt:
        push    ebp
        mov     ebp, esp
        pushad

        mov     ebx, [ebp + 8]
        mov     ecx, 32

        ; 循环8次显示一个整数，每次4位

    _loop_disp_hex:
        sub     ecx, 4
        mov     eax, ebx
        shr     eax, cl
        and     eax, 0fh
        push    ecx

        push    eax
        call    DisplayOneHex

        pop     ecx
        test    ecx, ecx
        jnz     _loop_disp_hex

        ; 数字后加上'h'，再加一个空格

        mov     ah, 7
        mov     al, 'h'
        mov     edi, [CurrentDisplayPosition]
        mov     [gs:edi], ax
        add     edi, 4
        mov     [CurrentDisplayPosition], edi

        popad
        mov     esp, ebp
        pop     ebp
        ret     4
; DisplayInt endp
;---------------------------------------------------



;---------------------------------------------------
; 在屏幕当前位置显示字符串，可指定颜色，遇到 0结束，0ah换行
;
; 采用栈传递参数，stdcall 调用方式
;
; 输入参数：
; 1. 指向要显示的字符串
; 2. 颜色
;---------------------------------------------------

DisplayColorString:
        push    ebp
        mov     ebp, esp
        pushad

        mov     esi, [ebp + 8]
        mov     edi, [CurrentDisplayPosition]
        cmp     edi, 25 * 80 * 2
        jb      _c_dis
        
        ; 若超出一屏，回到0行0列
        xor     edi, edi
        
    _c_dis:
        mov     ah, [ebp + 12]         ; 颜色

    _loop_char_disp:
        lodsb
        test    al, al
        jz      _ret_disp

        cmp     al, 0ah
        jnz     _disp_str

        ; 0ah,处理回车

        push    eax
        mov     eax, edi

        ; 每行 160字节，除以 160得到当前所在的行
        mov     bl, 160
        div     bl
        and     eax, 0ffh

        ; 换到下一行开头
        inc     eax
        mov     bl, 160
        mul     bl
        mov     edi, eax
        pop     eax
        jmp     _loop_char_disp

    _disp_str:
        mov     [gs:edi], ax
        add     edi, 2
        jmp     _loop_char_disp

    _ret_disp:
        mov     [CurrentDisplayPosition], edi
        popad
        mov     esp, ebp
        pop     ebp
        ret     8
; DisplayColorString endp
;---------------------------------------------------



;---------------------------------------------------
; 在屏幕当前位置显示字符串，黑底白字，遇到 0结束，0ah换行
;
; 采用栈传递参数，stdcall 调用方式
;
; 输入参数：
; 1. 指向要显示的字符串
;---------------------------------------------------

DisplayString:
        push    ebp
        mov     ebp, esp

        push    dword 0ch
        push    dword [ebp + 8]
        call    DisplayColorString

        mov     esp, ebp
        pop     ebp
        ret     4
; DisplayString endp
;---------------------------------------------------



;---------------------------------
; 换行
;---------------------------------

DisplayReturn:

        ; 在栈上构造一个串: 0ah, 0

        push    dword 0ah
        push    esp
        call    DisplayString
        pop     eax
        ret
; DisplayReturn endp
;---------------------------------



szStackPtr      db  'esp = ', 0
szStackEbp      db  ', ebp = ', 0

;---------------------------------
;---------------------------------

DisplayStackPtr:
        call    DisplayReturn
        
        push    szStackPtr
        call    DisplayString
        push    esp
        call    DisplayInt
        
        push    szStackEbp
        call    DisplayString
        push    ebp
        call    DisplayInt
        ret
; DisplayStackPtr endp
;---------------------------------



