
;------------------------------------------------------------
; loader for olios
; olios操作系统加载器
;
; nasm zhldr.asm -o zhldr
;
; 最后修改时间：2011-01-04
;
; 加载操作系统,从FAT32活动分区根目录中查找内核文件并加载
;
;
; 输入参数：
; bp -> FAT32引导扇区
;
; NOTE:  关于数据传送指令使用的段寄存器
;    1. rep movsb        ; es:[di] ds:[si]
;        这种情况下，目标 di使用 es寄存器，源 si使用 ds
;
;    2. mov ax, [di]     ; ds:[di]
;       mov ax, [si]     ; ds:[si]
;        其它普通的传送指令，都使用 ds寄存器
;-----------------------------------------------------------



%include    "loader.inc"


        org     LOADER_BASE
        
        jmp     _Startup
        
        align   16
        
GdtTable:
gdt0    DESCRIPTOR  00000000h, 000000h, 00000h
gdt1    DESCRIPTOR  00000000h, 0fffffh, 0c09ah  ; 0级执行/读的代码段
gdt2    DESCRIPTOR  00000000h, 0fffffh, 0c092h  ; 0级读/写的数据段
gdt3    DESCRIPTOR  00000000h, 0fffffh, 0c0fah  ; 3级执行/读的代码段
gdt4    DESCRIPTOR  00000000h, 0fffffh, 0c0f2h  ; 3级读/写的数据段
gdt5    DESCRIPTOR  000b8000h, 00ffffh, 040f2h  ; 显存地址描述,R3可访问
gdt6    DESCRIPTOR   TSS_BASE, 000068h, 00089h  ; 可用386TSS

GDT_TEMP_LEN        equ     $ - GdtTable



;选择子，也即相对于GDT头的偏移
SelectorCodeR0      equ     gdt1 - GdtTable
SelectorDataR0      equ     gdt2 - GdtTable
SelectorCodeR3      equ     gdt3 - GdtTable + 3     ; Ring3可访问
SelectorDataR3      equ     gdt4 - GdtTable + 3
SelectorVideo       equ     gdt5 - GdtTable + 3
SelectorTss         equ     gdt6 - GdtTable


; 16 位段

[BITS 16]

    _Startup:
        cli
        xor     ax, ax
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     ss, ax
        mov     sp, LOADER_STACK_TOP
        sti

        push    ax
        push    _real_start
        retf


    _real_start:
        
        ; 测试磁盘是否支持扩展 int 13h调用，不支持返回失败
        
        mov     dl, byte [bp + PhysicalDriverNumber]
        mov     ah, 41h
        mov     bx, 55aah
        int     13h
        jb      _error_disk
        cmp     bx, 0aa55h
        jnz     _error_disk
        test    cx, 1
        jz      _error_disk


        ; 清屏
        
        push    bp
        call    ClearScreen
        call    InitVideo       ; // FIXME
        pop     bp
        
        pushad
        mov     si, szZhldr
        call    DisplayText
        popad
        
        ; 加载内核到高内存
        
        call    LoadKernel

        push    es
        mov     ax, TEMP_BUFFER_SEG
        mov     es, ax
        xor     edi, edi
        call    GetMemoryRange
        pop     es

        ; 定位系统描述符表, GDT, IDT, TSS

        call    SetSystemDescriptors

        ; 设置内存范围描述数组和 LOADER参数块

        call    SetLoaderParamtersBlock
        
        ; call    InitVideo     ; // FIXME
        
        ;
        ; 下面切换到保护模式
        ;

        push    dword GDT_BASE
        push    dword GDT_LENGTH << 16
        lgdt    [esp + 2]
        add     sp, 8
        
        ; enable A20

        cli
        in      al, 92h
        or      al, 10b
        out     92h, al
        
        mov     eax, cr0
        or      eax, 1
        mov     cr0, eax
        
        jmp     dword SelectorCodeR0:_StartCode32
        
    
    ; 运行到这里表明出错，显示错误信息，停机
    
    _error_disk:
        mov     si, szDiskError
        call    DisplayText
        jmp     _pause
        
    _error_fs:
        mov     si, szFsError
        call    DisplayText
        jmp     _pause

    _error_no_kernel:
        mov     si, szNoKernel
        call    DisplayText
        jmp     _pause
        
    _error_move_data:
        mov     si, szMoveError
        call    DisplayText
        jmp     _pause

    _error_check_mem:
        mov     si, szMemError
        call    DisplayText
        jmp     _pause
        
    _pause:
        pause
        jmp     _pause



