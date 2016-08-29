
/*-------------------------------------------------------------
    处理 Page Fault 错误

    修改时间列表:
    2011-01-09 00:02:08 
    2011-01-09 22:04:13 
-------------------------------------------------------------*/


#include    "ke.h"
#include    "proto.h"


/*-------------------------------------------------------------
-------------------------------------------------------------*/
/*-----------------------------------------------------------*/




/*-------------------------------------------------------------
    Fage Fault 页错误处理
    
    输入参数
      ErrorCode         页错误的错误码
      VirtualAddress    发生错误的线性地址
      PreviousMode      发生错误时CPU所处的模式
      TrapFrame         陷阱帧
-------------------------------------------------------------*/

void
_stdcall
MmAccessFault(
    ulong       ErrorCode,
    pvoid       VirtualAddress,
    ulong       PreviousMode,
    PKTRAP_FRAME    TrapFrame
    )
{
    CurrentDisplayPosition = 0;
    
    DisplayString("\nPage Fault occured\n");
    DisplayString("Eip = ");
    DisplayInt((ulong)(TrapFrame->Eip));
    DisplayString(" ,  ErrorCode =");
    DisplayInt(ErrorCode);
    
    DisplayString("\nReferenceAddress = ");
    DisplayInt((ulong)(VirtualAddress));
    
    if (ErrorCode & 1)
    {
        DisplayString("\nThe fault was caused by a page-level protection violation");
    }
    else
    {
        DisplayString("\nThe fault was caused by a nonpresent page");
    }
    
    if (ErrorCode & 2)
    {
        DisplayString("\nThe access causing the fault was a write");
    }
    else
    {
        DisplayString("\nThe access causing the fault was a read");
    }
}
/*-----------------------------------------------------------*/


















