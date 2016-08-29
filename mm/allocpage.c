
/*-------------------------------------------------------------
    系统非分页池管理，支持页面粒度的分配和释放

    修改时间列表:
    2011-01-08 15:00:07  
    2011-01-10 22:24:29 
    2011-01-12 20:26:59 
-------------------------------------------------------------*/


#include    "ke.h"
#include    "proto.h"


/*-------------------------------------------------------------
    初始化非分页内存池管理结构
    把整个非换页池空间所有页面均加入第一个页面所在的 MMFREE_POOL_ENTRY项
-------------------------------------------------------------*/

void
_stdcall
MiInitNonPagedPool()
{
    PMMFREE_POOL_ENTRY  FreeEntry;
    PMMFREE_POOL_ENTRY  FirstEntry;
    PMMPTE      PointerPte;
    ulong       PagesInPool;
    ulong       Index;
    
    // 初始化空闲页面链表头
    
    for (Index = 0; Index < MI_MAX_FREE_LIST_HEADS; Index++)
    {
        InitializeListHead(&MmNonPagedPoolFreeListHead[Index]);
    }
    
    assert((MmNonPagedPoolStart & 0xfff) == 0);
    
    FreeEntry = (PMMFREE_POOL_ENTRY)MmNonPagedPoolStart;
    FirstEntry = FreeEntry;
    
    PagesInPool = NOPAGED_POOL_SIZE >> PAGE_SHIFT;
    
    MmNumberOfFreeNonPagedPool = PagesInPool;
    
    //
    // 页面数从1开始算起，索引从0开始
    //
    
    Index = MmNumberOfFreeNonPagedPool - 1;
    if (Index >= MI_MAX_FREE_LIST_HEADS)
    {
        Index = MI_MAX_FREE_LIST_HEADS - 1;
    }
    
    // 将所有的页面加入到对应的链表中
    
    InsertHeadList(&MmNonPagedPoolFreeListHead[Index], &FreeEntry->List);
    
    FreeEntry->Size = PagesInPool;
    FreeEntry->Signature = MI_FREE_POOL_SIGNATURE;
    FreeEntry->Owner = FirstEntry;
    
    // 将第一个页面后面的Owner设成指向第一个ENTRY，即第一个页面
    
    while (PagesInPool > 1)
    {
        FreeEntry = (PMMFREE_POOL_ENTRY)((pchar)FreeEntry + PAGE_SIZE);
        FreeEntry->Signature = MI_FREE_POOL_SIGNATURE;
        FreeEntry->Owner = FirstEntry;
        PagesInPool -= 1;
    }
    
    
    //
    // 初始化非分页池的开始和结束页帧
    //
    
    PointerPte = MiGetPteAddress(MmNonPagedPoolStart);
    assert(PointerPte->Hard.Valid == 1);
    MiStartOfNonPagedPoolFrame = PointerPte->Hard.PageFrameNumber;
    
    PointerPte = MiGetPteAddress(MmNonPagedPoolStart + NOPAGED_POOL_SIZE - 1);
    assert(PointerPte->Hard.Valid == 1);
    MiEndOfNonPagedPoolFrame = PointerPte->Hard.PageFrameNumber;
}
/*-----------------------------------------------------------*/


/*-------------------------------------------------------------
    非分页池分配(以页为单位)
-------------------------------------------------------------*/