szZhldr         db  'here is zhldr', 0dh, 0ah, 0
szDiskError     db  'a disk error occured', 0dh, 0ah, 0
szFsError       db  'file system error occured', 0dh, 0ah, 0
szMoveError     db  'transfer data to high memory error', 0dh, 0ah, 0
szMemError      db  'get memory range via e820 error', 0dh, 0ah, 0
szNoKernel      db  'no kernel have been found', 0dh, 0ah, 0



;---------------------------------------------------------------------
; 通过 e820得到内存范围
; 输入参数：
; es:di 内存信息的输出地址，指向一个缓冲区
; 全局变量 MemoryDescCount 记录找到的内存范围个数
;---------------------------------------------------------------------

GetMemoryRange:
        pushad
        xor     ebx, ebx
        
    _loop_e820:
        mov     eax, 0e820h
        mov     ecx, 20
        mov     edx, 534d4150h  ; 'SMAP'
        int     15h
        jc      _error_check_mem
        
        add     di, 20
        inc     dword [MemoryDescCount]
        test    ebx, ebx
        jnz     _loop_e820

        popad
        ret
; GetMemoryRange endp
;---------------------------------------------------------------------



;-----------------------------------------------
; 重新定位系统段，包括 GDT IDT TSS
; GDT   直接把LOADER中定义的几项拷贝过去，需要时再加
; IDT   暂时清 0
; TSS   暂时清 0
;-----------------------------------------------

SetSystemDescriptors:
        pushad
        push    ds
        push    es

        ; 先将 GDT IDT TSS所在的区域清 0

        push    SYS_DESC_SEG
        pop     es
        xor     eax, eax
        xor     edi, edi
        mov     ecx, 400h + 800h + 400h
        rep stosb

        ; 将LOADER中定义的GDT移到指定的位置

        mov     ax, LOADER_SEG
        mov     ds, ax
        mov     esi, GdtTable - LOADER_BASE     ; 相对于段的偏移
        xor     edi, edi
        mov     ecx, GDT_TEMP_LEN
        rep movsb

        pop     es
        pop     ds
        popad
        ret
; SetSystemDescriptors endp
;-----------------------------------------------



;--------------------------------------------------
; 这个函数包括两部分功能：
; 一  解析内存地址范围描述数组，把可用的内存范围放到指定位置
;     形成一个 Base Size 形式的只有两个双字的数组
; 二  为内核准备参数块
;     参数块的前两个双字为ARD数组的实模式地址和数组项数
;--------------------------------------------------

SetLoaderParamtersBlock:
        pushad
        push    ds
        push    es

        ; 先将参数块和 ARD数组清0

        push    LPB_SEG
        pop     es
        xor     eax, eax
        xor     edi, edi
        mov     ecx, 1000h + 400h
        rep stosb

        ; 重新解析之前得到的ARD数组

        xor     ebx, ebx        ; 用做新生成数组计数个数
        mov     ecx, [MemoryDescCount]
        push    dword [KernelFileSize]
        mov     ax, TEMP_BUFFER_SEG
        mov     ds, ax
        xor     esi, esi
        mov     ax, ARD_SEG
        mov     es, ax
        xor     edi, edi

    _loop_reset_ard:
        cmp     dword [si + 16], AddressRangeMemory     ; 1
        jnz     _nouse_mem

        ; 是一个可用的内存范围
        ; 分别取出开始地址和长度的低32位保存起来

        inc     ebx
        movsd               ; BaseLow32
        add     si, 4
        movsd               ; LengthLow32
        add     si, 8
        jmp     _next_reset_ard

    _nouse_mem:
        add     esi, 20

    _next_reset_ard:
        dec     ecx
        jnz     _loop_reset_ard

        ; 将参数块中前两个双字分别设成ARD数组的地址和个数
        ; +10h is KernelFileSize
        ; 这里向 di的数据传送要使用 ds段寄存器

        mov     ax, LPB_SEG
        mov     ds, ax
        xor     edi, edi
        mov     dword [di], ARD_BASE
        mov     [di + 4], ebx
        pop     dword [di + 10h]

        pop     es
        pop     ds
        popad
        ret
