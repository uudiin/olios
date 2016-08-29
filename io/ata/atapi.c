
/*-------------------------------------------------------------
    atapi磁盘任务线程
    
    修改时间列表:
    2011-02-27 18:34:43 
-------------------------------------------------------------*/


#include    "ke.h"
#include    "proto.h"
#include    "atapi.h"


ulong       NumberOfDisk;
ulong       HarddiskStatus;
short       HarddiskInfo[256];


/*-------------------------------------------------------------
-------------------------------------------------------------*/
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/

void
_stdcall
IoAtapiTask(
    ulong   Context
    )
{
    MESSAGE     Msg;
    ulong       SrcTid;
    
    KeRegisterTaskThread(TASK_ATA_WINI);
    
    IoInitAtapi(PsGetCurrentThread());
    
    while (TRUE)
    {
        KeSendRecvMsg(RECEIVE, ANY, &Msg);
        SrcTid = Msg.SourceTid;
        
        switch (Msg.Type)
        {
            case ATAPI_OPEN:
                AtapiIdentify(1);
                break;
            
            default:
                KiBugCheck("IoAtapiTask:unknown msg type\n");
                break;
        }
        
        KeSendRecvMsg(SEND, SrcTid, &Msg);
    }
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/

void
_stdcall
IoInitAtapi(
    PKTHREAD    ServiceThread
    )
{
//    NumberOfDisk = (ulong)(*(char*)0x475);    // FIXME 这里得映射
    
    KeConnectInterrupt(IRQ14_ATA_WINI, IoIrq14AtapiService, ServiceThread);
    
    HalEnableIrq(IRQ2_CASCADE);
    HalEnableIrq(IRQ14_ATA_WINI);
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/

void
_stdcall
IoIrq14AtapiService(
    PKINTERRUPT     Interrupt
    )
{
    HarddiskStatus = (ulong)HalReadPortChar(PORT_HD_STATUS_CMD);
    
    IoInformInterrupt(Interrupt);
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/

void
_stdcall
AtapiIdentify(
    ulong   Drive
    )
{
    HD_CMD_BLK      CommandBlock;
    
    memset(&CommandBlock, 0, sizeof(HD_CMD_BLK));
    CommandBlock.Device = MAKE_DEVICE_REG(0, Drive, 0);
    CommandBlock.Command = CMD_ATA_IDENTIFY;
    AtapiCommandOut(&CommandBlock);
    
    IoWaitInterrupt();
    HalReadPort(PORT_HD_DATA, HarddiskInfo, 512);
    AtapiPrintIdentifyInfo(HarddiskInfo);
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/

void
_stdcall
AtapiCommandOut(
    PHD_CMD_BLK     Command
    )
{
    if (!AtapiWaitForEmpty())
    {
        KiBugCheck("AtapiCommandOut: harddisk is busy\n");
    }
    
    //
    // 打开硬盘控制寄存器的IEN位，激活中断
    //
    
    HalWritePortChar(PORT_HD_DEV_CTRL, 0);
    
    //
    // 向命令块寄存器输入需要的参数
    //
    
    HalWritePortChar(PORT_HD_ERR_FEATURES, Command->Features);
    HalWritePortChar(PORT_HD_NSECTOR, Command->Count);
    HalWritePortChar(PORT_HD_LBA_LOW, Command->LbaLow);
    HalWritePortChar(PORT_HD_LBA_MID, Command->LbaMid);
    HalWritePortChar(PORT_HD_LBA_HIGH, Command->LbaHigh);
    HalWritePortChar(PORT_HD_DEVICE, Command->Device);
    
    //
    // 把命令写入命令寄存器
    //
    
    HalWritePortChar(PORT_HD_STATUS_CMD, Command->Command);
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/

ulong
_stdcall
AtapiWaitForEmpty()
{
    ulong   i;
    ulong   Status = 0;
    
    for (i = 0; i < 0xff00; i++)
    {
        Status = (ulong)HalReadPortChar(PORT_HD_STATUS_CMD);
        
        if ((Status & FLAG_STATUS_BSY) == 0)
            return TRUE;
    }
    
    return FALSE;
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/

void
_stdcall
AtapiPrintIdentifyInfo(
    short*  hdinfo
    )
{
	int capabilities = hdinfo[49];
	DisplayColorString ("\nLBA supported: ", 15);
	DisplayColorString (((capabilities & 0x0200) ? "Yes" : "No"), 15);

	int cmd_set_supp = hdinfo[83];
	DisplayColorString ("\nLBA48 supported: ", 15);
	DisplayColorString (((cmd_set_supp & 0x0400) ? "Yes" : "No"), 15);

	int sectors = ((int)hdinfo[61] << 16) + hdinfo[60];
	DisplayColorString ("\nHD size: ", 15);
	PrintInt (sectors * 512 / 1000000);
}
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/
/*-----------------------------------------------------------*/



/*-------------------------------------------------------------
-------------------------------------------------------------*/
/*-----------------------------------------------------------*/































