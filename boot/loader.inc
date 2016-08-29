
;---------------------------------------------------------------------
; load.inc
; 内核加载器LOADER加载到9000开始处地方，最大可以到9FC00处
; 真正的内核开了保护模式后加载到内存偏移8M处
;
; 修改时间列表:
; 2011-01-04 20:58:43
;---------------------------------------------------------------------


%include    "common.inc"


LOADER_SEG          equ     900h        ; LOADER加载的段
TEMP_BUFFER_SEG     equ     8000h

LOADER_BASE         equ     9000h
LOADER_STACK_TOP    equ     LOADER_BASE
KERNEL_PRE_BASE     equ     0c00000h    ; 调整之前的位置   12M处


; ARD之Type
AddressRangeMemory      equ     1
AddressRangeReserved    equ     2



;
; FAT32 define
;

; fsboot offset
BytesPerSector          equ     0 + 11
SectorsPerCluster       equ     2 + 11
ReservedSectors         equ     3 + 11
Fats                    equ     5 + 11
HiddenSectors           equ     11h + 11
LargeSectors            equ     15h + 11
LargeSectorsPerFat      equ     19h + 11
ExtendedFlags           equ     1dh + 11
RootDirFirstCluster     equ     21h + 11
PhysicalDriverNumber    equ     40h

; dir entry offset
FirstClusterOfFileHigh  equ     20
FirstClusterOfFileLow   equ     26
FileSize                equ     28

FAT32_ENTRY_MASK        equ     0fffffffh       ; 7 个 F
FAT32_DIRTY_ENTRY       equ     7fffffffh       ; 最高位为 0


; FAT表中有效的簇状态值
;                               12345678h
FAT_CLUSTER_AVAILABLE   equ     00000000h
FAT_CLUSTER_RESERVED    equ     0ffffff0h
FAT_CLUSTER_BAD         equ     0ffffff7h
FAT_CLUSTER_LAST        equ     0fffffffh       ; 一个文件的最后一个簇

; 名称域的首字符
FAT_DIRENT_NEVER_USED       equ     000h
FAT_DIRENT_REALLY_OE5       equ     005h
FAT_DIRENT_DIRECTORY_ALIAS  equ     02eh
FAT_DIRENT_DELETED          equ     0e5h

; FAT 目录项属性，偏移 11字节
FAT_DIRENT_ATTR_READ_ONLY   equ     001h
FAT_DIRENT_ATTR_HIDDEN      equ     002h
FAT_DIRENT_ATTR_SYSTEM      equ     004h
FAT_DIRENT_ATTR_VOLUME_ID   equ     008h
FAT_DIRENT_ATTR_DIRECTORY   equ     010h
FAT_DIRENT_ATTR_ARCHIVE     equ     020h
FAT_DIRENT_ATTR_DEVICE      equ     040h
FAT_DIRENT_ATTR_LFN         equ     00fh        ; 长文件名



IRQ_CLOCK	equ	0
IRQ_KEYBOARD	equ	1
IRQ_CASCADE	equ	2
IRQ_ETHER	equ	3
IRQ_PS2MOUSE	equ	12
IRQ_AT_WINI	equ	14

INT_VECTOR_IRQ0		equ	20h
INT_VECTOR_IRQ8		equ	28h
INT_VECTOR_SYS_CALL	equ	40h

SYS_CALL_GET_TICKCOUNT	equ	0
SYS_CALL_CREATE_PROCESS	equ	1

EOI		equ	20h
TIMER_FREQUENCY	equ	1193182
OLIEX_HZ	equ	100

VIDEO_800_600_16M	equ	115h
VIDEO_1024_768_16M	equ	118h

PORT_INTM_CTRL		equ	20h
PORT_INTM_CTRLMASK	equ	21h
PORT_INTS_CTRL		equ	0a0h
PORT_INTS_CTRLMASK	equ	0a1h
PORT_8253_MODE		equ	43h
PORT_8253_COUNTER0	equ	40h


