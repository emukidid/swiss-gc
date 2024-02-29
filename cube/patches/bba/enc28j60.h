/* 
 * Copyright (c) 2023-2024, Extrems <extrems@extremscorner.org>
 * 
 * This file is part of Swiss.
 * 
 * Swiss is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Swiss is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * with Swiss.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef ENC28J60_H
#define ENC28J60_H

#include "dolphin/exi.h"

#define ENC28J60_CMD_RCR(x) ((0x00 | ((x) & 0x1F)) << 24) // Read Control Register
#define ENC28J60_CMD_RBM    ((0x3A) << 24)                // Read Buffer Memory
#define ENC28J60_CMD_WCR(x) ((0x40 | ((x) & 0x1F)) << 24) // Write Control Register
#define ENC28J60_CMD_WBM    ((0x7A) << 24)                // Write Buffer Memory
#define ENC28J60_CMD_BFS(x) ((0x80 | ((x) & 0x1F)) << 24) // Bit Field Set
#define ENC28J60_CMD_BFC(x) ((0xA0 | ((x) & 0x1F)) << 24) // Bit Field Clear
#define ENC28J60_CMD_SRC    ((0xFF) << 24)                // System Reset Command

#define ENC28J60_EXI_SPEED(cmd) (((cmd) == ENC28J60_CMD_SRC) ? EXI_SPEED1MHZ : EXI_SPEED32MHZ)
#define ENC28J60_EXI_DUMMY(cmd) ((((cmd) >> 24) & 0x1F) < 0x1A)

#define ENC28J60_INIT_ERXST (0)
#define ENC28J60_INIT_ERXND (4096 - 1)
#define ENC28J60_INIT_ETXST (4096)
#define ENC28J60_INIT_ETXND (8192 - 1)

typedef enum {
	ENC28J60_ERDPTL   = 0x00, // Read Pointer Low Byte
	ENC28J60_ERDPTH,          // Read Pointer High Byte
	ENC28J60_EWRPTL,          // Write Pointer Low Byte
	ENC28J60_EWRPTH,          // Write Pointer High Byte
	ENC28J60_ETXSTL,          // TX Start Low Byte
	ENC28J60_ETXSTH,          // TX Start High Byte
	ENC28J60_ETXNDL,          // TX End Low Byte
	ENC28J60_ETXNDH,          // TX End High Byte
	ENC28J60_ERXSTL,          // RX Start Low Byte
	ENC28J60_ERXSTH,          // RX Start High Byte
	ENC28J60_ERXNDL,          // RX End Low Byte
	ENC28J60_ERXNDH,          // RX End High Byte
	ENC28J60_ERXRDPTL,        // RX RD Pointer Low Byte
	ENC28J60_ERXRDPTH,        // RX RD Pointer High Byte
	ENC28J60_ERXWRPTL,        // RX WR Pointer Low Byte
	ENC28J60_ERXWRPTH,        // RX WR Pointer High Byte
	ENC28J60_EDMASTL,         // DMA Start Low Byte
	ENC28J60_EDMASTH,         // DMA Start High Byte
	ENC28J60_EDMANDL,         // DMA End Low Byte
	ENC28J60_EDMANDH,         // DMA End High Byte
	ENC28J60_EDMADSTL,        // DMA Destination Low Byte
	ENC28J60_EDMADSTH,        // DMA Destination High Byte
	ENC28J60_EDMACSL,         // DMA Checksum Low Byte
	ENC28J60_EDMACSH,         // DMA Checksum High Byte

	ENC28J60_EHT0     = 0x20, // Hash Table Byte 0
	ENC28J60_EHT1,            // Hash Table Byte 1
	ENC28J60_EHT2,            // Hash Table Byte 2
	ENC28J60_EHT3,            // Hash Table Byte 3
	ENC28J60_EHT4,            // Hash Table Byte 4
	ENC28J60_EHT5,            // Hash Table Byte 5
	ENC28J60_ETH6,            // Hash Table Byte 6
	ENC28J60_ETH7,            // Hash Table Byte 7
	ENC28J60_EPMM0,           // Pattern Match Mask Byte 0
	ENC28J60_EPMM1,           // Pattern Match Mask Byte 1
	ENC28J60_EPMM2,           // Pattern Match Mask Byte 2
	ENC28J60_EPMM3,           // Pattern Match Mask Byte 3
	ENC28J60_EPMM4,           // Pattern Match Mask Byte 4
	ENC28J60_EPMM5,           // Pattern Match Mask Byte 5
	ENC28J60_EPMM6,           // Pattern Match Mask Byte 6
	ENC28J60_EPMM7,           // Pattern Match Mask Byte 7
	ENC28J60_EPMCSL,          // Pattern Match Checksum Low Byte
	ENC28J60_EPMCSH,          // Pattern Match Checksum High Byte
	ENC28J60_EPMOL    = 0x34, // Pattern Match Offset Low Byte
	ENC28J60_EPMOH,           // Pattern Match Offset High Byte
	ENC28J60_ERXFCON  = 0x38, // Ethernet Receive Filter Control Register
	ENC28J60_EPKTCNT,         // Ethernet Packet Count

	ENC28J60_MACON1   = 0x40, // MAC Control Register 1
	ENC28J60_MACON3   = 0x42, // MAC Control Register 3
	ENC28J60_MACON4,          // MAC Control Register 4
	ENC28J60_MABBIPG,         // Back-to-Back Inter-Packet Gap
	ENC28J60_MAIPGL   = 0x46, // Non-Back-to-Back Inter-Packet Gap Low Byte
	ENC28J60_MAIPGH,          // Non-Back-to-Back Inter-Packet Gap High Byte
	ENC28J60_MACLCON1,        // Retransmission Maximum
	ENC28J60_MACLCON2,        // Collision Window
	ENC28J60_MAMXFLL,         // Maximum Frame Length Low Byte
	ENC28J60_MAMXFLH,         // Maximum Frame Length High Byte
	ENC28J60_MICMD    = 0x52, // MII Command Register
	ENC28J60_MIREGADR = 0x54, // MII Register Address
	ENC28J60_MIWRL    = 0x56, // MII Write Data Low Byte
	ENC28J60_MIWRH,           // MII Write Data High Byte
	ENC28J60_MIRDL,           // MII Read Data Low Byte
	ENC28J60_MIRDH,           // MII Read Data High Byte

	ENC28J60_MAADR5   = 0x60, // MAC Address Byte 5
	ENC28J60_MAADR6,          // MAC Address Byte 6
	ENC28J60_MAADR3,          // MAC Address Byte 3
	ENC28J60_MAADR4,          // MAC Address Byte 4
	ENC28J60_MAADR1,          // MAC Address Byte 1
	ENC28J60_MAADR2,          // MAC Address Byte 2
	ENC28J60_EBSTSD,          // Built-in Self-Test Fill Seed
	ENC28J60_EBSTCON,         // Ethernet Self-Test Control Register
	ENC28J60_EBSTCSL,         // Built-in Self-Test Checksum Low Byte
	ENC28J60_EBSTCSH,         // Built-in Self-Test Checksum High Byte
	ENC28J60_MISTAT,          // MII Status Register
	ENC28J60_EREVID   = 0x72, // Ethernet Revision ID
	ENC28J60_ECOCON   = 0x75, // Clock Output Control Register
	ENC28J60_EFLOCON  = 0x77, // Ethernet Flow Control Register
	ENC28J60_EPAUSL,          // Pause Timer Value Low Byte
	ENC28J60_EPAUSH,          // Pause Timer Value High Byte

	ENC28J60_EIE      = 0xFB, // Ethernet Interrupt Enable Register
	ENC28J60_EIR,             // Ethernet Interrupt Request Register
	ENC28J60_ESTAT,           // Ethernet Status Register
	ENC28J60_ECON2,           // Ethernet Control Register 2
	ENC28J60_ECON1,           // Ethernet Control Register 1
} ENC28J60Reg;

typedef enum {
	ENC28J60_ERDPT    = 0x00, // Read Pointer
	ENC28J60_EWRPT    = 0x02, // Write Pointer
	ENC28J60_ETXST    = 0x04, // TX Start
	ENC28J60_ETXND    = 0x06, // TX End
	ENC28J60_ERXST    = 0x08, // RX Start
	ENC28J60_ERXND    = 0x0A, // RX End
	ENC28J60_ERXRDPT  = 0x0C, // RX RD Pointer
	ENC28J60_ERXWRPT  = 0x0E, // RX WR Pointer
	ENC28J60_EDMAST   = 0x10, // DMA Start
	ENC28J60_EDMAND   = 0x12, // DMA End
	ENC28J60_EDMADST  = 0x14, // DMA Destination
	ENC28J60_EDMACS   = 0x16, // DMA Checksum

	ENC28J60_EPMCS    = 0x30, // Pattern Match Checksum
	ENC28J60_EPMO     = 0x34, // Pattern Match Offset

	ENC28J60_MAIPG    = 0x46, // Non-Back-to-Back Inter-Packet Gap
	ENC28J60_MAMXFL   = 0x4A, // Maximum Frame Length
	ENC28J60_MIWR     = 0x56, // MII Write Data
	ENC28J60_MIRD     = 0x58, // MII Read Data

	ENC28J60_EBSTCS   = 0x68, // Built-in Self-Test Checksum
	ENC28J60_EPAUS    = 0x78, // Pause Timer Value
} ENC28J60Reg16;

#define ENC28J60_ERXFCON_UCEN               (1 << 7) // Unicast Filter Enable
#define ENC28J60_ERXFCON_ANDOR              (1 << 6) // AND/OR Filter Select
#define ENC28J60_ERXFCON_CRCEN              (1 << 5) // Post-Filter CRC Check Enable
#define ENC28J60_ERXFCON_PMEN               (1 << 4) // Pattern Match Filter Enable
#define ENC28J60_ERXFCON_MPEN               (1 << 3) // Magic Packet Filter Enable
#define ENC28J60_ERXFCON_HTEN               (1 << 2) // Hash Table Filter Enable
#define ENC28J60_ERXFCON_MCEN               (1 << 1) // Multicast Filter Enable
#define ENC28J60_ERXFCON_BCEN               (1 << 0) // Broadcast Filter Enable

#define ENC28J60_MACON1_TXPAUS              (1 << 3) // Pause Control Frame Transmission Enable
#define ENC28J60_MACON1_RXPAUS              (1 << 2) // Pause Control Frame Reception Enable
#define ENC28J60_MACON1_PASSALL             (1 << 1) // Pass All Received Frames Enable
#define ENC28J60_MACON1_MARXEN              (1 << 0) // MAC Receive Enable

#define ENC28J60_MACON3_PADCFG(x) (((x) & 0x7) << 5) // Automatic Pad and CRC Configuration
#define ENC28J60_MACON3_TXCRCEN             (1 << 4) // Transmit CRC Enable
#define ENC28J60_MACON3_PHDREN              (1 << 3) // Proprietary Header Enable
#define ENC28J60_MACON3_HFRMEN              (1 << 2) // Huge Frame Enable
#define ENC28J60_MACON3_FRMLNEN             (1 << 1) // Frame Length Checking Enable
#define ENC28J60_MACON3_FULDPX              (1 << 0) // MAC Full-Duplex Enable

#define ENC28J60_MACON4_DEFER               (1 << 6) // Defer Transmission Enable
#define ENC28J60_MACON4_BPEN                (1 << 5) // No Backoff During Backpressure Enable
#define ENC28J60_MACON4_NOBKOFF             (1 << 4) // No Backoff Enable

#define ENC28J60_MICMD_MIISCAN              (1 << 1) // MII Scan Enable
#define ENC28J60_MICMD_MIIRD                (1 << 0) // MII Read Enable

#define ENC28J60_EBSTCON_PSV(x)   (((x) & 0x7) << 5) // Pattern Shift Value
#define ENC28J60_EBSTCON_PSEL               (1 << 4) // Port Select
#define ENC28J60_EBSTCON_TMSEL(x) (((x) & 0x3) << 2) // Test Mode Select
#define ENC28J60_EBSTCON_TME                (1 << 1) // Test Mode Enable
#define ENC28J60_EBSTCON_BISTST             (1 << 0) // Built-in Self-Test Start/Busy

#define ENC28J60_MISTAT_NVALID              (1 << 2) // MII Management Read Data Not Valid
#define ENC28J60_MISTAT_SCAN                (1 << 1) // MII Management Scan Operation
#define ENC28J60_MISTAT_BUSY                (1 << 0) // MII Management Busy

#define ENC28J60_EFLOCON_FULDPXS            (1 << 2) // Read-Only MAC Full-Duplex Shadow
#define ENC28J60_EFLOCON_FCEN(x)  (((x) & 0x3) << 0) // Flow Control Enable

#define ENC28J60_EIE_INTIE                  (1 << 7) // Global INT Interrupt Enable
#define ENC28J60_EIE_PKTIE                  (1 << 6) // Receive Packet Pending Interrupt Enable
#define ENC28J60_EIE_DMAIE                  (1 << 5) // DMA Interrupt Enable
#define ENC28J60_EIE_LINKIE                 (1 << 4) // Link Status Change Interrupt Enable
#define ENC28J60_EIE_TXIE                   (1 << 3) // Transmit Interrupt Enable
#define ENC28J60_EIE_TXERIE                 (1 << 1) // Transmit Error Interrupt Enable
#define ENC28J60_EIE_RXERIE                 (1 << 0) // Receive Error Interrupt Enable

#define ENC28J60_EIR_PKTIF                  (1 << 6) // Receive Packet Pending Interrupt Flag
#define ENC28J60_EIR_DMAIF                  (1 << 5) // DMA Interrupt Flag
#define ENC28J60_EIR_LINKIF                 (1 << 4) // Link Status Change Interrupt Flag
#define ENC28J60_EIR_TXIF                   (1 << 3) // Transmit Interrupt Flag
#define ENC28J60_EIR_TXERIF                 (1 << 1) // Transmit Error Interrupt Flag
#define ENC28J60_EIR_RXERIF                 (1 << 0) // Receive Error Interrupt Flag

#define ENC28J60_ESTAT_INT                  (1 << 7) // INT Interrupt Flag
#define ENC28J60_ESTAT_BUFER                (1 << 6) // Ethernet Buffer Error
#define ENC28J60_ESTAT_LATECOL              (1 << 4) // Late Collision Error
#define ENC28J60_ESTAT_RXBUSY               (1 << 2) // Receive Busy
#define ENC28J60_ESTAT_TXABRT               (1 << 1) // Transmit Abort Error
#define ENC28J60_ESTAT_CLKRDY               (1 << 0) // Clock Ready

#define ENC28J60_ECON2_AUTOINC              (1 << 7) // Automatic Buffer Pointer Increment Enable
#define ENC28J60_ECON2_PKTDEC               (1 << 6) // Packet Decrement
#define ENC28J60_ECON2_PWRSV                (1 << 5) // Power Save Enable
#define ENC28J60_ECON2_VRPS                 (1 << 3) // Voltage Regulator Power Save Enable

#define ENC28J60_ECON1_TXRST                (1 << 7) // Transmit Logic Reset
#define ENC28J60_ECON1_RXRST                (1 << 6) // Receive Logic Reset
#define ENC28J60_ECON1_DMAST                (1 << 5) // DMA Start and Busy Status
#define ENC28J60_ECON1_CSUMEN               (1 << 4) // DMA Checksum Enable
#define ENC28J60_ECON1_TXRTS                (1 << 3) // Transmit Request to Send
#define ENC28J60_ECON1_RXEN                 (1 << 2) // Receive Enable
#define ENC28J60_ECON1_BSEL(x)    (((x) & 0x3) << 0) // Bank Select

typedef enum {
	ENC28J60_PHCON1   = 0x00, // PHY Control Register 1
	ENC28J60_PHSTAT1,         // Physical Layer Status Register 1
	ENC28J60_PHID1,           // PHY Identifier Register 1
	ENC28J60_PHID2,           // PHY Identifier Register 2
	ENC28J60_PHCON2   = 0x10, // PHY Control Register 2
	ENC28J60_PHSTAT2,         // Physical Layer Status Register 2
	ENC28J60_PHIE,            // PHY Interrupt Enable Register
	ENC28J60_PHIR,            // PHY Interrupt Request Register
	ENC28J60_PHLCON,          // PHY Module LED Control Register
} ENC28J60PHYReg;

#define ENC28J60_PHCON1_PRST               (1 << 15) // PHY Software Reset
#define ENC28J60_PHCON1_PLOOPBK            (1 << 14) // PHY Loopback
#define ENC28J60_PHCON1_PPWRSV             (1 << 11) // PHY Power-Down
#define ENC28J60_PHCON1_PDPXMD             (1 <<  8) // PHY Duplex Mode

#define ENC28J60_PHSTAT1_PFDPX             (1 << 12) // PHY Full-Duplex Capable
#define ENC28J60_PHSTAT1_PHDPX             (1 << 11) // PHY Half-Duplex Capable
#define ENC28J60_PHSTAT1_LLSTAT            (1 <<  2) // PHY Latching Link Status
#define ENC28J60_PHSTAT1_JBSTAT            (1 <<  1) // PHY Latching Jabber Status

#define ENC28J60_PHCON2_FRCLNK             (1 << 14) // PHY Force Linkup
#define ENC28J60_PHCON2_TXDIS              (1 << 13) // Twisted-Pair Transmitter Disable
#define ENC28J60_PHCON2_JABBER             (1 << 10) // Jabber Correction Disable
#define ENC28J60_PHCON2_HDLDIS             (1 <<  8) // PHY Half-Duplex Loopback Disable

#define ENC28J60_PHSTAT2_TXSTAT            (1 << 13) // PHY Transmit Status
#define ENC28J60_PHSTAT2_RXSTAT            (1 << 12) // PHY Receive Status
#define ENC28J60_PHSTAT2_COLSTAT           (1 << 11) // PHY Collision Status
#define ENC28J60_PHSTAT2_LSTAT             (1 << 10) // PHY Link Status
#define ENC28J60_PHSTAT2_DPXSTAT           (1 <<  9) // PHY Duplex Status
#define ENC28J60_PHSTAT2_PLRITY            (1 <<  5) // Polarity Status

#define ENC28J60_PHIE_PLNKIE               (1 <<  4) // PHY Link Change Interrupt Enable
#define ENC28J60_PHIE_PGEIE                (1 <<  1) // PHY Global Interrupt Enable

#define ENC28J60_PHIR_PLNKIF               (1 <<  4) // PHY Link Change Interrupt Flag
#define ENC28J60_PHIR_PGIF                 (1 <<  2) // PHY Global Interrupt Flag

#define ENC28J60_PHLCON_LACFG(x) (((x) & 0xF) <<  8) // LEDA Configuration
#define ENC28J60_PHLCON_LBCFG(x) (((x) & 0xF) <<  4) // LEDB Configuration
#define ENC28J60_PHLCON_LFRQ(x)  (((x) & 0x3) <<  2) // LED Pulse Stretch Time Configuration
#define ENC28J60_PHLCON_STRCH              (1 <<  1) // LED Pulse Stretching Enable

#endif /* ENC28J60_H */
