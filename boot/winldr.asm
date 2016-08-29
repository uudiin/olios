
;-----------------------------------------------------------------
; windows loader
; 该文件被 WINDOWS 的 ntldr 或者 winloader 当做启动 mbr加载到0x7c00，
; 这里的代码会把自己移到 0x600,然后把操作系统所在分区的引导扇区加载到0x7c00,
; 实现多系统共存
;
; 输入参数：
; dl : 可引导驱动器
;
; 输出参数：
; bp : 指向读出的操作系统所在分区的引导扇区,这里值是0x7c00
;
; 编译：
; nasm winldr.asm -o winldr
;
; 修改时间列表：
;   2011-03-01 21:30:11 
;   2011-05-03 19:34:58 
;   2011-05-07 22:34:16 
;   2011-05-08 14:30:53 
;-----------------------------------------------------------------


LOADER_STACK_TOP    equ     9000h
TEMP_BUFFER_SEG     equ     8000h

WINLDR_SIZE         equ     512 * 2     ; 假设这个文件的大小不会超1K
EXTENDED_TYPE       equ     5
W95_EXTENDED_TYPE   equ     0fh
W95_FAT32           equ     0bh

;
; 这个偏移处是 OS 所在分区的标志 "oliexztj"
;

OLIEX_SIGN_OFFSET   equ     180h


        org     600h
        
    _winldr_base:
        
        jmp     _Startup
        
BootDrive       db  0
        

    _Startup:
        cli
        xor     ax, ax
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     ss, ax
        mov     sp, LOADER_STACK_TOP
        sti
        
        ;
        ; 将自己移动到0x600,然后跳过去执行
        ;
        
        mov     cx, WINLDR_SIZE
        mov     si, 7c00h
        mov     di, _winldr_base
        rep movsb

        push    ax
        push    _real_start
        retf


    _real_start:
        
        ; 测试磁盘是否支持扩展 int 13h 调用，不支持返回失败
        
        mov     [BootDrive], dl
        mov     ah, 41h
        mov     bx, 55aah
        int     13h
        jb      _error_disk
        cmp     bx, 0aa55h
        jnz     _error_disk
        test    cx, 1
        jz      _error_disk


        ; 清屏
        
        call    ClearScreen
        
        ;
        ; 加载操作系统所在的 FAT32 文件系统引导扇区
        ;
        
        xor     eax, eax
        call    parse_partition_table
        jb      _error_disk


        
    
    ; 运行到这里表明出错，显示错误信息，停机
    
    _error_disk:
        mov     si, szDiskError
        call    DisplayText
        jmp     _pause


    _pause:
        pause
        jmp     _pause



;---------------------------------------------------------

        align   4

ExtendedBaseSector      dd  0

Signature       db  'oliexztj', 0

szDiskError     db  'a disk error occured', 0dh, 0ah, 0

sz1     db  'this is winldr', 0dh, 0ah, 0
sz2     db  'partition table', 0dh, 0ah, 0
sz3     db  '    fat32 partition', 0dh, 0ah, 0
sz4     db  '    unknown partition', 0dh, 0ah, 0
sz5     db  'read is normal', 0dh, 0ah, 0

;---------------------------------------------------------



;---------------------------------------------------------------
; 解析一个分区表，包括主分区和扩展分区的分区表
; 加载操作系统所在的 FAT32 文件系统引导扇区
; 输入参数： eax -> 分区表所在的扇区绝对地址，把这个扇区作为根扇区开始解析
; 输出参数： 若失败，C位被设置
;---------------------------------------------------------------

parse_partition_table:
        push    bp
        mov     bp, sp
        sub     sp, 44h         ; -4   : LBA
                                ; -44h : partition table
        push    eax
        push    ebx
        
        pushad
        mov     si, sz2
        call    DisplayText
        popad
        
        mov     [bp - 4], eax
        push    TEMP_BUFFER_SEG
        pop     es
        xor     ebx, ebx
        mov     dx, 1
        call    read_sector
        jb      _parse_ret
        
        ;
        ; 将分区表拷贝到当前函数的局部变量中
        ;
        
        xor     eax, eax
        push    ds
        mov     cx, 40h
        push    TEMP_BUFFER_SEG
        pop     ds
        mov     si, 1beh
        push    ss
        pop     es
        lea     ax, [bp - 44h]
        mov     di, ax                  ; eax -> 分区项
        rep movsb
        pop     ds
        
        xor     edi, edi
        mov     edi, eax                ; edi -> 分区项
        
        ;
        ; 这里开始对四个分区项循环查找，其中可能会出现递归
        ;
        
    _parse_loop:
    
        mov     cl, byte [edi + 4]      ; 文件系统标识
        test    cl, cl
        jz      _parse_next
        
        cmp     cl, W95_FAT32
        jnz     _not_fat32
        
        ;
        ; 到这里，是个 FAT32分区
        ;
        
        pushad
        mov     si, sz3
        call    DisplayText
        popad
        
        mov     esi, dword [edi + 8]    ; 相对扇区偏移
        add     esi, dword [bp - 4]
        call    process_fat32_partition
        
    _not_fat32:
    
        cmp     cl, EXTENDED_TYPE
        jz      _parse_expand_partition
        cmp     cl, W95_EXTENDED_TYPE
        jz      _parse_expand_partition
        
        pushad
        mov     si, sz4
        call    DisplayText
        popad
        
        jmp     _parse_next
        
    _parse_expand_partition:
    
        mov     eax, dword [edi + 8]
        
        cmp     dword [bp - 4], 0
        jnz     _parse3
        
        ;
        ; 一个扩展分区中的其它扩展分区基地址都是基于根扩展分区的
        ; 这里只对 MBR 中分区表的扩展分区赋值
        ;
        
        mov     dword [ExtendedBaseSector], eax
        xor     eax, eax
        
    _parse3:
    
        add     eax, dword [ExtendedBaseSector]
        call    parse_partition_table
        
    _parse_next:
    
        add     edi, 10h
        xor     eax, eax
        lea     ax, [bp - 44h + 40h]
        cmp     edi, eax
        jb      _parse_loop
        
    _parse_ret:
        
        pop     ebx
        pop     eax
        mov     sp, bp
        pop     bp
        ret
