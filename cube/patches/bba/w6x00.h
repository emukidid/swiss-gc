/* 
 * Copyright (c) 2024-2025, Extrems <extrems@extremscorner.org>
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

#ifndef W6X00_H
#define W6X00_H

#define W6100_CID (0x03000000)
#define W6300_CID (0xFF800000)

#define W6100_BSB(x)        (((x) & 0x1F) << 27) // Block Select Bits
#define W6100_RWB                      (1 << 26) // Read/Write Access Mode Bit
#define W6100_OM(x)          (((x) & 0x3) << 24) // SPI Operation Mode Bits
#define W6300_MOD(x)         (((x) & 0x3) << 22) // QSPI Mode Bits
#define W6300_RWB                      (1 << 21) // Read/Write Access Mode Bit
#define W6300_BSB(x)        (((x) & 0x1F) << 16) // Block Select Bits
#define W6X00_ADDR(x)             ((x) & 0xFFFF) // Offset Address

#define W6X00_REG(addr)        (W6100_BSB(0)         | W6300_BSB(0)         | W6X00_ADDR(addr))
#define W6X00_REG_S(n, addr)   (W6100_BSB(n * 4 + 1) | W6300_BSB(n * 4 + 1) | W6X00_ADDR(addr))
#define W6X00_TXBUF_S(n, addr) (W6100_BSB(n * 4 + 2) | W6300_BSB(n * 4 + 2) | W6X00_ADDR(addr))
#define W6X00_RXBUF_S(n, addr) (W6100_BSB(n * 4 + 3) | W6300_BSB(n * 4 + 3) | W6X00_ADDR(addr))

#define W6100_INIT_S0_TX_BSR (16)
#define W6100_INIT_S0_RX_BSR (16)

#define W6300_INIT_S0_TX_BSR (32)
#define W6300_INIT_S0_RX_BSR (32)

enum {
	Sn_MR      = 0x0000, // Socket n Mode Register
	Sn_PSR     = 0x0004, // Socket n Prefer Source IPv6 Address Register
	Sn_CR      = 0x0010, // Socket n Command Register
	Sn_IR      = 0x0020, // Socket n Interrupt Register
	Sn_IMR     = 0x0024, // Socket n Interrupt Mask Register
	Sn_IRCLR   = 0x0028, // Socket n Interrupt Clear Register
	Sn_SR      = 0x0030, // Socket n Status Register
	Sn_ESR,              // Socket n Extension Status Register
	Sn_PNR     = 0x0100, // Socket n IP Protocol Number Register
	Sn_TOSR    = 0x0104, // Socket n IP TOS Register
	Sn_TTLR    = 0x0108, // Socket n IP TTL Register
	Sn_FRGR    = 0x010C, // Socket n Fragment Offset in IP Header Register
	Sn_MSSR    = 0x0110, // Socket n Maximum Segment Size Register
	Sn_PORTR   = 0x0114, // Socket n Source Port Register
	Sn_DHAR    = 0x0118, // Socket n Destination Hardware Address Register
	Sn_DIPR    = 0x0120, // Socket n Destination IPv4 Address Register
	Sn_DIP6R   = 0x0130, // Socket n Destination IPv6 Address Register
	Sn_DPORTR  = 0x0140, // Socket n Destination Port Register
	Sn_MR2     = 0x0144, // Socket n Mode Register 2
	Sn_RTR     = 0x0180, // Socket n Retransmission Time Register
	Sn_RCR     = 0x0184, // Socket n Retransmission Count Register
	Sn_KPALVTR = 0x0188, // Socket n Keepalive Time Register
	Sn_TX_BSR  = 0x0200, // Socket n TX Buffer Size Register
	Sn_TX_FSR  = 0x0204, // Socket n TX Free Buffer Size Register
	Sn_TX_RD   = 0x0208, // Socket n TX Read Pointer Register
	Sn_TX_WR   = 0x020C, // Socket n TX Write Pointer Register
	Sn_RX_BSR  = 0x0220, // Socket n RX Buffer Size Register
	Sn_RX_RSR  = 0x0224, // Socket n RX Received Size Register
	Sn_RX_RD   = 0x0228, // Socket n RX Read Pointer Register
	Sn_RX_WR   = 0x022C, // Socket n RX Write Pointer Register
};

typedef enum {
	W6X00_CIDR0             = W6X00_REG(0x0000), // Chip Identification Register 0
	W6X00_CIDR1,                                 // Chip Identification Register 1
	W6X00_VER0,                                  // Chip Version Register 0
	W6X00_VER1,                                  // Chip Version Register 1
	W6X00_CIDR2,                                 // Chip Identification Register 2
	W6X00_SYSR              = W6X00_REG(0x2000), // System Status Register
	W6X00_SYCR0             = W6X00_REG(0x2004), // System Configuration Register 0
	W6X00_SYCR1,                                 // System Configuration Register 1
	W6X00_TCNTR0            = W6X00_REG(0x2016), // Tick Counter Register 0
	W6X00_TCNTR1,                                // Tick Counter Register 1
	W6X00_TCNTCLR           = W6X00_REG(0x2020), // Tick Counter Clear Register
	W6X00_IR                = W6X00_REG(0x2100), // Interrupt Register
	W6X00_SIR,                                   // Socket Interrupt Register
	W6X00_SLIR,                                  // Socketless Interrupt Register
	W6X00_IMR               = W6X00_REG(0x2104), // Interrupt Mask Register
	W6X00_IRCLR             = W6X00_REG(0x2108), // Interrupt Clear Register
	W6X00_SIMR              = W6X00_REG(0x2114), // Socket Interrupt Mask Register
	W6X00_SLIMR             = W6X00_REG(0x2124), // Socketless Interrupt Mask Register
	W6X00_SLIRCLR           = W6X00_REG(0x2128), // Socketless Interrupt Clear Register
	W6X00_SLPSR             = W6X00_REG(0x212C), // Socketless Prefer Source IPv6 Address Register
	W6X00_SLCR              = W6X00_REG(0x2130), // Socketless Command Register
	W6X00_PHYSR             = W6X00_REG(0x3000), // PHY Status Register
	W6X00_PHYRAR            = W6X00_REG(0x3008), // PHY Register Address Register
	W6X00_PHYDIR0           = W6X00_REG(0x300C), // PHY Data Input Register 0
	W6X00_PHYDIR1,                               // PHY Data Input Register 1
	W6X00_PHYDOR0           = W6X00_REG(0x3010), // PHY Data Output Register 0
	W6X00_PHYDOR1,                               // PHY Data Output Register 1
	W6X00_PHYACR            = W6X00_REG(0x3014), // PHY Access Control Register
	W6X00_PHYDIVR           = W6X00_REG(0x3018), // PHY Division Register
	W6X00_PHYCR0            = W6X00_REG(0x301C), // PHY Control Register 0
	W6X00_PHYCR1,                                // PHY Control Register 1
	W6X00_NET4MR            = W6X00_REG(0x4000), // Network IPv4 Mode Register
	W6X00_NET6MR            = W6X00_REG(0x4004), // Network IPv6 Mode Register
	W6X00_NETMR0            = W6X00_REG(0x4008), // Network Mode Register 0
	W6X00_NETMR1,                                // Network Mode Register 1
	W6X00_PTMR              = W6X00_REG(0x4100), // PPP LCP Request Timer Register
	W6X00_PMNR              = W6X00_REG(0x4104), // PPP LCP Magic Number Register
	W6X00_PHAR0             = W6X00_REG(0x4108), // PPPoE Server Hardware Address Register 0
	W6X00_PHAR1,                                 // PPPoE Server Hardware Address Register 1
	W6X00_PHAR2,                                 // PPPoE Server Hardware Address Register 2
	W6X00_PHAR3,                                 // PPPoE Server Hardware Address Register 3
	W6X00_PHAR4,                                 // PPPoE Server Hardware Address Register 4
	W6X00_PHAR5,                                 // PPPoE Server Hardware Address Register 5
	W6X00_PSIDR0            = W6X00_REG(0x4110), // PPPoE Session ID Register 0
	W6X00_PSIDR1,                                // PPPoE Session ID Register 1
	W6X00_PMRUR0            = W6X00_REG(0x4114), // PPPoE Maximum Receive Unit Register 0
	W6X00_PMRUR1,                                // PPPoE Maximum Receive Unit Register 1
	W6X00_SHAR0             = W6X00_REG(0x4120), // Source Hardware Address Register 0
	W6X00_SHAR1,                                 // Source Hardware Address Register 1
	W6X00_SHAR2,                                 // Source Hardware Address Register 2
	W6X00_SHAR3,                                 // Source Hardware Address Register 3
	W6X00_SHAR4,                                 // Source Hardware Address Register 4
	W6X00_SHAR5,                                 // Source Hardware Address Register 5
	W6X00_GAR0              = W6X00_REG(0x4130), // Gateway IP Address Register 0
	W6X00_GAR1,                                  // Gateway IP Address Register 1
	W6X00_GAR2,                                  // Gateway IP Address Register 2
	W6X00_GAR3,                                  // Gateway IP Address Register 3
	W6X00_SUBR0,                                 // Subnet Mask Register 0
	W6X00_SUBR1,                                 // Subnet Mask Register 1
	W6X00_SUBR2,                                 // Subnet Mask Register 2
	W6X00_SUBR3,                                 // Subnet Mask Register 3
	W6X00_SIPR0,                                 // IPv4 Source Address Register 0
	W6X00_SIPR1,                                 // IPv4 Source Address Register 1
	W6X00_SIPR2,                                 // IPv4 Source Address Register 2
	W6X00_SIPR3,                                 // IPv4 Source Address Register 3
	W6X00_LLAR0             = W6X00_REG(0x4140), // Link-Local Address Register 0
	W6X00_LLAR1,                                 // Link-Local Address Register 1
	W6X00_LLAR2,                                 // Link-Local Address Register 2
	W6X00_LLAR3,                                 // Link-Local Address Register 3
	W6X00_LLAR4,                                 // Link-Local Address Register 4
	W6X00_LLAR5,                                 // Link-Local Address Register 5
	W6X00_LLAR6,                                 // Link-Local Address Register 6
	W6X00_LLAR7,                                 // Link-Local Address Register 7
	W6X00_LLAR8,                                 // Link-Local Address Register 8
	W6X00_LLAR9,                                 // Link-Local Address Register 9
	W6X00_LLAR10,                                // Link-Local Address Register 10
	W6X00_LLAR11,                                // Link-Local Address Register 11
	W6X00_LLAR12,                                // Link-Local Address Register 12
	W6X00_LLAR13,                                // Link-Local Address Register 13
	W6X00_LLAR14,                                // Link-Local Address Register 14
	W6X00_LLAR15,                                // Link-Local Address Register 15
	W6X00_GUAR0,                                 // Global Unicast Address Register 0
	W6X00_GUAR1,                                 // Global Unicast Address Register 1
	W6X00_GUAR2,                                 // Global Unicast Address Register 2
	W6X00_GUAR3,                                 // Global Unicast Address Register 3
	W6X00_GUAR4,                                 // Global Unicast Address Register 4
	W6X00_GUAR5,                                 // Global Unicast Address Register 5
	W6X00_GUAR6,                                 // Global Unicast Address Register 6
	W6X00_GUAR7,                                 // Global Unicast Address Register 7
	W6X00_GUAR8,                                 // Global Unicast Address Register 8
	W6X00_GUAR9,                                 // Global Unicast Address Register 9
	W6X00_GUAR10,                                // Global Unicast Address Register 10
	W6X00_GUAR11,                                // Global Unicast Address Register 11
	W6X00_GUAR12,                                // Global Unicast Address Register 12
	W6X00_GUAR13,                                // Global Unicast Address Register 13
	W6X00_GUAR14,                                // Global Unicast Address Register 14
	W6X00_GUAR15,                                // Global Unicast Address Register 15
	W6X00_SUB6R0,                                // IPv6 Subnet Prefix Register 0
	W6X00_SUB6R1,                                // IPv6 Subnet Prefix Register 1
	W6X00_SUB6R2,                                // IPv6 Subnet Prefix Register 2
	W6X00_SUB6R3,                                // IPv6 Subnet Prefix Register 3
	W6X00_SUB6R4,                                // IPv6 Subnet Prefix Register 4
	W6X00_SUB6R5,                                // IPv6 Subnet Prefix Register 5
	W6X00_SUB6R6,                                // IPv6 Subnet Prefix Register 6
	W6X00_SUB6R7,                                // IPv6 Subnet Prefix Register 7
	W6X00_SUB6R8,                                // IPv6 Subnet Prefix Register 8
	W6X00_SUB6R9,                                // IPv6 Subnet Prefix Register 9
	W6X00_SUB6R10,                               // IPv6 Subnet Prefix Register 10
	W6X00_SUB6R11,                               // IPv6 Subnet Prefix Register 11
	W6X00_SUB6R12,                               // IPv6 Subnet Prefix Register 12
	W6X00_SUB6R13,                               // IPv6 Subnet Prefix Register 13
	W6X00_SUB6R14,                               // IPv6 Subnet Prefix Register 14
	W6X00_SUB6R15,                               // IPv6 Subnet Prefix Register 15
	W6X00_GA6R0,                                 // IPv6 Gateway Address Register 0
	W6X00_GA6R1,                                 // IPv6 Gateway Address Register 1
	W6X00_GA6R2,                                 // IPv6 Gateway Address Register 2
	W6X00_GA6R3,                                 // IPv6 Gateway Address Register 3
	W6X00_GA6R4,                                 // IPv6 Gateway Address Register 4
	W6X00_GA6R5,                                 // IPv6 Gateway Address Register 5
	W6X00_GA6R6,                                 // IPv6 Gateway Address Register 6
	W6X00_GA6R7,                                 // IPv6 Gateway Address Register 7
	W6X00_GA6R8,                                 // IPv6 Gateway Address Register 8
	W6X00_GA6R9,                                 // IPv6 Gateway Address Register 9
	W6X00_GA6R10,                                // IPv6 Gateway Address Register 10
	W6X00_GA6R11,                                // IPv6 Gateway Address Register 11
	W6X00_GA6R12,                                // IPv6 Gateway Address Register 12
	W6X00_GA6R13,                                // IPv6 Gateway Address Register 13
	W6X00_GA6R14,                                // IPv6 Gateway Address Register 14
	W6X00_GA6R15,                                // IPv6 Gateway Address Register 15
	W6X00_SLDIP6R0,                              // Socketless Destination IPv6 Address Register 0
	W6X00_SLDIP6R1,                              // Socketless Destination IPv6 Address Register 1
	W6X00_SLDIP6R2,                              // Socketless Destination IPv6 Address Register 2
	W6X00_SLDIP6R3,                              // Socketless Destination IPv6 Address Register 3
	W6X00_SLDIP6R4,                              // Socketless Destination IPv6 Address Register 4
	W6X00_SLDIP6R5,                              // Socketless Destination IPv6 Address Register 5
	W6X00_SLDIP6R6,                              // Socketless Destination IPv6 Address Register 6
	W6X00_SLDIP6R7,                              // Socketless Destination IPv6 Address Register 7
	W6X00_SLDIP6R8,                              // Socketless Destination IPv6 Address Register 8
	W6X00_SLDIP6R9,                              // Socketless Destination IPv6 Address Register 9
	W6X00_SLDIP6R10,                             // Socketless Destination IPv6 Address Register 10
	W6X00_SLDIP6R11,                             // Socketless Destination IPv6 Address Register 11
	W6X00_SLDIP6R12,                             // Socketless Destination IPv6 Address Register 12
	W6X00_SLDIP6R13,                             // Socketless Destination IPv6 Address Register 13
	W6X00_SLDIP6R14,                             // Socketless Destination IPv6 Address Register 14
	W6X00_SLDIP6R15,                             // Socketless Destination IPv6 Address Register 15
	W6X00_SLDIPR0           = W6X00_REG(0x418C), // Socketless Destination IPv4 Address Register 0
	W6X00_SLDIPR1,                               // Socketless Destination IPv4 Address Register 1
	W6X00_SLDIPR2,                               // Socketless Destination IPv4 Address Register 2
	W6X00_SLDIPR3,                               // Socketless Destination IPv4 Address Register 3
	W6X00_SLDHAR0,                               // Socketless Destination Hardware Address Register 0
	W6X00_SLDHAR1,                               // Socketless Destination Hardware Address Register 1
	W6X00_SLDHAR2,                               // Socketless Destination Hardware Address Register 2
	W6X00_SLDHAR3,                               // Socketless Destination Hardware Address Register 3
	W6X00_SLDHAR4,                               // Socketless Destination Hardware Address Register 4
	W6X00_SLDHAR5,                               // Socketless Destination Hardware Address Register 5
	W6X00_PINGIDR0          = W6X00_REG(0x4198), // PING Identifier Register 0
	W6X00_PINGIDR1,                              // PING Identifier Register 1
	W6X00_PINGSEQR0         = W6X00_REG(0x419C), // PING Sequence Number Register 0
	W6X00_PINGSEQR1,                             // PING Sequence Number Register 1
	W6X00_UIPR0             = W6X00_REG(0x41A0), // Unreachable IP Address Register 0
	W6X00_UIPR1,                                 // Unreachable IP Address Register 1
	W6X00_UIPR2,                                 // Unreachable IP Address Register 2
	W6X00_UIPR3,                                 // Unreachable IP Address Register 3
	W6X00_UPORTR0,                               // Unreachable Port Register 0
	W6X00_UPORTR1,                               // Unreachable Port Register 1
	W6X00_UIP6R0            = W6X00_REG(0x41B0), // Unreachable IPv6 Address Register 0
	W6X00_UIP6R1,                                // Unreachable IPv6 Address Register 1
	W6X00_UIP6R2,                                // Unreachable IPv6 Address Register 2
	W6X00_UIP6R3,                                // Unreachable IPv6 Address Register 3
	W6X00_UIP6R4,                                // Unreachable IPv6 Address Register 4
	W6X00_UIP6R5,                                // Unreachable IPv6 Address Register 5
	W6X00_UIP6R6,                                // Unreachable IPv6 Address Register 6
	W6X00_UIP6R7,                                // Unreachable IPv6 Address Register 7
	W6X00_UIP6R8,                                // Unreachable IPv6 Address Register 8
	W6X00_UIP6R9,                                // Unreachable IPv6 Address Register 9
	W6X00_UIP6R10,                               // Unreachable IPv6 Address Register 10
	W6X00_UIP6R11,                               // Unreachable IPv6 Address Register 11
	W6X00_UIP6R12,                               // Unreachable IPv6 Address Register 12
	W6X00_UIP6R13,                               // Unreachable IPv6 Address Register 13
	W6X00_UIP6R14,                               // Unreachable IPv6 Address Register 14
	W6X00_UIP6R15,                               // Unreachable IPv6 Address Register 15
	W6X00_UPORT6R0,                              // Unreachable IPv6 Port Register 0
	W6X00_UPORT6R1,                              // Unreachable IPv6 Port Register 1
	W6X00_INTPTMR0          = W6X00_REG(0x41C5), // Interrupt Pending Time Register 0
	W6X00_INTPTMR1,                              // Interrupt Pending Time Register 1
	W6X00_PLR               = W6X00_REG(0x41D0), // Prefix Length Register
	W6X00_PFR               = W6X00_REG(0x41D4), // Prefix Flag Register
	W6X00_VLTR0             = W6X00_REG(0x41D8), // Valid Lifetime Register 0
	W6X00_VLTR1,                                 // Valid Lifetime Register 1
	W6X00_VLTR2,                                 // Valid Lifetime Register 2
	W6X00_VLTR3,                                 // Valid Lifetime Register 3
	W6X00_PLTR0,                                 // Preferred Lifetime Register 0
	W6X00_PLTR1,                                 // Preferred Lifetime Register 1
	W6X00_PLTR2,                                 // Preferred Lifetime Register 2
	W6X00_PLTR3,                                 // Preferred Lifetime Register 3
	W6X00_PAR0,                                  // Prefix Address Register 0
	W6X00_PAR1,                                  // Prefix Address Register 1
	W6X00_PAR2,                                  // Prefix Address Register 2
	W6X00_PAR3,                                  // Prefix Address Register 3
	W6X00_PAR4,                                  // Prefix Address Register 4
	W6X00_PAR5,                                  // Prefix Address Register 5
	W6X00_PAR6,                                  // Prefix Address Register 6
	W6X00_PAR7,                                  // Prefix Address Register 7
	W6X00_PAR8,                                  // Prefix Address Register 8
	W6X00_PAR9,                                  // Prefix Address Register 9
	W6X00_PAR10,                                 // Prefix Address Register 10
	W6X00_PAR11,                                 // Prefix Address Register 11
	W6X00_PAR12,                                 // Prefix Address Register 12
	W6X00_PAR13,                                 // Prefix Address Register 13
	W6X00_PAR14,                                 // Prefix Address Register 14
	W6X00_PAR15,                                 // Prefix Address Register 15
	W6X00_ICMP6BLKR,                             // ICMPv6 Block Register
	W6X00_CHPLCKR           = W6X00_REG(0x41F4), // Chip Lock Register
	W6X00_NETLCKR,                               // Network Lock Register
	W6X00_PHYLCKR,                               // PHY Lock Register
	W6X00_RTR0              = W6X00_REG(0x4200), // Retransmission Time Register 0
	W6X00_RTR1,                                  // Retransmission Time Register 1
	W6X00_RCR               = W6X00_REG(0x4204), // Retransmission Count Register
	W6X00_SLRTR0            = W6X00_REG(0x4208), // Socketless Retransmission Time Register 0
	W6X00_SLRTR1,                                // Socketless Retransmission Time Register 1
	W6X00_SLRCR             = W6X00_REG(0x420C), // Socketless Retransmission Count Register
	W6X00_SLHOPR            = W6X00_REG(0x420F), // Socketless Hop Limit Register

#define W6X00_S(n) \
	W6X00_S##n##_MR         = W6X00_REG_S(n, Sn_MR),      \
	W6X00_S##n##_PSR        = W6X00_REG_S(n, Sn_PSR),     \
	W6X00_S##n##_CR         = W6X00_REG_S(n, Sn_CR),      \
	W6X00_S##n##_IR         = W6X00_REG_S(n, Sn_IR),      \
	W6X00_S##n##_IMR        = W6X00_REG_S(n, Sn_IMR),     \
	W6X00_S##n##_IRCLR      = W6X00_REG_S(n, Sn_IRCLR),   \
	W6X00_S##n##_SR         = W6X00_REG_S(n, Sn_SR),      \
	W6X00_S##n##_ESR,                                     \
	W6X00_S##n##_PNR        = W6X00_REG_S(n, Sn_PNR),     \
	W6X00_S##n##_TOSR       = W6X00_REG_S(n, Sn_TOSR),    \
	W6X00_S##n##_TTLR       = W6X00_REG_S(n, Sn_TTLR),    \
	W6X00_S##n##_FRGR0      = W6X00_REG_S(n, Sn_FRGR),    \
	W6X00_S##n##_FRGR1,                                   \
	W6X00_S##n##_MSSR0      = W6X00_REG_S(n, Sn_MSSR),    \
	W6X00_S##n##_MSSR1,                                   \
	W6X00_S##n##_PORTR0     = W6X00_REG_S(n, Sn_PORTR),   \
	W6X00_S##n##_PORTR1,                                  \
	W6X00_S##n##_DHAR0      = W6X00_REG_S(n, Sn_DHAR),    \
	W6X00_S##n##_DHAR1,                                   \
	W6X00_S##n##_DHAR2,                                   \
	W6X00_S##n##_DHAR3,                                   \
	W6X00_S##n##_DHAR4,                                   \
	W6X00_S##n##_DHAR5,                                   \
	W6X00_S##n##_DIPR0      = W6X00_REG_S(n, Sn_DIPR),    \
	W6X00_S##n##_DIPR1,                                   \
	W6X00_S##n##_DIPR2,                                   \
	W6X00_S##n##_DIPR3,                                   \
	W6X00_S##n##_DIP6R0     = W6X00_REG_S(n, Sn_DIP6R),   \
	W6X00_S##n##_DIP6R1,                                  \
	W6X00_S##n##_DIP6R2,                                  \
	W6X00_S##n##_DIP6R3,                                  \
	W6X00_S##n##_DIP6R4,                                  \
	W6X00_S##n##_DIP6R5,                                  \
	W6X00_S##n##_DIP6R6,                                  \
	W6X00_S##n##_DIP6R7,                                  \
	W6X00_S##n##_DIP6R8,                                  \
	W6X00_S##n##_DIP6R9,                                  \
	W6X00_S##n##_DIP6R10,                                 \
	W6X00_S##n##_DIP6R11,                                 \
	W6X00_S##n##_DIP6R12,                                 \
	W6X00_S##n##_DIP6R13,                                 \
	W6X00_S##n##_DIP6R14,                                 \
	W6X00_S##n##_DIP6R15,                                 \
	W6X00_S##n##_DPORTR0    = W6X00_REG_S(n, Sn_DPORTR),  \
	W6X00_S##n##_DPORTR1,                                 \
	W6X00_S##n##_MR2        = W6X00_REG_S(n, Sn_MR2),     \
	W6X00_S##n##_RTR0       = W6X00_REG_S(n, Sn_RTR),     \
	W6X00_S##n##_RTR1,                                    \
	W6X00_S##n##_RCR        = W6X00_REG_S(n, Sn_RCR),     \
	W6X00_S##n##_KPALVTR    = W6X00_REG_S(n, Sn_KPALVTR), \
	W6X00_S##n##_TX_BSR     = W6X00_REG_S(n, Sn_TX_BSR),  \
	W6X00_S##n##_TX_FSR0    = W6X00_REG_S(n, Sn_TX_FSR),  \
	W6X00_S##n##_TX_FSR1,                                 \
	W6X00_S##n##_TX_RD0     = W6X00_REG_S(n, Sn_TX_RD),   \
	W6X00_S##n##_TX_RD1,                                  \
	W6X00_S##n##_TX_WR0     = W6X00_REG_S(n, Sn_TX_WR),   \
	W6X00_S##n##_TX_WR1,                                  \
	W6X00_S##n##_RX_BSR     = W6X00_REG_S(n, Sn_RX_BSR),  \
	W6X00_S##n##_RX_RSR0    = W6X00_REG_S(n, Sn_RX_RSR),  \
	W6X00_S##n##_RX_RSR1,                                 \
	W6X00_S##n##_RX_RD0     = W6X00_REG_S(n, Sn_RX_RD),   \
	W6X00_S##n##_RX_RD1,                                  \
	W6X00_S##n##_RX_WR0     = W6X00_REG_S(n, Sn_RX_WR),   \
	W6X00_S##n##_RX_WR1,                                  \

W6X00_S(0)
W6X00_S(1)
W6X00_S(2)
W6X00_S(3)
W6X00_S(4)
W6X00_S(5)
W6X00_S(6)
W6X00_S(7)
#undef W6X00_S
} W6X00Reg;

typedef enum {
	W6X00_CIDR              = W6X00_REG(0x0000), // Chip Identification Register
	W6X00_VER               = W6X00_REG(0x0002), // Chip Version Register
	W6X00_SYCR              = W6X00_REG(0x2004), // System Configuration Register
	W6X00_TCNTR             = W6X00_REG(0x2016), // Tick Counter Register
	W6X00_PHYDIR            = W6X00_REG(0x300C), // PHY Data Input Register
	W6X00_PHYDOR            = W6X00_REG(0x3010), // PHY Data Output Register
	W6X00_PHYCR             = W6X00_REG(0x301C), // PHY Control Register
	W6X00_NETMR             = W6X00_REG(0x4008), // Network Mode Register
	W6X00_PSIDR             = W6X00_REG(0x4110), // PPPoE Session ID Register
	W6X00_PMRUR             = W6X00_REG(0x4114), // PPPoE Maximum Receive Unit Register
	W6X00_PINGIDR           = W6X00_REG(0x4198), // PING Identifier Register
	W6X00_PINGSEQR          = W6X00_REG(0x419C), // PING Sequence Number Register
	W6X00_UPORTR            = W6X00_REG(0x41A4), // Unreachable Port Register
	W6X00_UPORT6R           = W6X00_REG(0x41C0), // Unreachable IPv6 Port Register
	W6X00_INTPTMR           = W6X00_REG(0x41C5), // Interrupt Pending Time Register
	W6X00_RTR               = W6X00_REG(0x4200), // Retransmission Time Register
	W6X00_SLRTR             = W6X00_REG(0x4208), // Socketless Retransmission Time Register

#define W6X00_S(n) \
	W6X00_S##n##_FRGR       = W6X00_REG_S(n, Sn_FRGR),    \
	W6X00_S##n##_MSSR       = W6X00_REG_S(n, Sn_MSSR),    \
	W6X00_S##n##_PORTR      = W6X00_REG_S(n, Sn_PORTR),   \
	W6X00_S##n##_DPORTR     = W6X00_REG_S(n, Sn_DPORTR),  \
	W6X00_S##n##_RTR        = W6X00_REG_S(n, Sn_RTR),     \
	W6X00_S##n##_TX_FSR     = W6X00_REG_S(n, Sn_TX_FSR),  \
	W6X00_S##n##_TX_RD      = W6X00_REG_S(n, Sn_TX_RD),   \
	W6X00_S##n##_TX_WR      = W6X00_REG_S(n, Sn_TX_WR),   \
	W6X00_S##n##_RX_RSR     = W6X00_REG_S(n, Sn_RX_RSR),  \
	W6X00_S##n##_RX_RD      = W6X00_REG_S(n, Sn_RX_RD),   \
	W6X00_S##n##_RX_WR      = W6X00_REG_S(n, Sn_RX_WR),   \

W6X00_S(0)
W6X00_S(1)
W6X00_S(2)
W6X00_S(3)
W6X00_S(4)
W6X00_S(5)
W6X00_S(6)
W6X00_S(7)
#undef W6X00_S
} W6X00Reg16;

typedef enum {
	W6X00_GAR               = W6X00_REG(0x4130), // Gateway IP Address Register
	W6X00_SUBR              = W6X00_REG(0x4134), // Subnet Mask Register
	W6X00_SIPR              = W6X00_REG(0x4138), // IPv4 Source Address Register
	W6X00_SLDIPR            = W6X00_REG(0x418C), // Socketless Destination IPv4 Address Register
	W6X00_UIPR              = W6X00_REG(0x41A0), // Unreachable IP Address Register
	W6X00_VLTR              = W6X00_REG(0x41D8), // Valid Lifetime Register
	W6X00_PLTR              = W6X00_REG(0x41DC), // Preferred Lifetime Register

#define W6X00_S(n) \
	W6X00_S##n##_DIPR       = W6X00_REG_S(n, Sn_DIPR),    \

W6X00_S(0)
W6X00_S(1)
W6X00_S(2)
W6X00_S(3)
W6X00_S(4)
W6X00_S(5)
W6X00_S(6)
W6X00_S(7)
#undef W6X00_S
} W6X00Reg32;

#define W6X00_SYSR_CHPL                 (1 << 7) // Chip Lock Status
#define W6X00_SYSR_NETL                 (1 << 6) // Network Lock Status
#define W6X00_SYSR_PHYL                 (1 << 5) // PHY Lock Status
#define W6X00_SYSR_IND                  (1 << 1) // Parallel Bus Interface Mode
#define W6X00_SYSR_SPI                  (1 << 0) // SPI Interface Mode

#define W6X00_SYCR_RST                 (1 << 15) // Software Reset
#define W6X00_SYCR_IEN                 (1 <<  7) // Interrupt Enable
#define W6X00_SYCR_CLKSEL              (1 <<  0) // System Operation Clock Select

#define W6X00_IR_WOL                    (1 << 7) // WOL Magic Packet Interrupt
#define W6X00_IR_UNR6                   (1 << 4) // Destination IPv6 Port Unreachable Interrupt
#define W6X00_IR_IPCONF                 (1 << 2) // IP Conflict Interrupt
#define W6X00_IR_UNR4                   (1 << 1) // Destination Port Unreachable Interrupt
#define W6X00_IR_PTERM                  (1 << 0) // PPPoE Terminated Interrupt

#define W6X00_SIR_S(n)                (1 << (n)) // Socket n Interrupt

#define W6X00_SLIR_TOUT                 (1 << 7) // TIMEOUT Interrupt
#define W6X00_SLIR_ARP4                 (1 << 6) // ARP Interrupt
#define W6X00_SLIR_PING4                (1 << 5) // PING Interrupt
#define W6X00_SLIR_ARP6                 (1 << 4) // IPv6 ARP Interrupt
#define W6X00_SLIR_PING6                (1 << 3) // IPv6 PING Interrupt
#define W6X00_SLIR_NS                   (1 << 2) // DAD NS Interrupt
#define W6X00_SLIR_RS                   (1 << 1) // Autoconfiguration RS Interrupt
#define W6X00_SLIR_RA                   (1 << 0) // RA Receive Interrupt

#define W6X00_IMR_WOL                   (1 << 7) // WOL Magic Packet Interrupt Mask
#define W6X00_IMR_UNR6                  (1 << 4) // Destination IPv6 Port Unreachable Interrupt Mask
#define W6X00_IMR_IPCONF                (1 << 2) // IP Conflict Interrupt Mask
#define W6X00_IMR_UNR4                  (1 << 1) // Destination Port Unreachable Interrupt Mask
#define W6X00_IMR_PTERM                 (1 << 0) // PPPoE Terminated Interrupt Mask

#define W6X00_IRCLR_WOL                 (1 << 7) // WOL Magic Packet Interrupt Clear
#define W6X00_IRCLR_UNR6                (1 << 4) // Destination IPv6 Port Unreachable Interrupt Clear
#define W6X00_IRCLR_IPCONF              (1 << 2) // IP Conflict Interrupt Clear
#define W6X00_IRCLR_UNR4                (1 << 1) // Destination Port Unreachable Interrupt Clear
#define W6X00_IRCLR_PTERM               (1 << 0) // PPPoE Terminated Interrupt Clear

#define W6X00_SIMR_S(n)               (1 << (n)) // Socket n Interrupt Mask

#define W6X00_SLIMR_TOUT                (1 << 7) // TIMEOUT Interrupt Mask
#define W6X00_SLIMR_ARP4                (1 << 6) // ARP Interrupt Mask
#define W6X00_SLIMR_PING4               (1 << 5) // PING Interrupt Mask
#define W6X00_SLIMR_ARP6                (1 << 4) // IPv6 ARP Interrupt Mask
#define W6X00_SLIMR_PING6               (1 << 3) // IPv6 PING Interrupt Mask
#define W6X00_SLIMR_NS                  (1 << 2) // DAD NS Interrupt Mask
#define W6X00_SLIMR_RS                  (1 << 1) // Autoconfiguration RS Interrupt Mask
#define W6X00_SLIMR_RA                  (1 << 0) // RA Receive Interrupt Mask

#define W6X00_SLIRCLR_TOUT              (1 << 7) // TIMEOUT Interrupt Clear
#define W6X00_SLIRCLR_ARP4              (1 << 6) // ARP Interrupt Clear
#define W6X00_SLIRCLR_PING4             (1 << 5) // PING Interrupt Clear
#define W6X00_SLIRCLR_ARP6              (1 << 4) // IPv6 ARP Interrupt Clear
#define W6X00_SLIRCLR_PING6             (1 << 3) // IPv6 PING Interrupt Clear
#define W6X00_SLIRCLR_NS                (1 << 2) // DAD NS Interrupt Clear
#define W6X00_SLIRCLR_RS                (1 << 1) // Autoconfiguration RS Interrupt Clear
#define W6X00_SLIRCLR_RA                (1 << 0) // RA Receive Interrupt Clear

enum {
	W6X00_SLPSR_AUTO        = 0x00,
	W6X00_SLPSR_LLA         = 0x02,
	W6X00_SLPSR_GUA,
};

#define W6X00_SLCR_ARP4                 (1 << 6) // ARP Request Transmission Command
#define W6X00_SLCR_PING4                (1 << 5) // PING Request Transmission Command
#define W6X00_SLCR_ARP6                 (1 << 4) // IPv6 ARP Transmission Command
#define W6X00_SLCR_PING6                (1 << 3) // IPv6 PING Request Transmission Command
#define W6X00_SLCR_NS                   (1 << 2) // DAD NS Transmission Command
#define W6X00_SLCR_RS                   (1 << 1) // Autoconfiguration RS Transmission Command
#define W6X00_SLCR_UNA                  (1 << 0) // Unsolicited NA Transmission Command

#define W6X00_PHYSR_CAB                 (1 << 7) // Cable Unplugged
#define W6X00_PHYSR_MODE(x)   (((x) & 0x7) << 3) // PHY Operation Mode
#define W6X00_PHYSR_DPX                 (1 << 2) // PHY Duplex Status
#define W6X00_PHYSR_SPD                 (1 << 1) // PHY Speed Status
#define W6X00_PHYSR_LNK                 (1 << 0) // PHY Link Status

#define W6X00_PHYRAR_ADDR(x) (((x) & 0x1F) << 0) // PHY Register Address

enum {
	W6X00_PHYACR_WRITE      = 0x01,
	W6X00_PHYACR_READ,
};

enum {
	W6X00_PHYDIVR_32        = 0x00,
	W6X00_PHYDIVR_64,
	W6X00_PHYDIVR_128,
};

#define W6X00_PHYCR_MODE(x)  (((x) & 0x7) <<  8) // PHY Operation Mode
#define W6X00_PHYCR_PWDN               (1 <<  5) // PHY Power Down Mode
#define W6X00_PHYCR_TE                 (1 <<  3) // PHY 10BASE-Te Mode
#define W6X00_PHYCR_RST                (1 <<  0) // PHY Reset

#define W6X00_NET4MR_UNRB               (1 << 3) // UDP Port Unreachable Packet Block
#define W6X00_NET4MR_PARP               (1 << 2) // ARP for PING Reply
#define W6X00_NET4MR_RSTB               (1 << 1) // TCP RST Packet Block
#define W6X00_NET4MR_PB                 (1 << 0) // PING Reply Block

#define W6X00_NET6MR_UNRB               (1 << 3) // UDP Port Unreachable Packet Block
#define W6X00_NET6MR_PARP               (1 << 2) // ARP for PING Reply
#define W6X00_NET6MR_RSTB               (1 << 1) // TCP RST Packet Block
#define W6X00_NET6MR_PB                 (1 << 0) // PING Reply Block

#define W6X00_NETMR_ANB                (1 << 13) // IPv6 Allnode Block
#define W6X00_NETMR_M6B                (1 << 12) // IPv6 Multicast Block
#define W6X00_NETMR_WOL                (1 << 10) // Wake-on-LAN
#define W6X00_NETMR_IP6B               (1 <<  9) // IPv6 Packet Block
#define W6X00_NETMR_IP4B               (1 <<  8) // IPv4 Packet Block
#define W6X00_NETMR_DHAS               (1 <<  7) // Destination Hardware Address Selection
#define W6X00_NETMR_PPPOE              (1 <<  0) // PPPoE Mode

#define W6X00_ICMP6BLKR_PING6           (1 << 4) // ICMPv6 Echo Request Block
#define W6X00_ICMP6BLKR_MLD             (1 << 3) // ICMPv6 Multicast Listener Query Block
#define W6X00_ICMP6BLKR_RA              (1 << 2) // ICMPv6 Router Advertisement Block
#define W6X00_ICMP6BLKR_NA              (1 << 1) // ICMPv6 Neighbor Advertisement Block
#define W6X00_ICMP6BLKR_NS              (1 << 0) // ICMPv6 Neighbor Solicitation Block

enum {
	W6X00_CHPLCKR_UNLOCK    = 0xCE,
	W6X00_CHPLCKR_LOCK      = 0x31,
};

enum {
	W6X00_NETLCKR_UNLOCK    = 0x3A,
	W6X00_NETLCKR_LOCK      = 0xC5,
};

enum {
	W6X00_PHYLCKR_UNLOCK    = 0x53,
	W6X00_PHYLCKR_LOCK      = 0xAC,
};

#define W6X00_Sn_MR_MULTI               (1 << 7) // Multicast Mode
#define W6X00_Sn_MR_MF                  (1 << 7) // MAC Filter Enable
#define W6X00_Sn_MR_BRDB                (1 << 6) // Broadcast Block
#define W6X00_Sn_MR_FPSH                (1 << 6) // Force PSH Flag
#define W6X00_Sn_MR_ND                  (1 << 5) // No Delayed ACK
#define W6X00_Sn_MR_MC                  (1 << 5) // Multicast IGMP Version
#define W6X00_Sn_MR_SMB                 (1 << 5) // UDPv6 Solicited Multicast Block
#define W6X00_Sn_MR_MMB                 (1 << 5) // UDPv4 Multicast Block in MACRAW Mode
#define W6X00_Sn_MR_UNIB                (1 << 4) // Unicast Block
#define W6X00_Sn_MR_MMB6                (1 << 4) // UDPv6 Multicast Block in MACRAW Mode
#define W6X00_Sn_MR_P(x)      (((x) & 0xF) << 0) // Protocol Mode

enum {
	W6X00_Sn_MR_CLOSE       = 0x00,
	W6X00_Sn_MR_TCP4,
	W6X00_Sn_MR_UDP4,
	W6X00_Sn_MR_IPRAW4,
	W6X00_Sn_MR_MACRAW      = 0x07,
	W6X00_Sn_MR_TCP6        = 0x09,
	W6X00_Sn_MR_UDP6,
	W6X00_Sn_MR_IPRAW6,
	W6X00_Sn_MR_TCPD        = 0x0D,
	W6X00_Sn_MR_UDPD,
};

enum {
	W6X00_Sn_PSR_AUTO       = 0x00,
	W6X00_Sn_PSR_LLA        = 0x02,
	W6X00_Sn_PSR_GUA,
};

enum {
	W6X00_Sn_CR_OPEN        = 0x01,
	W6X00_Sn_CR_LISTEN,
	W6X00_Sn_CR_CONNECT     = 0x04,
	W6X00_Sn_CR_DISCON      = 0x08,
	W6X00_Sn_CR_CLOSE       = 0x10,
	W6X00_Sn_CR_SEND        = 0x20,
	W6X00_Sn_CR_SEND_KEEP   = 0x22,
	W6X00_Sn_CR_RECV        = 0x40,
	W6X00_Sn_CR_CONNECT6    = 0x84,
	W6X00_Sn_CR_SEND6       = 0xA0,
};

#define W6X00_Sn_IR_SENDOK              (1 << 4) // SEND OK Interrupt
#define W6X00_Sn_IR_TIMEOUT             (1 << 3) // TIMEOUT Interrupt
#define W6X00_Sn_IR_RECV                (1 << 2) // RECEIVED Interrupt
#define W6X00_Sn_IR_DISCON              (1 << 1) // DISCONNECTED Interrupt
#define W6X00_Sn_IR_CON                 (1 << 0) // CONNECTED Interrupt

#define W6X00_Sn_IMR_SENDOK             (1 << 4) // SEND OK Interrupt Mask
#define W6X00_Sn_IMR_TIMEOUT            (1 << 3) // TIMEOUT Interrupt Mask
#define W6X00_Sn_IMR_RECV               (1 << 2) // RECEIVED Interrupt Mask
#define W6X00_Sn_IMR_DISCON             (1 << 1) // DISCONNECTED Interrupt Mask
#define W6X00_Sn_IMR_CON                (1 << 0) // CONNECTED Interrupt Mask

#define W6X00_Sn_IRCLR_SENDOK           (1 << 4) // SEND OK Interrupt Clear
#define W6X00_Sn_IRCLR_TIMEOUT          (1 << 3) // TIMEOUT Interrupt Clear
#define W6X00_Sn_IRCLR_RECV             (1 << 2) // RECEIVED Interrupt Clear
#define W6X00_Sn_IRCLR_DISCON           (1 << 1) // DISCONNECTED Interrupt Clear
#define W6X00_Sn_IRCLR_CON              (1 << 0) // CONNECTED Interrupt Clear

enum {
	W6X00_Sn_SR_CLOSED      = 0x00,
	W6X00_Sn_SR_INIT        = 0x13,
	W6X00_Sn_SR_LISTEN,
	W6X00_Sn_SR_SYNSENT,
	W6X00_Sn_SR_SYNRECV,
	W6X00_Sn_SR_ESTABLISHED,
	W6X00_Sn_SR_FIN_WAIT,
	W6X00_Sn_SR_TIME_WAIT   = 0x1B,
	W6X00_Sn_SR_CLOSE_WAIT,
	W6X00_Sn_SR_LAST_ACK,
	W6X00_Sn_SR_UDP         = 0x22,
	W6X00_Sn_SR_IPRAW       = 0x32,
	W6X00_Sn_SR_IPRAW6,
	W6X00_Sn_SR_MACRAW      = 0x42,
};

#define W6X00_Sn_ESR_TCPM               (1 << 2) // TCP Mode
#define W6X00_Sn_ESR_TCPOP              (1 << 1) // TCP Operation Mode
#define W6X00_Sn_ESR_IP6T               (1 << 0) // IPv6 Address Type

#define W6X00_Sn_MR2_DHAM               (1 << 1) // Destination Hardware Address Mode
#define W6X00_Sn_MR2_FARP               (1 << 0) // Force ARP

#define W6100_TX_BUFSIZE  (W6100_INIT_S0_TX_BSR * 1024)
#define W6300_TX_BUFSIZE  (W6300_INIT_S0_TX_BSR * 1024)
#define W6X00_TX_QUEUELEN ((W6300_TX_BUFSIZE * 27) / (1536 * 20))

#endif /* W6X00_H */
