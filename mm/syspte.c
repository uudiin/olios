
/*-------------------------------------------------------------
    系统PTE区域的管理支持

    修改时间列表:
    2011-01-08 15:00:40  
    2011-01-14 22:37:24 
-------------------------------------------------------------*/


#include    "ke.h"
#include    "proto.h"


/*-------------------------------------------------------------
    系统 PTE区域初始化
-------------------------------------------------------------*/

void
_stdcall
MiInitSystemPtes(
    PMMPTE  StartingPte,
    ulong   NumberOfPtes
    )
{
    ulong       i;
    PMMPTE      PointerPte;
    
    assert(MmNumberOfSystemPtes != 0);
    
    MmPteSystemPteBase = StartingPte;
    MmPteSystemPteEnd = StartingPte + NumberOfPtes - 1;
    
    memset(StartingPte, 0, NumberOfPtes * sizeof(MMPTE));
    
    StartingPte->List.NextEntry = MM_EMPTY_PTE_LIST;
    
    MmFirstFreeSystemPte.Long = 0;
    MmFirstFreeSystemPte.List.NextEntry = StartingPte - (PMMPTE)PTE_BASE;
    
    if (NumberOfPtes == 1)
    {
        StartingPte->List.OneEntry = 1;
    }
    else
    {
        StartingPte += 1;
        StartingPte->Long = 0;
        StartingPte->List.NextEntry = NumberOfPtes;
    }
    
    MmTotalFreeSystemPtes = NumberOfPtes;
    
    MiLowestSystemPteVirtualAddress = MiGetVirtualAddressMappedByPte(MmPteSystemPteBase);
    MmTotalSystemPtes = NumberOfPtes;
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    在系统 PTE区域中保留一段空间
-------------------------------------------------------------*/

PMMPTE
_stdcall
MiReserveSystemPtes(
    ulong   NumberOfPtes
    )
{
    PMMPTE      PointerPte;
    PMMPTE      Previous;
    PMMPTE      PointerFollowingPte;
    ulong       SizeInSet;
    
    PointerPte = &MmFirstFreeSystemPte;
    if (PointerPte->List.NextEntry == MM_EMPTY_PTE_LIST)
        return 0;
        
    Previous = PointerPte;
    PointerPte = (PMMPTE)PTE_BASE + PointerPte->List.NextEntry;
    
    while (1)
    {
        if (PointerPte->List.OneEntry)
        {
            if (NumberOfPtes == 1)
                goto _ExactFit;     // 恰好匹配一个块
                
            goto _NextEntry;
        }
        
        PointerFollowingPte = PointerPte + 1;
        SizeInSet = PointerFollowingPte->List.NextEntry;
        
        //
        // 若是一个块中的一部分，则把块截成两部分，后面的部分是需要返回的
        //
        
        if (NumberOfPtes < SizeInSet)
        {
            if ((SizeInSet - NumberOfPtes) == 1)
                PointerPte->List.OneEntry = 1;
            else
                PointerFollowingPte->List.NextEntry = SizeInSet - NumberOfPtes;
                
            MmTotalFreeSystemPtes -= NumberOfPtes;
            
            PointerPte += (SizeInSet - NumberOfPtes);
            
            break;
        }
        
        if (NumberOfPtes == SizeInSet)
        {
        
    _ExactFit:
    
            Previous->List.NextEntry = PointerPte->List.NextEntry;
            
            MmTotalFreeSystemPtes -= NumberOfPtes;
            
            break;
        }
        
    _NextEntry:
        
        if (PointerPte->List.NextEntry == MM_EMPTY_PTE_LIST)
            return 0;
            
        Previous = PointerPte;
        PointerPte = (PMMPTE)PTE_BASE + PointerPte->List.NextEntry;
    }
    
    return PointerPte;
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
    释放系统 PTE区域中分配的空间
-------------------------------------------------------------*/

void
_stdcall
MiReleaseSystemPtes(
    PMMPTE  StartingPte,
    ulong   NumberOfPtes
    )
{
    PMMPTE      PointerPte;
    PMMPTE      PointerFollowingPte;
    PMMPTE      NextPte;
    ulong       Size;
    ulong       PteOffset;
    
    memset(StartingPte, 0, NumberOfPtes * sizeof(MMPTE));
    
    PteOffset = StartingPte - (PMMPTE)PTE_BASE;
    
    MmTotalFreeSystemPtes += NumberOfPtes;
    
    PointerPte = &MmFirstFreeSystemPte;
    
    while (1)
    {
        NextPte = (PMMPTE)PTE_BASE + PointerPte->List.NextEntry;
        
        if (PteOffset < PointerPte->List.NextEntry)
        {
            assert(((StartingPte + NumberOfPtes) <= NextPte) || (PointerPte->List.NextEntry == MM_EMPTY_PTE_LIST));
            
            PointerFollowingPte = PointerPte + 1;
            
            if (PointerPte->List.OneEntry)
                Size = 1;
            else
                Size = PointerFollowingPte->List.NextEntry;
                
            if ((PointerPte + Size) == StartingPte)
            {
                // 可以合并PTE簇
                
                NumberOfPtes += Size;
                PointerFollowingPte->List.NextEntry = NumberOfPtes;
                PointerPte->List.OneEntry = 0;
                
                StartingPte = PointerPte;
            }
            else
            {
                StartingPte->List.NextEntry = PointerPte->List.NextEntry;
                
                PointerPte->List.NextEntry = PteOffset;
                
                if (NumberOfPtes == 1)
                {
                    StartingPte->List.OneEntry = 1;
                }
                else
                {
                    StartingPte->List.OneEntry = 0;
                    PointerFollowingPte = StartingPte + 1;
                    PointerFollowingPte->List.NextEntry = NumberOfPtes;
                }
            }
            
            if ((StartingPte + NumberOfPtes) == NextPte)
            {
                StartingPte->List.NextEntry = NextPte->List.NextEntry;
                StartingPte->List.OneEntry = 0;
                PointerFollowingPte = StartingPte + 1;
                
                if (NextPte->List.OneEntry)
                {
                    Size = 1;
                }
                else
                {
                    NextPte += 1;
                    Size = NextPte->List.NextEntry;
                }
                
                PointerFollowingPte->List.NextEntry = NumberOfPtes + Size;
            }
            
            return;
        }
        
        PointerPte = NextPte;
    }
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/
/*-----------------------------------------------------------*/