; parse_partition_table endp
;---------------------------------------------------------------



sz10    db  'process fat32 partition', 0dh, 0ah, 0
sz11    db  '^_^ olios fat32 ^_^', 0dh, 0ah, 0



;------------------------------------------------------------------
; 处理一个FAT32分区,通过指定的标志判断是不是 oliew 系统的分区
; 输入参数: esi = FAT32分区所在的扇区绝对地址
; 返回值: 若不是oliew系统的分区，返回，否则加载文件系统第二部分引导代码，不返回
;------------------------------------------------------------------

process_fat32_partition:
        pushad
        
        pushad
        mov     si, sz10
        call    DisplayText
        popad

        ; 把该分区的引导扇区直接读到 0x7c00

        xor     eax, eax
        mov     ds, ax
        mov     es, ax
        mov     eax, esi
        mov     ebx, 7c00h
        mov     dx, 1
        call    read_sector
        jb      _fat32_ret

        mov     edi, 7c00h
        
        ;
        ; 这里判断标志 "oliexztj"
        ;
        
        mov     eax, [edi + OLIEX_SIGN_OFFSET]
        or      eax, 20202020h
        cmp     eax, [Signature]
        jnz     _fat32_ret
        mov     eax, [edi + OLIEX_SIGN_OFFSET + 4]
        or      eax, 20202020h
        cmp     eax, [Signature + 4]
        jnz     _fat32_ret
        
        ;
        ; 修改 FAT32 中 BPB 相应的参数，使引导扇区可以访问到正确的位置
        ;
        
        mov     dword [edi + 1ch], esi        ; Bpb.HiddenSectors
        
        pushad
        mov     si, sz11
        call    DisplayText
        popad

        ; /*
        ; 这里是否应直接加载操作系统所在分区的引导扇区    // FIXME
        ;
        
    ;    xor     eax, eax
    ;    mov     ax, word [edi + 27h]         ; BackupBootSector
    ;    imul    eax, eax, 2

        ;
        ; 读出保留扇区，其中是进一步引导的代码
        ;
        
    ;    add     eax, esi
    ;    push    eax
    ;    push    0
    ;    pop     es
    ;    mov     ebx, 7c00h
    ;    mov     dx, 1
    ;    call    read_sector
    ;    pop     eax
    ;    jb      _fat32_ret
    ; */

        ;
        ; 跳到 0x7c00执行，这里寄存器的初始化可以不要
        ;
        
        xor     eax, eax
        xor     ebx, ebx
        xor     ecx, ecx
        xor     edx, edx
        xor     esi, esi
        xor     edi, edi
        mov     dl, byte [BootDrive]
        mov     ebp, 7c00h - 4
        mov     esp, ebp
        
        push    ax
        push    7c00h
        retf

    _fat32_ret:

        popad
        ret
; process_fat32_partition end
;------------------------------------------------------------------



;----------------------------------------
; 读取指定数量的扇区到指定位置
; 参数:
; es:bx 读出的数据放入的缓冲区地址
; eax:  扇区LBA绝对地址
; dx:   要读的扇区数
; 返回值: 若失败，C位被设置
;----------------------------------------

read_sector:
        pushad
        push    ds
        push    es

        ; 构造磁盘地址数据包

        push    dword 0
        push    eax
        push    es
        push    bx
        push    dx
        push    10h
        mov     ah, 42h
        mov     dl, byte [BootDrive]
        push    ss
        pop     ds
        mov     si, sp
        int     13h
        add     sp, 10h

        pop     es
        pop     ds
        popad
        ret
; read_sector endp
;----------------------------------------



;------------------------------------------------
; 在屏幕上输出字符串，通过 int 10h调用
; 输入参数：
; si -> 要显示的字符，以 0结尾
;------------------------------------------------

DisplayText:
        lodsb
        test    al, al
        jz      _ret_display
        
        mov     ah, 0eh
        mov     bx, 7
        int     10h
        jmp     DisplayText
        
    _ret_display:
        ret
; DisplayText endp
;------------------------------------------------



;-------------------------------
; 清屏
;-------------------------------

ClearScreen:
        mov     ax, 600h
        mov     bx, 700h
        mov     cx, 0
        mov     dx, 184fh
        int     10h
        ret
; ClearScreen endp
;-------------------------------



;--------------------
; 停机
;--------------------
Halt:
    pause
    jmp     Halt
; Halt endp
;--------------------