; SetLoaderParamtersBlock endp
;--------------------------------------------------



;----------------------------------------
; 全局变量
;----------------------------------------
        align   4
RootDirSector       dd  0
CurrentFatSector    dd  0
KernelFileSize      dd  0
MemoryDescCount     dd  0
KernelPrevBase      dd  KERNEL_PRE_BASE
KernelModuleName    db  "ZHOSKRNL   "



;-------------------------------------------------------
; 加载内核
; 输入参数： bp -> FAT32 引导扇区
;-------------------------------------------------------

LoadKernel:

        ;
        ; 取得根目录开始扇区
        ; 该分区前的隐藏扇区 + 保留扇区(包括引导扇区) + 两个FAT表的大小
        ;

        movzx   eax, byte [bp + Fats]
        mov     ecx, [bp + LargeSectorsPerFat]
        mul     ecx
        add     eax, [bp + HiddenSectors]
        movzx   edx, word [bp + ReservedSectors]
        add     eax, edx
        mov     [RootDirSector], eax
        
        ; 
        ; 从 BPB中拿出根目录的第一个簇
        ;

        mov     dword [CurrentFatSector], 0ffffffffh
        mov     eax, [bp + RootDirFirstCluster]
        cmp     eax, 2              ; 最小簇号
        jb      _error_fs
        cmp     eax, 0ffffff8h      ; 最大簇号
        jnb     _error_fs
        
        
    _loop_root_dir:

        ;
        ; 从根目录开始遍历，eax是 FAT中的簇号
        ; 把簇号转换成扇区号
        ;

        push    eax
        sub     eax, 2
        movzx   ebx, byte [bp + SectorsPerCluster]
        mov     si, bx
        mul     ebx
        add     eax, dword [RootDirSector]
        
    _loop_cluster:

        ;
        ; 遍历根目录中入口项所在的簇，读出每一个扇区到 8200h
        ; eax 中是扇区号
        ;

        mov     bx, 8200h
        mov     di, bx
        mov     cx, 1
        call    ReadSector
        
    _loop_sector:

        ;
        ; 遍历一个扇区中的目录入口
        ; 比较是否是内核文件
        ;

        cmp     [di], ch        ; 0
        jz      _error_fs
        mov     cl, 11
        push    si
        mov     si, KernelModuleName
        repz cmpsb
        
        pop     si
        jz      _found_kernel
        
        ; 使 di指向下一个入口

        add     di, cx
        add     di, 21
        
        cmp     di, bx
        jb      _loop_sector
        
        dec     si              ; si = SectorsPerCluster
        jnz     _loop_cluster
        
        pop     eax
        call    GetNextCluster
        jb      _loop_root_dir
        
        add     sp, 4
        jmp     _error_no_kernel
        
        
    _found_kernel:

        ;
        ; 找到内核文件
        ; 取出它的开始簇号到 eax中
        ;

        add     sp, 4
        sub     di, 11      ; di -> dir entry

        ; 先得到内核文件大小(字节)

        mov     eax, [di + FileSize]
        mov     [KernelFileSize], eax

        mov     si, [di + FirstClusterOfFileHigh]
        mov     di, [di + FirstClusterOfFileLow]
        mov     ax, si
        shl     eax, 16
        mov     ax, di
        
        cmp     eax, 2
        jb      _error_fs
        cmp     eax, 0ffffff8h
        jnb     _error_fs
        
    _read_cluster:

        ;
        ; 循环读出内核文件并传送到高地址
        ; eax 是内核文件的开始簇号
        ;

        push    eax
        sub     eax, 2
        movzx   ecx, byte [bp + SectorsPerCluster]
        mul     ecx
        add     eax, [RootDirSector]
        xor     ebx, ebx
        push    es
        push    TEMP_BUFFER_SEG
        pop     es
        call    ReadSector
        pop     es
        
        ;
        ; 把数据传送到高地址空间
        ;
        
        mov     ecx, ebx
        mov     esi, TEMP_BUFFER_SEG
        shl     esi, 4
        mov     edi, [KernelPrevBase]
        call    TransferToHighMemory
        jc      _error_move_data        ; this, if int 15h fails
        
        pop     eax
        add     [KernelPrevBase], ebx
        call    GetNextCluster
        jb      _read_cluster
        
    _finish_load:
        ret
