
/*-------------------------------------------------------------
    内存管理初始化

    修改时间列表:
    2011-01-07 22:04:17 
    2011-01-08 03:13:54 
    2011-01-09 01:18:07 
    2011-01-09 22:01:04 
-------------------------------------------------------------*/


#include    "ke.h"
#include    "proto.h"


/*-------------------------------------------------------------
-------------------------------------------------------------*/
/*-----------------------------------------------------------*/




/*-------------------------------------------------------------
    内存管理初始化
    
    主要完成如下几项任务：
    1. 设定指定系统空间各区域的全局变量
    2. 清除低 2G空间的映射
    3. 根据LOADER传进来的内存描述符数组，取得内存信息
    4. 为PFN数据库和非换页内存池分配页表及页面
    5. 为系统PTE区域分配页表
    6. 调用建立非分页管理结构和系统PTE区域结构的例程
-------------------------------------------------------------*/

void
_stdcall
MmInitSystem(
    PLOADER_PARAMETER_BLOCK LoaderBlock
    )
{
    MMPTE       TempPde;
    MMPTE       TempPte;
    PMMPTE      StartPde;
    PMMPTE      EndPde;
    PMMPTE      StartPte;
    PMMPTE      LastPte;
    PMMPTE      PointerPte;
    ulong       VirtualAddress;
    ulong       PageFrameIndex;
    ulong       FirstPfnDatabasePage;
    ulong       PagesNeeded;
    PMEMORY_RANGE   MemoryRange;
    PMEMORY_RANGE   MostFreeRange = 0;
    ulong       LowestPhysical = 0xffffffff;
    ulong       HighestPhysical = 0;
    ulong       MostFreeBytes = 0;
    ulong       TotalFreeBytes;
    int         i;
    

    // 设置系统空间开始
    
    MmSystemRangeStart = KSEG0_BASE;
    
    TempPte = MmValidKernelPte;
    TempPde = MmValidKernelPte;
    
    // Unmap 低2G的空间
    
    StartPde = MiGetPdeAddress(0);
    EndPde = MiGetPdeAddress(KSEG0_BASE);
    memset(StartPde, 0, (EndPde - StartPde) * sizeof(MMPTE));
    
    //
    // 遍历LOADER传进来的内存描述符表
    // 得到空闲物理内存的最低对齐地址和物理页个数
    //
    
    MemoryRange = LoaderBlock->MemoryRangeArray;
    TotalFreeBytes = 0;
    
    for (i = 0; i < LoaderBlock->MemRangeCount; i++)
    {
        // 总可用空间的物理页
        
        MmNumberOfPhysicalPages += (MemoryRange->Size >> PAGE_SHIFT);
        
        // 最低物理地址，应该是0
        
        if (MemoryRange->Base < LowestPhysical)
        {
            LowestPhysical = MemoryRange->Base;
        }
        
        // 最高物理地址
        
        if (MemoryRange->Base + MemoryRange->Size > HighestPhysical)
        {
            HighestPhysical = MemoryRange->Base + MemoryRange->Size - 1;
        }
        
        // 不考虑最低的以 0 开始的空间
        if (MemoryRange->Base != 0)
        {
            // 得到描述数组中描述空间最大的那一项
            
            if (MemoryRange->Size > MostFreeBytes)
            {
                MostFreeBytes = MemoryRange->Size;
                MostFreeRange = MemoryRange;
            }
            
            // 总空闲空间,默认不使用低16M空间
            
            if (MemoryRange->Base < _16M)
            {
                if (MemoryRange->Base + MemoryRange->Size > _16M)
                {
                    TotalFreeBytes += (MemoryRange->Size - (_16M - MemoryRange->Base));
                }
            }
            else
            {
                TotalFreeBytes += MemoryRange->Size;
            }
        }
        
        MemoryRange++;
    } // end for
    
    assert(i == 2);
    
    //
    // 忽略低16M，重新计算最大内存描述符信息
    //
    
    MxFreeDescriptor.BasePage = (_16M >> PAGE_SHIFT);
    
    if (MostFreeRange->Base < _16M)
    {
        ulong   TempSize;
        
        if (MostFreeRange->Base + MostFreeRange->Size <= _16M)
        {
            return;     // 内存小于16M，产生严重错误   FIXME
        }
        else
        {
            TempSize = (MostFreeRange->Size - (_16M - MostFreeRange->Base));
            MxFreeDescriptor.PageCount = TempSize >> PAGE_SHIFT;
        }
    }
    else
    {
        MxFreeDescriptor.PageCount = MostFreeRange->Size >> PAGE_SHIFT;
    }
    
    MmLowestPhysicalPage = LowestPhysical >> PAGE_SHIFT;
    MmHighestPhysicalPage = HighestPhysical >> PAGE_SHIFT;
    MmTotalFreePages = TotalFreeBytes >> PAGE_SHIFT;
    
    assert(MmLowestPhysicalPage == 0);
    
    
    //
    // 计算PFN及非换页内存池的地址及PFN本身需要的非换页内存开销    FIXME
    //
    
    MmPfnDatabase = (PMMPFN)PFN_DATA_BASE;
    
    // PFN数据库所占的页数
    MxPfnAllocation = ((MmHighestPhysicalPage * sizeof(MMPFN) + PAGE_SIZE - 1) >> PAGE_SHIFT);
    
    MmNonPagedPoolStart = PFN_DATA_BASE + MxPfnAllocation * PAGE_SIZE;
    
    PagesNeeded = MxPfnAllocation + (NOPAGED_POOL_SIZE >> PAGE_SHIFT);
    
    
    //
    // 真正从 MxFreeDescriptor结构中为PFN数据库和非换页内存池保留物理内存
    //
    
    FirstPfnDatabasePage = MxFreeDescriptor.BasePage;
    MxFreeDescriptor.BasePage += PagesNeeded;
    MxFreeDescriptor.PageCount -= PagesNeeded;
    MmTotalFreePages -= PagesNeeded;
    
    
    //
    // 为系统PTE区域分配页表
    //
    
    MmSystemPteBase = SYSTEM_PTES_BASE;
    MmSystemPteEnd = SYSTEM_PTES_BASE + SYSTEM_PTES_SIZE;
    
    StartPde = MiGetPdeAddress(MmSystemPteBase);
    EndPde = MiGetPdeAddress(MmSystemPteEnd - 1);
    
    while (StartPde <= EndPde)
    {
        TempPde.Hard.PageFrameNumber = MxGetNextPage(1);
        *StartPde = TempPde;
        PointerPte = MiGetVirtualAddressMappedByPte(StartPde);
        memset(PointerPte, 0, PAGE_SIZE);
        StartPde += 1;
    }
    
    
    //
    // 分配PFN数据库和非换页内存池的页表
    //
    
    StartPde = MiGetPdeAddress(MmPfnDatabase);
    
    VirtualAddress = MmNonPagedPoolStart + NOPAGED_POOL_SIZE - 1;
    
    EndPde = MiGetPdeAddress(VirtualAddress);
    
    while (StartPde <= EndPde)
    {
        if (StartPde->Hard.Valid == 0)
        {
            TempPde.Hard.PageFrameNumber = MxGetNextPage(1);
            *StartPde = TempPde;
            PointerPte = MiGetVirtualAddressMappedByPte(StartPde);
            memset(PointerPte, 0, PAGE_SIZE);
        }
        
        StartPde += 1;
    }
    
    
    //
    // 为非换页内存池分配页面
    //
    
    PageFrameIndex = FirstPfnDatabasePage + MxPfnAllocation;
    
    StartPte = MiGetPteAddress(MmNonPagedPoolStart);
    
    LastPte = MiGetPteAddress(MmNonPagedPoolStart + NOPAGED_POOL_SIZE - 1);
    
    while (StartPte <= LastPte)
    {
        TempPte.Hard.PageFrameNumber = PageFrameIndex;
        *StartPte = TempPte;
        StartPte += 1;
        PageFrameIndex += 1;
    }
    
    
    // 非分页池已经映射，建立起管理结构
    
    MiInitNonPagedPool();
    
    MmInitializeNonPagedPool();
    
    
    //
    // 为PFN数据库分配页面并清 0
    //

    StartPte = MiGetPteAddress(MmPfnDatabase);
    VirtualAddress = (ulong)MmPfnDatabase + MxPfnAllocation * PAGE_SIZE - 1;
    LastPte = MiGetPteAddress(VirtualAddress);
    PageFrameIndex = FirstPfnDatabasePage;
    /*
    print_var("\n\n\n\nPagesNeeded          = ", PagesNeeded);
    print_var("FirstPfnDatabasePage = ", FirstPfnDatabasePage);
    print_var("MxPfnAllocation      = ", MxPfnAllocation);
    print_var("PfnDatabase StartPte = ", (ulong)StartPte);
    print_var("PfnDatabase LastPte  = ", (ulong)LastPte);
    print_var("NonPagedPool StartPte= ", (ulong)MiGetPteAddress(MmNonPagedPoolStart));
    print_var("NonPagedPool LastPte = ", (ulong)MiGetPteAddress(MmNonPagedPoolStart + NOPAGED_POOL_SIZE - 1));
    */
    while (StartPte <= LastPte)
    {
        TempPte.Hard.PageFrameNumber = PageFrameIndex;
        *StartPte = TempPte;
        
        StartPte += 1;
        PageFrameIndex += 1;
    }

    memset((pvoid)MmPfnDatabase, 0, MxPfnAllocation << PAGE_SHIFT);
    
    //
    // 建立起系统PTE的内存管理结构
    //
    
    StartPte = MiGetPteAddress(MmSystemPteBase);
    MmNumberOfSystemPtes = MiGetPteAddress(MmSystemPteEnd) - StartPte;
    
    MiInitSystemPtes(StartPte, MmNumberOfSystemPtes);
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    原始的物理内存分配
    从 MxFreeDescriptor 内存描述符表中扣掉要分配的页数
    并调整全局变量 MmTotalFreePages
-------------------------------------------------------------*/

ulong
_stdcall
MxGetNextPage(
    ulong   PagesNeeded
    )
{
    ulong   PageFrameIndex = 0;
    
    if (PagesNeeded > MxFreeDescriptor.PageCount)
    {
        KiBugCheck("MxGetNextPage: have no enough memory to allocate\n");
    }
        
    PageFrameIndex = MxFreeDescriptor.BasePage;
    
    MxFreeDescriptor.BasePage += PagesNeeded;
    MxFreeDescriptor.PageCount -= PagesNeeded;
    MmTotalFreePages -= PagesNeeded;
    
    return PageFrameIndex;
}
/*-----------------------------------------------------------*/



