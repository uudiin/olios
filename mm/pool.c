
/*-------------------------------------------------------------
    任意大小内存池分配支持

    修改时间列表:
    2011-01-15 02:01:32 
    2011-01-15 13:47:26 
    2011-01-16 01:06:11 
-------------------------------------------------------------*/


#include    "ke.h"
#include    "proto.h"



/*-------------------------------------------------------------
-------------------------------------------------------------*/
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    高层的非换页池初始化
-------------------------------------------------------------*/

void
_stdcall
MmInitializeNonPagedPool()
{
    PLIST_ENTRY     ListEntry;
    PLIST_ENTRY     LastListEntry;
    PPOOL_DESCRIPTOR    PoolDesc;
    
    //
    // 初始化非换页池的池描述述结构
    //
    
    PoolDesc = &MmNonPagedPoolDescriptor;
    
    PoolDesc->RunningAllocs = 0;
    PoolDesc->RunningDeAllocs = 0;
    PoolDesc->TotalPages = 0;
    PoolDesc->TotalBytes = 0;
    PoolDesc->LockAddress = 0;
    PoolDesc->PendingFrees = 0;
    PoolDesc->PendingFreeDepth = 0;
    
    ListEntry = PoolDesc->ListHeads;
    LastListEntry = ListEntry + POOL_LIST_HEADS;
    
    while (ListEntry < LastListEntry)
    {
        InitializeListHead(ListEntry);
        ListEntry += 1;
    }
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    任意粒度的非换页内存分配函数
-------------------------------------------------------------*/

pvoid
_stdcall
ExAllocatePoolWithTag(
    ulong   NumberOfBytes,
    ulong   Tag
    )
{
    PPOOL_HEADER    Entry;
    PPOOL_HEADER    NextEntry;
    PPOOL_HEADER    SplitEntry;
    PPOOL_DESCRIPTOR    PoolDesc;
    PLIST_ENTRY     ListHead;
    ulong           ListNumber;
    ulong           NeededSize;
    ulong           NumberOfPages;
    ulong           Index;
    pvoid           Block;
    
    PoolDesc = &MmNonPagedPoolDescriptor;
    
    //
    // 如果请求的大小可以分配一个页，直接从底层池中分配页面后返回
    // 如此分配得到的地址是页面对齐的，低12位为0
    //
    
    if (NumberOfBytes > POOL_BUDDY_MAX)
    {
        Entry = (PPOOL_HEADER)MiAllocatePoolPages(NumberOfBytes);
        if (Entry == 0)
        {
            KiBugCheck("ExAllocatePoolWithTag: have no enough physical memory\n");
        }
        
        NumberOfPages = BYTES_TO_PAGES(NumberOfBytes);
        
        // 这里应该使用互锁操作
        
        PoolDesc->TotalBytes += (NumberOfPages << PAGE_SHIFT);
        PoolDesc->RunningAllocs++;
        
        return Entry;
    }
    
    if (NumberOfBytes == 0)
    {
        KiBugCheck("ExAllocatePoolWithTag: invalid parameter\n");
    }
    
    //
    // 为请求分配的大小计算链表头所在数组的索引
    //
    
    ListNumber = ((NumberOfBytes + sizeof(POOL_HEADER) + (POOL_SMALLEST_BLOCK - 1)) >> POOL_BLOCK_SHIFT);
    
    NeededSize = ListNumber;
    
    ListHead = &PoolDesc->ListHeads[ListNumber];
    
    do
    {
        //
        // 如果链表不空，则从选定的链表中分配一个块
        //
        
        if (IsListEmpty(ListHead) == 0)
        {
            //
            // 从链表中取出一项，并让Entry指向POOL_HEADER
            //
            
            Block = RemoveHeadList(ListHead);
            Entry = (PPOOL_HEADER)((pchar)Block - sizeof(POOL_HEADER));
            
            assert(Entry->BlockSize >= NeededSize);
            assert(Entry->PoolType == 0);
            
            if (Entry->BlockSize != NeededSize)
            {
                //
                // 找到的块的大小大于分配请求大小，分割这个块并把剩余的碎片插入到相应链表
                //
                // 如果入口在一个页的开始，为了减小碎片率把该块前面的部分返回
                // 否则把后面的部分返回
                //
                
                if (Entry->PreviousSize == 0)
                {
                    // 这个入口在一个页的开始
                    
                    SplitEntry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry + NeededSize);
                    SplitEntry->BlockSize = Entry->BlockSize - NeededSize;
                    SplitEntry->PreviousSize = NeededSize;
                    
                    //
                    // 如果分配的块不在一个页的结尾，调整下一个块的大小
                    //
                    
                    NextEntry = (PPOOL_HEADER)((PPOOL_BLOCK)SplitEntry + SplitEntry->BlockSize);
                    
                    if (PAGE_ALIGNED(NextEntry) == 0)
                    {
                        NextEntry->PreviousSize = SplitEntry->BlockSize;
                    }
                }
                else
                {
                    //
                    // 入口不在页的开始
                    //
                    
                    SplitEntry = Entry;
                    Entry->BlockSize = Entry->BlockSize - NeededSize;
                    Entry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry + Entry->BlockSize);
                    Entry->PreviousSize = SplitEntry->BlockSize;
                    
                    NextEntry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry + NeededSize);
                    if (PAGE_ALIGNED(NextEntry) == 0)
                    {
                        NextEntry->PreviousSize = NeededSize;
                    }
                }
                
                //
                // 设置分配入口的大小，并把分割后的块插入到合适的空闲链表
                // Entry总是指向要分配的块
                //
                
                Entry->BlockSize = NeededSize;
                SplitEntry->PoolType = 0;
                Index = SplitEntry->BlockSize;
                
                if (SplitEntry->BlockSize != 1)
                {
                    InsertTailList(&PoolDesc->ListHeads[Index - 1], ((PLIST_ENTRY)((pchar)SplitEntry + sizeof(POOL_HEADER))));
                }
            }
            
            Entry->PoolType = 1;        // 置上已分配标志
            
            PoolDesc->RunningAllocs++;
            PoolDesc->TotalBytes += (Entry->BlockSize << POOL_BLOCK_SHIFT);
            
            Entry->PoolTag = Tag;
            
            //
            // 清除内部管理结构的LIST_ENTRY
            //
            
            ((pulong)((pchar)Entry + sizeof(POOL_HEADER)))[0] = 0;
            ((pulong)((pchar)Entry + sizeof(POOL_HEADER)))[1] = 0;
            
            return (pchar)Entry + sizeof(POOL_HEADER);
        }
        
        //
        // 链表为空，从下一个更大块的链表来分配
        //
        
        ListHead += 1;
    } while (ListHead != &PoolDesc->ListHeads[POOL_LIST_HEADS]);
    
    //
    // 没有找到期望大小的内存块，且没有更大的可以被分割的块来满足分配
    // 通过分配额外的页到非换页池来扩大POOL
    //
    
    Entry = (PPOOL_HEADER)MiAllocatePoolPages(PAGE_SIZE);
    if (Entry == 0)
    {
        KiBugCheck("ExAllocatePoolWithTag: have no enough physical memory\n");
    }
    
    //
    // 为新分配的页初始化 pool header
    //
    
    Entry->Long = 0;
    Entry->BlockSize = NeededSize;
    
    Entry->PoolType = 1;            // 置上分配标志
    
    SplitEntry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry + NeededSize);
    
    SplitEntry->Long = 0;
    
    Index = (PAGE_SIZE / sizeof(POOL_BLOCK)) - NeededSize;
    
    SplitEntry->BlockSize = Index;
    SplitEntry->PreviousSize = NeededSize;
    
    //
    // 分割分配的页，把剩余的碎片插入到合适的链表
    //
    
    PoolDesc->TotalPages++;
    PoolDesc->TotalBytes += NeededSize;
    
    NeededSize <<= POOL_BLOCK_SHIFT;
    
    //
    // 仅把能容得下POOL_HEADER和LIST_ENTRY大小的碎片块插入链表
    //
    
    if (SplitEntry->BlockSize != 1)
    {
        InsertTailList(&PoolDesc->ListHeads[Index - 1], ((PLIST_ENTRY)((pchar)SplitEntry + sizeof(POOL_HEADER))));
    }
    
    PoolDesc->RunningAllocs++;
    
    Block = (pvoid)((pchar)Entry + sizeof(POOL_HEADER));
    
    Entry->PoolTag = Tag;
    
    return Block;
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    释放内存分配
-------------------------------------------------------------*/