; LoadKernel endp
;-------------------------------------------------------



;-------------------------------------------------------
; 下面的表用于 int 15h, 87h  把数据传送到高内存区
; 该描述符表共 6项，每项 8字节
;
        
blk_mov_dt:
        dw  0, 0, 0, 0      ; 伪描述符
        dw  0, 0, 0, 0      ; 描述表的描述符
        
blk_mov_src:
                dw  0ffffh
blk_src_base    db  00, 00, 01  ; base = 10000h
                db  93h         ; type
                dw  0           ; limit16, base24 = 0
                
blk_mov_dst:
                dw  0ffffh
blk_dst_base    db  00, 00, 10  ;base = 100000
                db  93h
                dw  0
                
        dw  0, 0, 0, 0      ; BIOS CS
        dw  0, 0, 0, 0      ; BIOS DS


;----------------------------------------------------
; 把数据从低地址(< 1M)传送到高地址
; 输入参数：
; esi   源地址
; edi   目标地址
; ecx   传输的长度(字节)
;
; 输出：
; 若发生错误，CF = 1
;----------------------------------------------------

TransferToHighMemory:
        pushad
        mov     eax, esi
        mov     word [blk_src_base], ax
        shr     eax, 16
        mov     byte [blk_src_base + 2], al
        
        mov     eax, edi
        mov     word [blk_dst_base], ax
        shr     eax, 16
        mov     byte [blk_dst_base + 2], al
        
        shr     ecx, 1
        mov     esi, blk_mov_dt
        mov     ax, 8700h
        int     15h
        popad
        ret
; TransferToHighMemory endp
;----------------------------------------------------



;-----------------------------------
; 取得 FAT表簇链的下一个簇
; 输入参数：
; eax   当前簇号 ID
;
; 返回值：
; C位置位，表明簇链还没到结尾
;-----------------------------------

GetNextCluster:
        shl     eax, 2
        call    ReadFatSector
        mov     eax, [bx + di]
        and     eax, 0fffffffh
        cmp     eax, 0ffffff8h
        ret
; GetNextCluster endp
;-----------------------------------



;----------------------------------------------------
; 读取簇号在 FAT表中对应的扇区到 7e00h
; 输入参数：
; eax   簇号在 FAT表中的偏移(字节)
;
; 返回值：
; di    7e00h
; bx    相对于一个扇区的偏移
;----------------------------------------------------

ReadFatSector:
        mov     di, 7e00h
        movzx   ecx, word [bp + BytesPerSector]
        xor     edx, edx
        
        ; eax / 扇区字节数，得到该 FAT项所在的扇区
        ; edx 余数，相对于一个扇区的偏移
        
        div     ecx
        
        ; 如果这个扇区已经被读入，直接跳过
        
        cmp     eax, [CurrentFatSector]
        jz      _ret_read_fat_sector
        
        mov     [CurrentFatSector], eax
        
        add     eax, [bp + HiddenSectors]
        movzx   ecx, word [bp + ReservedSectors]
        add     eax, ecx
        
        ; 判断是否有活动 FAT数目
        
        movzx   ebx, word [bp + ExtendedFlags]
        and     bx, 0fh
        jz      __read
        
        cmp     bl, [bp + Fats]
        jnb     _error_fs
        
        push    dx
        mov     ecx, eax
        mov     eax, [bp + LargeSectorsPerFat]
        mul     ebx
        add     eax, ecx
        pop     dx
        
    __read:
        push    dx
        mov     bx, di
        mov     cx, 1
        call    ReadSector
        
        pop     dx
        
    _ret_read_fat_sector:
        mov     bx, dx      ; 余数
        ret
