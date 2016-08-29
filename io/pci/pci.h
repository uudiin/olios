
#ifndef __PCI_H__
#define __PCI_H__



#define  PCI_VENDOR_ID_INTEL        0x8086
#define  PCI_VENDOR_ID_COMPAQ       0x0e11
#define  PCI_CLASS_DISPLAY_VGA      0x0300
#define  PCI_CLASS_BRIDGE_HOST      0x0600
#define  PCI_VENDOR_ID              0x00
#define  PCI_DEVICE_ID              0x02
#define  PCI_COMMAND                0x04
#define  PCI_COMMAND_IO             0x1
#define  PCI_COMMAND_MEMORY         0x2
#define  PCI_COMMAND_MASTER         0x4
#define  PCI_COMMAND_SPECIAL        0x8
#define  PCI_COMMAND_INVALIDATE     0x10
#define  PCI_COMMAND_VGA_PALETTE    0x20
#define  PCI_COMMAND_PARITY         0x40
#define  PCI_COMMAND_WAIT           0x80
#define  PCI_COMMAND_SERR           0x100
#define  PCI_COMMAND_FAST_BACK      0x200

#define  PCI_STATUS                 0x06
#define  PCI_STATUS_CAP_LIST        0x10
#define  PCI_STATUS_66MHZ           0x20
#define  PCI_STATUS_UDF             0x40
#define  PCI_STATUS_FAST_BACK       0x80
#define  PCI_STATUS_PARITY          0x100
#define  PCI_STATUS_DEVSEL_MASK     0x600
#define  PCI_STATUS_DEVSEL_FAST     0x000	
#define  PCI_STATUS_DEVSEL_MEDIUM   0x200
#define  PCI_STATUS_DEVSEL_SLOW     0x400
#define  PCI_STATUS_SIG_TARGET_ABORT    0x800
#define  PCI_STATUS_REC_TARGET_ABORT    0x1000
#define  PCI_STATUS_REC_MASTER_ABORT    0x2000
#define  PCI_STATUS_SIG_SYSTEM_ERROR    0x4000
#define  PCI_STATUS_DETECTED_PARITY     0x8000

#define  PCI_CLASS_REVISION         0x08
#define  PCI_REVISION_ID            0x08
#define  PCI_CLASS_PROG             0x09
#define  PCI_CLASS_DEVICE           0x0a

#define  PCI_CACHE_LINE_SIZE        0x0c
#define  PCI_LATENCY_TIMER          0x0d
#define  PCI_HEADER_TYPE            0x0e
#define  PCI_HEADER_TYPE_NORMAL     0
#define  PCI_HEADER_TYPE_BRIDGE     1
#define  PCI_HEADER_TYPE_CARDBUS    2

#define  PCI_BIST                   0x0f
#define  PCI_BIST_CODE_MASK         0x0f
#define  PCI_BIST_START             0x40
#define  PCI_BIST_CAPABLE           0x80


#define     CONFIG_CMD(bus, dev, fn, reg)        (0x80000000 | ((bus) << 16 | (dev) << 11 | (fn) << 8 | (reg) & ~3))




void
_stdcall
PciPutInfo(
    ulong   Bus,
    ulong   Dev,
    ulong   Fun
    );

uchar
_stdcall
PciScanDevice(
    ulong   Bus,
    ulong   Device,
    ulong   Function
    );

uchar
_stdcall
PciCheckDirect();

uchar
_stdcall
PciSanityCheck();

uchar
_stdcall
PciReadConfigByte(
    ulong   Bus,
    ulong   Device,
    ulong   Function,
    ulong   Register
    );

ushort
_stdcall
PciReadConfigWord(
    ulong   Bus,
    ulong   Device,
    ulong   Function,
    ulong   Register
    );

ulong
_stdcall
PciReadConfigDword(
    ulong   Bus,
    ulong   Device,
    ulong   Function,
    ulong   Register
    );

void
_stdcall
PciWriteConfigByte(
    ulong   Bus,
    ulong   Device,
    ulong   Function,
    ulong   Register,
    uchar   Value
    );

void
_stdcall
PciWriteConfigWord(
    ulong   Bus,
    ulong   Device,
    ulong   Function,
    ulong   Register,
    ushort  Value
    );

void
_stdcall
PciWriteConfigDword(
    ulong   Bus,
    ulong   Device,
    ulong   Function,
    ulong   Register,
    ulong   Value
    );


#endif  // __PCI_H__

