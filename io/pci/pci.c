
/*-------------------------------------------------------------
    PCI 枚举，这个不是服务
    
    修改时间列表:
    2011-07-29 19:05:35 
-------------------------------------------------------------*/


#include    "ke.h"
#include    "proto.h"
#include    "pci.h"



/*-------------------------------------------------------------
-------------------------------------------------------------*/

void
_stdcall
IoPciEnum()
{
    ulong   bus;
    ulong   dev;
    ulong   fun;
    uchar   HdrType;
    uchar   IsMulti;
    
    if (!PciCheckDirect())
        DisplayColorString("\nPCI: No PCI bus detected\n", 15);
    
    //
    // 枚举0号总线和1号总线
    //
    
    for (bus = 0; bus < 2; bus++)
    {
        for (dev = 0; dev < 32; dev++)
        {
            IsMulti = FALSE;
            
            for (fun = 0; fun < 8; fun++)
            {
                if (fun && IsMulti == FALSE)
                    continue;
                
                if (PciScanDevice(bus, dev, fun) == FALSE)
                    continue;
                
                HdrType = PciReadConfigByte(bus, dev, fun, PCI_HEADER_TYPE);
                if (fun == 0)
                    IsMulti = HdrType & 0x80;
                
                PciPutInfo(bus, dev, fun);
            }
        }
    }
}
/*-----------------------------------------------------------*/




/*-------------------------------------------------------------
-------------------------------------------------------------*/
void
_stdcall
PciPutInfo(
    ulong   Bus,
    ulong   Dev,
    ulong   Fun
    )
{
    ulong   reg;
    ushort  Class;
    ulong   Address;
    
    Class = PciReadConfigWord(Bus, Dev, Fun, PCI_CLASS_DEVICE);
    if ((Class & 0xff00) == 0x300)
    {
        for (reg = 0x10; reg < 0x20; reg += 4)
        {
            Address = PciReadConfigDword(Bus, Dev, Fun, reg);
            
            // bit0 : 为0表示存储器区间，1表示是个I/O区间
            // bit3 : 为1表示对这个区间的操作可以流水线化，即可预取
            if (((Address & 1) == 0) && (Address & 8))
            {
                DisplayReturn();
                DisplayReturn();
                DisplayReturn();
                DisplayColorString("\nPCI: VGA flat buffer : ", 15);
                DisplayInt(Address);
                DisplayReturn();
            }
        }
    }
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/
uchar
_stdcall
PciScanDevice(
    ulong   Bus,
    ulong   Device,
    ulong   Function
    )
{
    ulong   v;
    
    v = PciReadConfigDword(Bus, Device, Function, PCI_VENDOR_ID);
    if (v == 0xffffffff || v == 0 || v == 0xffff || v == 0xffff0000)
        return FALSE;
    
    return TRUE;
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/
uchar
_stdcall
PciCheckDirect()
{
    ulong   f;
    uchar   Status = FALSE;
    
    HalWritePortChar(0xcfb, 1);
    f = HalReadPortDword(0xcf8);
    HalWritePortDword(0xcf8, 0x80000000);
    if (HalReadPortDword(0xcf8) == 0x80000000 && PciSanityCheck())
    {
        Status = TRUE;
    }
    
    HalWritePortDword(0xcf8, f);
    return Status;
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/
uchar
_stdcall
PciSanityCheck()
{
    ushort  DeviceClass;
    ushort  Vendor;
    ulong   dev;
    ulong   fun;
    
    for (dev = 0; dev < 32; dev++)
    {
        for (fun = 0; fun < 8; fun++)
        {
            DeviceClass = PciReadConfigWord(0, dev, fun, PCI_CLASS_DEVICE);
            Vendor = PciReadConfigWord(0, dev, fun, PCI_VENDOR_ID);
            
            if (DeviceClass == PCI_CLASS_BRIDGE_HOST ||
                DeviceClass == PCI_CLASS_DISPLAY_VGA ||
                Vendor == PCI_VENDOR_ID_INTEL ||
                Vendor == PCI_VENDOR_ID_COMPAQ)
            {
                return TRUE;
            }
        }
    }
    
    return FALSE;
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/
uchar
_stdcall
PciReadConfigByte(
    ulong   Bus,
    ulong   Device,
    ulong   Function,
    ulong   Register
    )
{
    HalWritePortDword(0xcf8, CONFIG_CMD(Bus, Device, Function, Register));
    return HalReadPortChar(0xcfc | (Register & 3));
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/
ushort
_stdcall
PciReadConfigWord(
    ulong   Bus,
    ulong   Device,
    ulong   Function,
    ulong   Register
    )
{
    HalWritePortDword(0xcf8, CONFIG_CMD(Bus, Device, Function, Register));
    return HalReadPortWord(0xcfc | (Register & 2));
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/
ulong
_stdcall
PciReadConfigDword(
    ulong   Bus,
    ulong   Device,
    ulong   Function,
    ulong   Register
    )
{
    HalWritePortDword(0xcf8, CONFIG_CMD(Bus, Device, Function, Register));
    return HalReadPortDword(0xcfc);
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/
void
_stdcall
PciWriteConfigByte(
    ulong   Bus,
    ulong   Device,
    ulong   Function,
    ulong   Register,
    uchar   Value
    )
{
    HalWritePortDword(0xcf8, CONFIG_CMD(Bus, Device, Function, Register));
    HalWritePortChar((0xcfc | (Register & 3)), Value);
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/
void
_stdcall
PciWriteConfigWord(
    ulong   Bus,
    ulong   Device,
    ulong   Function,
    ulong   Register,
    ushort  Value
    )
{
    HalWritePortDword(0xcf8, CONFIG_CMD(Bus, Device, Function, Register));
    HalWritePortWord((0xcfc | (Register & 2)), Value);
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/
void
_stdcall
PciWriteConfigDword(
    ulong   Bus,
    ulong   Device,
    ulong   Function,
    ulong   Register,
    ulong   Value
    )
{
    HalWritePortDword(0xcf8, CONFIG_CMD(Bus, Device, Function, Register));
    HalWritePortDword(0xcfc, Value);
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/
/*-----------------------------------------------------------*/