; ReadFatSector endp
;----------------------------------------------------



;-----------------------------------------------------
; 读扇区支持函数，用扩展 int 13h读 42h
; 输入参数：
; eax   要读的扇区的绝对地址
; es:bx 目标地址
; cx    读的扇区数
;
; 输出：
; eax   下一个 LBA
; bx    读出数据的末尾
; cx    0
;-----------------------------------------------------

ReadSector:
        pushad
        
        ; 构造磁盘地址数据包
        
        push    dword 0
        push    eax
        push    es
        push    bx
        push    dword 10010h    ; 仅读取一个扇区
        mov     ah, 42h
        mov     dl, [bp + PhysicalDriverNumber]
        mov     si, sp
        int     13h
        add     sp, 10h
        
        popad
        jb      _error_disk
        add     bx, 200h
        inc     eax
        dec     cx
        jnz     ReadSector
        
        ret
; ReadSector endp
;-----------------------------------------------------



;------------------------------------------------
; 在屏幕上输出字符串，通过 int 10h调用
; 输入参数：
; si    -> 要显示的字符，以 0结尾
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



;------------------------------------------------
        align   4
VbeInfoBlock:   times 512 db 0
ModeInfoBlock:  times 256 db 0
PhysicalBasePtr dd  0


;------------------------------------------------
; 初始化显示信息
;------------------------------------------------
InitVideo:

        ;
        ; 取得VBE信息
        ;

        mov     ax, 4f00h
        mov     di, VbeInfoBlock
        int     10h

        ;
        ; 取得模式信息
        ;

        mov     ax, 4f01h
        mov     cx, VIDEO_800_600_16M
        mov     di, ModeInfoBlock
        int     10h

        ;
        ; 设置D14，使用平坦缓冲区
        ;

        ;mov     ax, 4f02h
        ;mov     bx, VIDEO_800_600_16M + 4000h
        ;mov     di, CrtcInfoBlock
        ;int     10h

        ;
        ; 取得当前模式
        ;

        ;xor     ebx, ebx
        ;mov     ax, 4f03h
        ;int     10h
        ;mov     [CurrentVbeMode], ebx

        mov     edi, ModeInfoBlock
        movzx   eax, word [edi + 16]
        ;mov     [BytesPerScanLine], eax    ; 2400
        
        movzx   eax, byte [edi + 25]
        ;mov     [BitsPerPixel], eax        ; 24
        
        mov     eax, [edi + 40]
        mov     [PhysicalBasePtr], eax     ; 0xe0000000

        ret
; InitVideo endp
;------------------------------------------------



;------------------------------------------------------------------;
;------------------------------------------------------------------;
;                                                                  ;
;                         保护模式 32 位代码                          ;
;                                                                  ;
;------------------------------------------------------------------;
;------------------------------------------------------------------;

[BITS 32]

szWelcome           db  'welcome to olios OS', 0ah, 0
szHaveLoadedKernel  db  'zhoskrnl has been loaded to physical address ', 0
szVideoBuf          db  'the vga flat buffer : ', 0


        align   4

CurrentDisplayPosition  dd  (80 * 1 + 0) * 2    ; 屏幕1行0列，每字符两字节


    _StartCode32:
        cli
        xor     eax, eax
        mov     ax, SelectorDataR0
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     ss, ax

        mov     ax, SelectorVideo
        mov     gs, ax
        mov     esp, LOADER_STACK_TOP

        push    szWelcome
        call    DisplayString

        ; 显示实模式下得到的内存地址范围数组信息

        call    DisplayMemoryRange

        ; 按ELF格式重新定位内核文件,这里是加载到物理地址，要减掉 KSEG0_BASE

        call    AdjustKernelImage

        push    szHaveLoadedKernel
        call    DisplayString
        push    KRNL_PBYSICAL_BASE
        call    DisplayInt
        call    DisplayReturn
        
        ;;;;;;;;;;;;;;;;;;;;;;;;;;
        call    DisplayReturn
        call    DisplayReturn
        call    DisplayReturn
        push    szVideoBuf
        call    DisplayString
        mov     eax, [PhysicalBasePtr]
        push    eax
        call    DisplayInt
        call    DisplayReturn
        ;;;;;;;;;;;;;;;;;;;;;;;;;;

        ; 在PLB中设置内核加载的物理地址，内存中调整后的映像大小，以及物理内存大小

        mov     eax, LPB_BASE
        mov     dword [eax + 8], KRNL_PBYSICAL_BASE
        mov     ecx, [KernelImageSize]
        mov     [eax + 0ch], ecx
        mov     ecx, [TotalMemorySize]
        mov     [eax + 14h], ecx
        or      dword [eax], KSEG0_BASE

        ; 跳到内核入口执行

        mov     eax, KRNL_PBYSICAL_BASE
        mov     eax, [eax + 18h]    ; entry 虚拟地址
        sub     eax, KSEG0_BASE     ; 转到物理地址
        jmp     eax
        ret             ; faked ret

    ; 出错，执行到这里，停机

    _pause_machine:
        pause
        jmp     _pause_machine



