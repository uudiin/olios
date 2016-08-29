
#ifndef	_ATAPI_H_
#define	_ATAPI_H_


#define     ATAPI_OPEN          1


// 端口
#define     PORT_HD_DATA        0x1f0
#define     PORT_HD_ERR_FEATURES    0x1f1
#define     PORT_HD_NSECTOR     0x1f2
#define     PORT_HD_LBA_LOW     0x1f3
#define     PORT_HD_LBA_MID     0x1f4
#define     PORT_HD_LBA_HIGH    0x1f5
#define     PORT_HD_DEVICE      0x1f6
#define     PORT_HD_STATUS_CMD  0x1f7
#define     PORT_HD_DEV_CTRL    0x3f6
#define     PORT_HD_DRV_ADDR    0x3f7

// 命令
#define     CMD_ATA_IDENTIFY    0xec
#define     CMD_ATA_READ        0x20
#define     CMD_ATA_WRITE       0x30

// 标志位
#define     FLAG_STATUS_BSY     0x80
#define     FLAG_STATUS_DRDY    0x40
#define     FLAG_STATUS_DFSE    0x20
#define     FLAG_STATUS_DSC     0x10
#define     FLAG_STATUS_DRQ     0x08
#define     FLAG_STATUS_CORR    0x04
#define     FLAG_STATUS_IDX     0x02
#define     FLAG_STATUS_ERR     0x01


typedef struct _HD_CMD_BLK
{
    uchar    Features;
    uchar    Count;
    uchar    LbaLow;
    uchar    LbaMid;
    uchar    LbaHigh;
    uchar    Device;
    uchar    Command;
    uchar    uuu;
} HD_CMD_BLK, *PHD_CMD_BLK;


#define	MAKE_DEVICE_REG(Lba, Drive, LbaHighest) \
            (((Lba) << 6) | ((Drive) << 4) | ((LbaHighest) & 0xf) | 0xa0)


void    _stdcall  IoInitAtapi(PKTHREAD);
void    _stdcall  IoIrq14AtapiService(PKINTERRUPT);
void    _stdcall  AtapiIdentify(ulong);
void    _stdcall  AtapiCommandOut(PHD_CMD_BLK);
ulong   _stdcall  AtapiWaitForEmpty();
void    _stdcall  AtapiPrintIdentifyInfo(short*);


#endif	// _ATAPI_H_

















