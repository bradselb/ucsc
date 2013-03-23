#if !defined RTL8139BKS_H
#define RTL8139BKS_H

// make symbolic names for the important registers and 
// for the bits within some of those registers. 
// once again, I have taken considerable inspiration from 
// the production drivers, 8139too.c and 8139cp.c
enum {
    MacAddr = 0, // start of 6 byte mac address
    MultiCastAddr = 0x0008, // there are eight bytes register, must be written 32 bits at a time.
    TxStatus0 = 0x0010, // four 32 bit registers. (aka Tx Status of Descriptor 0)
    TxAddr0 = 0x0020, // four 32 bit registers. Each holds the start address of the respective Tx buffer.
    RxBuf = 0x0030, // receive buffer start address register.
    ChipCmd = 0x0037, // command register
    RxBufPtr = 0x0038, // the present position in the rx buffer? 
    RxByteCount = 0x003A, // others call is RxBufAddr...but data sheet says it is a byte count?
    IntrMask = 0x003c, // interrrupt mask register
    IntrStatus = 0x003E, // interrupt status
    TxConfig = 0x0040, 
    RxConfig = 0x0044, 
    Timer = 0x0048, 
    RxMissedCount = 0x004c, // 24 bit missed packet counter. any write clears
	Cfg9346 = 0x0050,
	Config0 = 0x0051,
	Config1 = 0x0052,
	TimerInt = 0x0054, // a count down timer that generates an interrupt if it reaches zero?
	MediaStatus = 0x0058,
	Config3 = 0x0059,
	Config4 = 0x005A,
	MultiIntr = 0x005C, // multiple interrupt (2 bytes)
    PciRevision = 0x005E, // 0x10 ==> 8139D ? or PCI version 2?
    TxSummary = 0x0060,
	BasicModeCtrl = 0x0062,
	BasicModeStatus	= 0x0064,
    DisconnectCount = 0x006c,
    FalseCarrierSenseCount = 0x006e,
    NwayTest = 0x0070,
    RxErrCount = 0x0072,
    ChipStatusCfg = 0x0074, // two byte chip status and config.
};

// symbolic names for the bits in the Tx Status register

// symbolic names for the bits in the command register
#define CmdRxBufferEmpty (1)
#define CmdTxEnable (0x04)
#define CmdRxEnable (0x08)
#define CmdSoftReset (0x10)

#endif // !defined RTL8139BKS_H
