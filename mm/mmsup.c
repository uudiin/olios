
/*-------------------------------------------------------------
    内存管理器支持函数

    修改时间列表:
    2011-01-18 20:12:18 
-------------------------------------------------------------*/


#include    "ke.h"
#include    "proto.h"



/*-------------------------------------------------------------
    映射IO地址空间到虚拟内存
-------------------------------------------------------------*/
pvoid
_stdcall
MmMapIoSpace(
    pvoid   PhysicalAddress,
    ulong   NumberOfBytes
    )
{
    PMMPTE      PointerPte;
    pvoid       BaseVa;
    MMPTE       TempPte;
    ulong       NumberOfPages;
    ulong       PageFrameIndex;
    
    NumberOfPages = ((ulong)PhysicalAddress & (PAGE_SIZE - 1) + NumberOfBytes + (PAGE_SIZE - 1)) >> PAGE_SHIFT;
    
    PageFrameIndex = (ulong)((ulong)PhysicalAddress >> PAGE_SHIFT);
    
    PointerPte = MiReserveSystemPtes(NumberOfPages);
    if (PointerPte == 0)
    {
        return 0;
    }
    
    BaseVa = (pvoid)MiGetVirtualAddressMappedByPte(PointerPte);
    
    TempPte = MmValidKernelPte;
    
    do
    {
        assert(PointerPte->Hard.Valid == 0);
        
        TempPte.Hard.PageFrameNumber = PageFrameIndex;
        *PointerPte = TempPte;
        
        PointerPte += 1;
        PageFrameIndex += 1;
        NumberOfPages -= 1;
    } while (NumberOfPages);
    
    return BaseVa;
}
/*-----------------------------------------------------------*/


/*-------------------------------------------------------------
    取消IO地址空间的映射
-------------------------------------------------------------*/
void
_stdcall
MmUnmapIoSpace(
    pvoid   BaseAddress,
    ulong   NumberOfBytes
    )
{
    PMMPTE      PointerPte;
    ulong       NumberOfPages;
    
    assert(NumberOfBytes != 0);
    
    NumberOfPages = ((ulong)BaseAddress & (PAGE_SIZE - 1) + NumberOfBytes + (PAGE_SIZE - 1)) >> PAGE_SHIFT;
    
    PointerPte = MiGetPteAddress(BaseAddress);
    
    MiReleaseSystemPtes(PointerPte, NumberOfPages);
    
    return;
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    创建进程地址空间
-------------------------------------------------------------*/

ulong
_stdcall
MmCreateProcessAddressSpace()
{
    return;
}
/*-----------------------------------------------------------*/






















