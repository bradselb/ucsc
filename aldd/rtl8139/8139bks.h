#if !defined RTL8139BKS_H
#define RTL8139BKS_H

// make symbolic names for the important registers and 
// for the bits within some of those registers. 
// once again, I have taken considerable inspiration from 
// the production drivers, 8139too.c and 8139cp.c
enum RegisterNames {
    EthernetAddr = 0, // start of 6 byte hw address
    MultiCastAddr = 0x0008, // there are eight bytes register, must be written 32 bits at a time.
    TxStatus0 = 0x0010, // four 32 bit registers. (aka Tx Status of Descriptor 0)
    TxAddr0 = 0x0020, // four 32 bit registers. Each holds the start address of the respective Tx buffer.
    RxBufStartAddr = 0x0030, // receive buffer start address register.
    ChipCmd = 0x0037, // command register
    RxBufOffset = 0x0038, // the present position in the rx buffer? 
    RxByteCount = 0x003A, // aka RxBufAddr...apparently, this is where the device is in the rx buffer? 
    IntrMask = 0x003c, // interrrupt mask register
    IntrStatus = 0x003E, // interrupt status register
    TxConfig = 0x0040, 
    RxConfig = 0x0044, 
    TimerCount = 0x0048, // a counter/timer that counts up from zero. Reads clear it.
    RxMissedCount = 0x004c, // 24 bit missed packet counter. any write clears
    Cfg9346 = 0x0050,
    Config0 = 0x0051,
    Config1 = 0x0052,
    TimerInterval = 0x0054, // 32 bit value. when timer reaches this value, an interrupt is generated. 
    MediaStatus = 0x0058,
    Config3 = 0x0059,
    Config4 = 0x005A,
    MultiIntr = 0x005C, // multiple interrupt (2 bytes)
    PciRevision = 0x005E,
    TxSummary = 0x0060,
    BasicModeCtrl = 0x0062,
    BasicModeStatus = 0x0064,
    DisconnectCount = 0x006c,
    FalseCarrierSenseCount = 0x006e,
    NwayTest = 0x0070,
    RxErrCount = 0x0072,
    ChipStatusCfg = 0x0074, // two byte chip status and config.
};

// symbolic names for the bits in the Tx Status register

// symbolic names for the bits in the command register
enum ChipCommandBits {
    RxBufferEmpty = (1<<0), // read only.
    CmdTxEnable = (1<<2),
    CmdRxEnable = (1<<3),
    CmdSoftReset = (1<<4),
    // bits 5,6,7 are reserved.
};

// interrrupt status and mask register bits
enum InterruptStatusBits {
    RxOK = (1<<0),
    RxError = (1<<1),
    TxOK = (1<<2),
    TxError = (1<<3),
    RxBufferOverflow = (1<<4),
    PacketUnderRunLinkChange = (1<<5),
    RxFifoOverflow = (1<<6),
    // bits [7..12] reserved
    CableLengthChange = (1<<13),
    TimerTimedOut = (1<<14),
    SystemError = (1<<15),

    RxAckBits = (RxFifoOverflow | RxBufferOverflow | RxOK),
}; 

// Tx Status register bits
enum TxStatusBits {
    TxStatusTxByteCountMask = (0x1fff), // bits [0..12]
    TxStatusOwn = (1<<13),
    TxStatusFifoUnderrun = (1<<14),
    TxStatusTxOk = (1<<15),
    TxStatusEarlyTxThreshShift = (16),
    TxStatusEarlyTxThreshMask = (0x3f << TxStatusEarlyTxThreshShift),
    TxStatusCollisionCountShift = (24),
    TxStatusCollisionCountMask = (0x0f << TxStatusCollisionCountShift),
    TxStatusOutOfWindow = (1<<29),
    TxStatusTxAbort = (1<<30),
    TxStatusCarrierSenseLost = (1<<31),
    TxStatusTxComplete = (TxStatusTxOk | TxStatusTxAbort), 
};

// sixteen bits of RX status is prepended to the received packet
enum RxStatusBits {
    RxStatusOk = (1<<0),
    RxStatusFrameAlignmentError = (1<<1),
    RxStatusCrcError = (1<<2),
    RxStatusLongPacket = (1<<3),
    RxStatusRuntPacket = (1<<4),
    RxStatusInvalidSymbolError = (1<<5),
    RxStatusReserved = (0x7f<<6),
    RxStatusBroadcastAddr = (1<<13),
    RxStatusPhysicalAddr = (1<<14),
    RxStatusMulticastAddr = (1<<15),
};

#endif // !defined RTL8139BKS_H
