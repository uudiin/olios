
/*-------------------------------------------------------------
    进程线程间通信
    
    修改时间列表:
    2011-01-23 01:53:07 
    2011-02-27 23:06:55 
-------------------------------------------------------------*/


#include    "ke.h"
#include    "proto.h"


/*-------------------------------------------------------------
-------------------------------------------------------------*/
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/

ulong
_stdcall
KeSendRecvMsg(
    ulong       ControlCode,
    ulong       SrcDest,        // TID
    PMESSAGE    Msg
    )
{
    ulong       Status;
    PKTHREAD    CurrentThread;
    
    CurrentThread = PsGetCurrentThread();
    Msg->SourceTid = PsGetCurrentThreadId();
    
    if (ControlCode == BOTH)
    {
        Status = KiSendMsg(CurrentThread, SrcDest, Msg);
        if (Status == FALSE)
        {
            KiRecvMsg(CurrentThread, SrcDest, Msg);
        }
    }
    else if (ControlCode == SEND)
    {
        Status = KiSendMsg(CurrentThread, SrcDest, Msg);
    }
    else if (ControlCode == RECEIVE)
    {
        memset(Msg, 0, sizeof(MESSAGE));
        Status = KiRecvMsg(CurrentThread, SrcDest, Msg);
    }
    else
    {
        KiBugCheck("KeSendRecvMsg: invalid parameter\n");
    }
    
    return Status;
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/

ulong
_stdcall
KiSendMsg(
    PKTHREAD    SenderThread,
    ulong       DstTid,     // TID
    PMESSAGE    Msg
    )
{
    PKTHREAD    DestThread;
    
    DestThread = PsLookupProcessThreadById(DstTid);
    
    assert(DestThread != 0);
    assert(SenderThread != DestThread);
    
    if (KeIsDeadLock(SenderThread, DestThread))
    {
        KiBugCheck("KiSendMsg: a dead lock occured\n");
    }
    
    //
    // 目标正在等待当前的发送进程或者任意进程消息
    //
    
    if (DestThread->Receiving &&
        (DestThread->ReceiveFrom == SenderThread->Tid ||
        DestThread->ReceiveFrom == ANY))
    {
        assert(DestThread->Msg);
        assert(Msg);
        
        memcpy(DestThread->Msg, Msg, sizeof(MESSAGE));
        
        DestThread->Msg = NULL;
        DestThread->Receiving = FALSE;
        DestThread->ReceiveFrom = 0;
        
        KiReadyThread(DestThread);
        
        assert(DestThread->Flags == 0);
        assert(DestThread->SendTo == 0);
        assert(SenderThread->Flags == 0);
        assert(SenderThread->Msg == 0);
        assert(SenderThread->ReceiveFrom == 0);
        assert(SenderThread->SendTo == 0);
    }
    else
    {
        SenderThread->Sending = TRUE;
        SenderThread->SendTo = DestThread->Tid;
        SenderThread->Msg = Msg;
        
        InsertTailList(&DestThread->SenderListHead, &SenderThread->SenderListEntry);
        
        //
        // 进行一次线程调度,阻塞当前发送线程
        //
        
        KeSwapThread(0, 0);   // FIXME
    }
    
    return 0;   // FIXME
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/

ulong
_stdcall
KiRecvMsg(
    PKTHREAD    RecvThread,
    ulong       SenderTid,
    PMESSAGE    Msg
    )
{
    PKTHREAD    SenderThread;
    PLIST_ENTRY Entry;
    ulong       CopyOk = FALSE;
    
    //
    // 该线程准备接收一个中断，且有一个中断消息需要该线程处理
    //
    
    if (RecvThread->HasInterruptMsg && (SenderTid == ANY || SenderTid == INTERRUPT))
    {
        MESSAGE     InterruptMsg;
        
        memset(&InterruptMsg, 0, sizeof(MESSAGE));
        InterruptMsg.SourceTid = INTERRUPT;
        InterruptMsg.Type = HARDWARE_INTERRUPT;
        
        memcpy(Msg, &InterruptMsg, sizeof(MESSAGE));
        RecvThread->HasInterruptMsg = FALSE;
        
        return 0;
    }
    
    //
    //
    //
    
    if (SenderTid == ANY)
    {
        //
        // 等待任意线程的消息，若有发送线程排队，则取出最前面一个线程的消息
        //
        
        if (IsListEmpty(&RecvThread->SenderListHead) == FALSE)
        {
            Entry = RemoveHeadList(&RecvThread->SenderListHead);
            SenderThread = CONTAINING_RECORD(Entry, KTHREAD, SenderListEntry);
            CopyOk = TRUE;
        }
    }
    else
    {
        //
        // 等待指定任务的消息
        //
        
        SenderThread = PsLookupProcessThreadById(SenderTid);
        
        if (SenderThread->Sending && (SenderThread->SendTo == RecvThread->Tid))
        {
            CopyOk = TRUE;
        }
    }
    
    //
    //
    //
    
    if (CopyOk)
    {
        RemoveEntryList(&SenderThread->SenderListEntry);
        
        memcpy(Msg, SenderThread->Msg, sizeof(MESSAGE));
        
        SenderThread->Msg = NULL;
        SenderThread->SendTo = NULL;
        SenderThread->Sending = FALSE;
        
        KiReadyThread(SenderThread);
    }
    else
    {
    //    DisplayColorString("[Waiting for receive]", 9);
        
        RecvThread->Receiving = TRUE;
        RecvThread->Msg = Msg;
        
        RecvThread->ReceiveFrom = SenderTid;
        
        assert(RecvThread->SendTo == NULL);
        assert(RecvThread->HasInterruptMsg == FALSE);
        
        //
        // 进行一次调度，阻塞该发关进程
        //
        
        KeSwapThread(0, 0);   // 同上   FIXME
    }
    
    return 0;
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/

ulong
_stdcall
KeIsDeadLock(
    PKTHREAD    SrcThread,
    PKTHREAD    DstThread
    )
{
    PKTHREAD    TempThread;
    
    TempThread = DstThread;
    
    while (TRUE)
    {
        if (TempThread->Sending)
        {
            if (TempThread->SendTo == SrcThread->Tid)
                return TRUE;
        
            TempThread = PsLookupProcessThreadById(TempThread->SendTo);
            
            continue;
        }
        
        break;
    }
    
    return FALSE;
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/

void
_stdcall
IoInformInterrupt(
    PKINTERRUPT     Interrupt
    )
{
    PKTHREAD    Thread;
    
    Thread = Interrupt->ServiceThread;
    
    if ((Thread->Receiving) && ((Thread->ReceiveFrom == ANY) || (Thread->ReceiveFrom == INTERRUPT)))
    {
        Thread->Msg->SourceTid = INTERRUPT;
        Thread->Msg->Type = HARDWARE_INTERRUPT;
        Thread->Msg = NULL;
        Thread->HasInterruptMsg = FALSE;
        Thread->Receiving = FALSE;
        Thread->ReceiveFrom = 0;       // 0是系统任务 FIXME
        
        assert(Thread->Flags == 0);
        
        KiReadyThread(Thread);
        
        assert(Thread->Flags == 0);
        assert(Thread->Msg == 0);
        assert(Thread->ReceiveFrom == 0);
        assert(Thread->SendTo == 0);
    }
    else
    {
        Thread->HasInterruptMsg = TRUE;
    }
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/
/*-----------------------------------------------------------*/





