void
_stdcall
ExFreePoolWithTag(
    pvoid   p,
    ulong   Tag
    )
{
    PPOOL_DESCRIPTOR    PoolDesc;
    PPOOL_HEADER    Entry;
    PPOOL_HEADER    NextEntry;
    ulong           Combined;
    ulong           BlockSize;

    PoolDesc = &MmNonPagedPoolDescriptor;
    
    //
    // 如果入口是页对齐的，直接释放到系统内存池，否则释放到高层分配链表
    //
    
    if (PAGE_ALIGNED(p))
    {
        PoolDesc->RunningDeAllocs++;
        
        MiFreePoolPages(p);
        
        return;
    }
    
    //
    // 对齐入口地址到池分配边界
    //
    
    Entry = (PPOOL_HEADER)((pchar)p - sizeof(POOL_HEADER));
    
    BlockSize = Entry->BlockSize;
    
    //
    // 释放这个内存块
    //
    
    NextEntry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry + BlockSize);
    
    if ((PAGE_ALIGNED(NextEntry) == 0) && (BlockSize != NextEntry->PreviousSize))
    {
        KiBugCheck("ExFreePoolWithTag: bad pool header\n");
    }
    
    Combined = 0;
    
    assert(BlockSize == Entry->BlockSize);
    
    PoolDesc->RunningDeAllocs++;
    PoolDesc->TotalBytes -= (BlockSize << POOL_BLOCK_SHIFT);
    
    //
    // 检查后面的内存块是否是空闲的
    //
    
    if (PAGE_ALIGNED(NextEntry) == 0)
    {
        if (NextEntry->PoolType == 0)
        {
            Combined = 1;
            
            if (NextEntry->BlockSize != 1)
            {
                RemoveEntryList((PLIST_ENTRY)((pchar)NextEntry + sizeof(POOL_HEADER)));
            }
            
            Entry->BlockSize = Entry->BlockSize + NextEntry->BlockSize;
        }
    }
    
    //
    // 检查前面的内存块是否是空闲的
    //
    
    if (Entry->PreviousSize != 0)
    {
        NextEntry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry - Entry->PreviousSize);
        if (NextEntry->PoolType == 0)
        {
            Combined = 1;
            
            if (NextEntry->BlockSize != 1)
            {
                RemoveEntryList((PLIST_ENTRY)((pchar)NextEntry + sizeof(POOL_HEADER)));
            }
            
            NextEntry->BlockSize = NextEntry->BlockSize + Entry->BlockSize;
            
            Entry = NextEntry;
        }
    }
    
    //
    // 如果正在释放的内存块可以被合并成一个完整的页，也归还该页到内存管理器
    //
    
    if (PAGE_ALIGNED(Entry) && PAGE_ALIGNED((PPOOL_BLOCK)Entry + Entry->BlockSize))
    {
        PoolDesc->TotalPages--;
        
        MiFreePoolPages(Entry);
    }
    else
    {
        //
        // 组合成一个大块插入到合适的链表中
        //
        
        Entry->PoolType = 0;
        BlockSize = Entry->BlockSize;
        
        assert(BlockSize != 1);
        
        if (Combined)
        {
            NextEntry = (PPOOL_HEADER)((PPOOL_BLOCK)Entry + BlockSize);
            if (PAGE_ALIGNED(NextEntry) == 0)
            {
                NextEntry->PreviousSize = BlockSize;
            }
        }
        
        //
        // 总是插入到链表头，希望复用缓存线
        //
        
        InsertHeadList(&PoolDesc->ListHeads[BlockSize - 1], ((PLIST_ENTRY)((pchar)Entry + sizeof(POOL_HEADER))));
    }
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/

pvoid
_stdcall
ExAllocatePool(
    ulong   NumberOfBytes
    )
{
    return ExAllocatePoolWithTag(NumberOfBytes, DEFAULT_TAG);
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/

void
_stdcall
ExFreePool(
    pvoid   p
    )
{
    return ExFreePoolWithTag(p, 0);
}
/*-----------------------------------------------------------*/














