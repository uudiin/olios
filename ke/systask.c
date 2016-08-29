
/*-------------------------------------------------------------
    系统任务线程
    
    修改时间列表:
    2011-02-27 14:11:21 
-------------------------------------------------------------*/


#include    "ke.h"
#include    "proto.h"


/*-------------------------------------------------------------
-------------------------------------------------------------*/
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/

void
_stdcall
KeSystemTask(
    ulong   Context
    )
{
    MESSAGE     Msg;
    ulong       SrcTid;
    
    KeRegisterTaskThread(TASK_SYSTEM);
    
    while (TRUE)
    {
        DisplayColorString("sss", 9);
        KeSendRecvMsg(RECEIVE, ANY, &Msg);
        SrcTid = Msg.SourceTid;
        
        switch (Msg.Type)
        {
            case SYS_GET_TICKS:
                Msg.Reserved = KiSystemTicks;       // FIXME
                KeSendRecvMsg(SEND, SrcTid, &Msg);
                break;
            
            default:
                KiBugCheck("ThreadSystemTask: Unknown msg type\n");
                break;
        }
    }
}
/*-----------------------------------------------------------*/