;-------------------------------------------------

        align   4

KernelImageSize    dd  0

;-------------------------------------------------
; 把实模式下读入高内存空间的内核映像按ELF格式重定位
; 读入的原映像地址定义为 KERNEL_PREV_BASE
; 新的执行虚拟地址在编译时指定，这里在ELF文件中取得
; 内核编译时的基地址在内核空间之上，这里加载到低的物理地址
; 需要减去内核与用户地址空间的分界
;-------------------------------------------------

AdjustKernelImage:
        pushad

        mov     ebx, KERNEL_PRE_BASE
        movzx   ecx, word [ebx + 2ch]   ; 程序头表中的各条目数量，即段的数目
        mov     edx, [ebx + 1ch]        ; 程序头表相对文件头的偏移
        add     edx, ebx                ; 指向程序头表

    _loop_adj:

        ; 判断 Type,为0则不加载这个段

        mov     edi, [edx]
        or      edi, edi
        jz      _next_adj

        push    ecx
        mov     edi, [edx + 8]      ; VA
        sub     edi, KSEG0_BASE     ; 转到物理内存低地址

        mov     ecx, [edx + 10h]    ; Size

        lea     eax, [edi + ecx]
        cmp     eax, [KernelImageSize]
        jb      _small_sec

        mov     [KernelImageSize], eax

    _small_sec:

        mov     esi, [edx + 4]      ; 文件中偏移
        add     esi, ebx            ; 加上文件在内存中的地址，定位到数据
        cld
        rep movsb
        pop     ecx

    _next_adj:
        add     edx, 20h
        loop    _loop_adj

        sub     dword [KernelImageSize], KRNL_PBYSICAL_BASE

        popad
        ret
; AdjustKernelImage endp
;-------------------------------------------------



;-------------------------------------------------
; 显示串定义

        align   4
TotalMemorySize     dd  0
szMemoryRangeTitle  db '  Base       Size',0ah,0
szRamSize           db 'RAM size : ',0

;-------------------------------------------------
; 显示实模式下取得的内存地址范围信息
; 并取得范围中最大的地址，也就是内存总容量
;-------------------------------------------------

DisplayMemoryRange:
        pushad

        push    szMemoryRangeTitle
        call    DisplayString

        ; 从LOADER参数块中取出内存地址范围数组的地址和个数

        mov     eax, LPB_BASE
        mov     esi, [eax]
        mov     ecx, [eax + 4]

    _loop_all_mem_info:
        push    ecx
        push    dword [esi]
        call    DisplayInt
        push    dword [esi + 4]
        call    DisplayInt
        call    DisplayReturn
        pop     ecx

        lodsd
        mov     ebx, eax
        lodsd
        add     ebx, eax
        cmp     ebx, [TotalMemorySize]
        jb      _n_disp_mem_info

        mov     [TotalMemorySize], ebx

    _n_disp_mem_info:
        loop    _loop_all_mem_info

        call    DisplayReturn

        push    szRamSize
        call    DisplayString

        push    dword [TotalMemorySize]
        call    DisplayInt

        call    DisplayReturn

        popad
        ret
; DisplayMemoryRange endp
;-------------------------------------------------



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