pvoid
_stdcall
MiAllocatePoolPages(
    ulong   SizeInBytes
    )
{
    ulong           SizeInPages;
    ulong           Index;
    PLIST_ENTRY     ListHead;
    PLIST_ENTRY     LastListHead;
    PLIST_ENTRY     Entry;
    PMMFREE_POOL_ENTRY  FreePageEntry;
    PMMPTE          PointerPte;
    PMMPFN          Pfn1;
    ulong           PageFrameIndex;
    pvoid           BaseVa;
    
    SizeInPages = BYTES_TO_PAGES(SizeInBytes);
    Index = SizeInPages - 1;
    if (Index >= MI_MAX_FREE_LIST_HEADS)
    {
        Index = MI_MAX_FREE_LIST_HEADS - 1;
    }
    
    //
    // 非分页内存池通过它们自身的页相互链起来
    //
    
    ListHead = &MmNonPagedPoolFreeListHead[Index];
    LastListHead = &MmNonPagedPoolFreeListHead[MI_MAX_FREE_LIST_HEADS];
    
    do
    {
        Entry = ListHead->Flink;
        
        while (Entry != ListHead)
        {
            FreePageEntry = CONTAINING_RECORD(Entry, MMFREE_POOL_ENTRY, List);
            
            assert(FreePageEntry->Signature == MI_FREE_POOL_SIGNATURE);
            
            if (FreePageEntry->Size >= SizeInPages)
            {
                //
                // 这个入口项有足够的空间，移出末尾的页
                //
                
                FreePageEntry->Size -= SizeInPages;
                
                BaseVa = (pvoid)((pchar)FreePageEntry + (FreePageEntry->Size << PAGE_SHIFT));
                
                RemoveEntryList(&FreePageEntry->List);
                
                //
                // 若有剩余页面,把剩余页面插入到相应的链表
                //
                
                if (FreePageEntry->Size != 0)
                {
                    Index = FreePageEntry->Size - 1;
                    if (Index >= MI_MAX_FREE_LIST_HEADS)
                    {
                        Index = MI_MAX_FREE_LIST_HEADS - 1;
                    }
                    
                    InsertTailList(&MmNonPagedPoolFreeListHead[Index], &FreePageEntry->List);
                }
                
                //
                // 调整非分页池的剩余空闲页面数量
                //
                
                MmNumberOfFreeNonPagedPool -= SizeInPages;
                assert(MmNumberOfFreeNonPagedPool);
                
                //
                // 在PFN数据库中为开始页和结束页作上分配标记
                //
                
                PointerPte = MiGetPteAddress(BaseVa);
                assert(PointerPte->Hard.Valid == 1);
                PageFrameIndex = PointerPte->Hard.PageFrameNumber;
                
                Pfn1 = MiPfnElement(PageFrameIndex);
                assert(Pfn1->u3.ShortFlags.StartOfAllocation == 0);
                Pfn1->u3.ShortFlags.StartOfAllocation = 1;
                
                if (SizeInPages != 1)
                {
                    if (PointerPte == 0)
                    {
                        Pfn1 += SizeInPages - 1;
                    }
                    else
                    {
                        PointerPte += SizeInPages - 1;
                        assert(PointerPte->Hard.Valid == 1);
                        Pfn1 = MiPfnElement(PointerPte->Hard.PageFrameNumber);
                    }
                }
                
                assert(Pfn1->u3.ShortFlags.EndOfAllocation == 0);
                Pfn1->u3.ShortFlags.EndOfAllocation = 1;
                
                return BaseVa;
            }
            
            Entry = FreePageEntry->List.Flink;
        }
        
        ListHead += 1;
    } while (ListHead < LastListHead);
    
    return 0;
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    非分页池内存释放
    
    输入参数:
        StartingAddress 要释放的内存起始地址
        
    返回值: 释放的页的个数
-------------------------------------------------------------*/

ulong
_stdcall
MiFreePoolPages(
    pvoid   StartingAddress
    )
{
    PMMPFN      Pfn1;
    PMMPFN      StartPfn;
    ulong       Index;
    ulong       i;
    ulong       NumberOfPages;
    PMMPTE      PointerPte;
    PMMPTE      StartPte;
    PMMFREE_POOL_ENTRY  Entry;
    PMMFREE_POOL_ENTRY  NextEntry;
    PMMFREE_POOL_ENTRY  LastEntry;
    
    if ((ulong)StartingAddress < MmNonPagedPoolStart || ((ulong)StartingAddress >= MmNonPagedPoolStart + NOPAGED_POOL_SIZE))
    {
        KiBugCheck("MiFreePoolPages: expand the nonpaged pool\n");
    }
    
    PointerPte = MiGetPteAddress(StartingAddress);
    Pfn1 = MiPfnElement(PointerPte->Hard.PageFrameNumber);
    if (Pfn1->u3.ShortFlags.StartOfAllocation == 0)
    {
        KiBugCheck("MiFreePoolPages: StartingAddress is not start of pool\n");
    }
    
    //
    // 在PFN数据库中找到分配的结束标志,清除
    //
    
    StartPfn = Pfn1;
    StartPte = PointerPte;
    
    while (Pfn1->u3.ShortFlags.EndOfAllocation == 0)
    {
        PointerPte += 1;
        Pfn1 = MiPfnElement(PointerPte->Hard.PageFrameNumber);
    }
        
    NumberOfPages = PointerPte - StartPte + 1;
    
    StartPfn->u3.ShortFlags.StartOfAllocation = 0;
    Pfn1->u3.ShortFlags.EndOfAllocation = 0;
    
    MmNumberOfFreeNonPagedPool += NumberOfPages;
    
    i = NumberOfPages;
    
    //
    // 若紧临后面的页空闲，则合并
    //
    
    if (MiPfnElementToIndex(Pfn1) == MiEndOfNonPagedPoolFrame)
    {
        PointerPte += 1;
        Pfn1 = 0;
    }
    else
    {
        PointerPte += 1;
        if (PointerPte->Hard.Valid == 1)
        {
            Pfn1 = MiPfnElement(PointerPte->Hard.PageFrameNumber);
        }
        else
        {
            Pfn1 = 0;
        }
    }
    
    if ((Pfn1 != 0) && (Pfn1->u3.ShortFlags.StartOfAllocation == 0))
    {
        Entry = (PMMFREE_POOL_ENTRY)((pchar)StartingAddress + (NumberOfPages << PAGE_SHIFT));
        assert(Entry->Signature == MI_FREE_POOL_SIGNATURE);
        assert(Entry->Owner == Entry);
        
        i += Entry->Size;
        RemoveEntryList(&Entry->List);
    }
    
    //
    // 若该区域前面的页空闲，则合并
    //
    
    Entry = (PMMFREE_POOL_ENTRY)StartingAddress;
    assert(MiStartOfNonPagedPoolFrame != 0);
    
    if (MiPfnElementToIndex(StartPfn) == MiStartOfNonPagedPoolFrame)
    {
        Pfn1 = 0;
    }
    else
    {
        PointerPte -= (NumberOfPages + 1);      // 此时的PointerPte指向要释放的区域的后面那一页
        if (PointerPte->Hard.Valid == 1)
        {
            Pfn1 = MiPfnElement(PointerPte->Hard.PageFrameNumber);
        }
        else
        {
            Pfn1 = 0;
        }
    }
    
    if (Pfn1 != 0 && Pfn1->u3.ShortFlags.EndOfAllocation == 0)
    {
        Entry = (PMMFREE_POOL_ENTRY)((pchar)StartingAddress - PAGE_SIZE);
        assert(Entry->Signature == MI_FREE_POOL_SIGNATURE);
        Entry = Entry->Owner;
        
        if (Entry->Size < MI_MAX_FREE_LIST_HEADS)
        {
            RemoveEntryList(&Entry->List);
            
            Entry->Size += i;
            
            Index = (ulong)(Entry->Size - 1);
            if (Index >= MI_MAX_FREE_LIST_HEADS)
            {
                Index = MI_MAX_FREE_LIST_HEADS - 1;
            }
            
            InsertTailList(&MmNonPagedPoolFreeListHead[Index], &Entry->List);
        }
        else
        {
            Entry->Size += i;
        }
    }
    
    //
    // 不能跟前面的页合并，将这些页插入相应的链表
    //
    
    if (Entry == (PMMFREE_POOL_ENTRY)StartingAddress)
    {
        Entry->Size = i;
        Index = (ulong)(Entry->Size - 1);
        if (Index >= MI_MAX_FREE_LIST_HEADS)
        {
            Index = MI_MAX_FREE_LIST_HEADS - 1;
        }
        
        InsertTailList(&MmNonPagedPoolFreeListHead[Index], &Entry->List);
    }
    
    //
    // 设定后续页的Owner
    //
    
    assert(i != 0);
    NextEntry = (PMMFREE_POOL_ENTRY)StartingAddress;
    LastEntry = (PMMFREE_POOL_ENTRY)((pchar)NextEntry + (i << PAGE_SHIFT));
    
    do
    {
        NextEntry->Owner = Entry;
        NextEntry->Signature = MI_FREE_POOL_SIGNATURE;
        NextEntry = (PMMFREE_POOL_ENTRY)((pchar)NextEntry + PAGE_SIZE);
    } while (NextEntry != LastEntry);
    
    return (ulong)NumberOfPages;
}
/*-----------------------------------------------------------*/




/*-------------------------------------------------------------
-------------------------------------------------------------*/


/*-----------------------------------------------------------*/
















