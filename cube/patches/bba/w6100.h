/* 
 * Copyright (c) 2024, Extrems <extrems@extremscorner.org>
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

#ifndef W6100_H
#define W6100_H

#define W6100_BSB(x)        (((x) & 0x1F) << 27) // Block Select Bits
#define W6100_RWB                      (1 << 26) // Read/Write Access Mode Bit
#define W6100_OM(x)          (((x) & 0x3) << 24) // SPI Operation Mode Bits
#define W6100_ADDR(x)             ((x) & 0xFFFF) // Offset Address

#define W6100_REG(addr)        (W6100_BSB(0)         | W6100_ADDR(addr))
#define W6100_REG_S(n, addr)   (W6100_BSB(n * 4 + 1) | W6100_ADDR(addr))
#define W6100_TXBUF_S(n, addr) (W6100_BSB(n * 4 + 2) | W6100_ADDR(addr))
#define W6100_RXBUF_S(n, addr) (W6100_BSB(n * 4 + 3) | W6100_ADDR(addr))

#define W6100_INIT_S0_TX_BSR (16)
#define W6100_INIT_S0_RX_BSR (16)

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
	W6100_CIDR0             = W6100_REG(0x0000), // Chip Identification Register 0
	W6100_CIDR1,                                 // Chip Identification Register 1
	W6100_VER0,                                  // Chip Version Register 0
	W6100_VER1,                                  // Chip Version Register 1
	W6100_SYSR              = W6100_REG(0x2000), // System Status Register
	W6100_SYCR0             = W6100_REG(0x2004), // System Configuration Register 0
	W6100_SYCR1,                                 // System Configuration Register 1
	W6100_TCNTR0            = W6100_REG(0x2016), // Tick Counter Register 0
	W6100_TCNTR1,                                // Tick Counter Register 1
	W6100_TCNTCLR           = W6100_REG(0x2020), // Tick Counter Clear Register
	W6100_IR                = W6100_REG(0x2100), // Interrupt Register
	W6100_SIR,                                   // Socket Interrupt Register
	W6100_SLIR,                                  // Socketless Interrupt Register
	W6100_IMR               = W6100_REG(0x2104), // Interrupt Mask Register
	W6100_IRCLR             = W6100_REG(0x2108), // Interrupt Clear Register
	W6100_SIMR              = W6100_REG(0x2114), // Socket Interrupt Mask Register
	W6100_SLIMR             = W6100_REG(0x2124), // Socketless Interrupt Mask Register
	W6100_SLIRCLR           = W6100_REG(0x2128), // Socketless Interrupt Clear Register
	W6100_SLPSR             = W6100_REG(0x212C), // Socketless Prefer Source IPv6 Address Register
	W6100_SLCR              = W6100_REG(0x2130), // Socketless Command Register
	W6100_PHYSR             = W6100_REG(0x3000), // PHY Status Register
	W6100_PHYRAR            = W6100_REG(0x3008), // PHY Register Address Register
	W6100_PHYDIR0           = W6100_REG(0x300C), // PHY Data Input Register 0
	W6100_PHYDIR1,                               // PHY Data Input Register 1
	W6100_PHYDOR0           = W6100_REG(0x3010), // PHY Data Output Register 0
	W6100_PHYDOR1,                               // PHY Data Output Register 1
	W6100_PHYACR            = W6100_REG(0x3014), // PHY Access Control Register
	W6100_PHYDIVR           = W6100_REG(0x3018), // PHY Division Register
	W6100_PHYCR0            = W6100_REG(0x301C), // PHY Control Register 0
	W6100_PHYCR1,                                // PHY Control Register 1
	W6100_NET4MR            = W6100_REG(0x4000), // Network IPv4 Mode Register
	W6100_NET6MR            = W6100_REG(0x4004), // Network IPv6 Mode Register
	W6100_NETMR0            = W6100_REG(0x4008), // Network Mode Register 0
	W6100_NETMR1,                                // Network Mode Register 1
	W6100_PTMR              = W6100_REG(0x4100), // PPP LCP Request Timer Register
	W6100_PMNR              = W6100_REG(0x4104), // PPP LCP Magic Number Register
	W6100_PHAR0             = W6100_REG(0x4108), // PPPoE Server Hardware Address Register 0
	W6100_PHAR1,                                 // PPPoE Server Hardware Address Register 1
	W6100_PHAR2,                                 // PPPoE Server Hardware Address Register 2
	W6100_PHAR3,                                 // PPPoE Server Hardware Address Register 3
	W6100_PHAR4,                                 // PPPoE Server Hardware Address Register 4
	W6100_PHAR5,                                 // PPPoE Server Hardware Address Register 5
	W6100_PSIDR0            = W6100_REG(0x4110), // PPPoE Session ID Register 0
	W6100_PSIDR1,                                // PPPoE Session ID Register 1
	W6100_PMRUR0            = W6100_REG(0x4114), // PPPoE Maximum Receive Unit Register 0
	W6100_PMRUR1,                                // PPPoE Maximum Receive Unit Register 1
	W6100_SHAR0             = W6100_REG(0x4120), // Source Hardware Address Register 0
	W6100_SHAR1,                                 // Source Hardware Address Register 1
	W6100_SHAR2,                                 // Source Hardware Address Register 2
	W6100_SHAR3,                                 // Source Hardware Address Register 3
	W6100_SHAR4,                                 // Source Hardware Address Register 4
	W6100_SHAR5,                                 // Source Hardware Address Register 5
	W6100_GAR0              = W6100_REG(0x4130), // Gateway IP Address Register 0
	W6100_GAR1,                                  // Gateway IP Address Register 1
	W6100_GAR2,                                  // Gateway IP Address Register 2
	W6100_GAR3,                                  // Gateway IP Address Register 3
	W6100_SUBR0,                                 // Subnet Mask Register 0
	W6100_SUBR1,                                 // Subnet Mask Register 1
	W6100_SUBR2,                                 // Subnet Mask Register 2
	W6100_SUBR3,                                 // Subnet Mask Register 3
	W6100_SIPR0,                                 // IPv4 Source Address Register 0
	W6100_SIPR1,                                 // IPv4 Source Address Register 1
	W6100_SIPR2,                                 // IPv4 Source Address Register 2
	W6100_SIPR3,                                 // IPv4 Source Address Register 3
	W6100_LLAR0             = W6100_REG(0x4140), // Link-Local Address Register 0
	W6100_LLAR1,                                 // Link-Local Address Register 1
	W6100_LLAR2,                                 // Link-Local Address Register 2
	W6100_LLAR3,                                 // Link-Local Address Register 3
	W6100_LLAR4,                                 // Link-Local Address Register 4
	W6100_LLAR5,                                 // Link-Local Address Register 5
	W6100_LLAR6,                                 // Link-Local Address Register 6
	W6100_LLAR7,                                 // Link-Local Address Register 7
	W6100_LLAR8,                                 // Link-Local Address Register 8
	W6100_LLAR9,                                 // Link-Local Address Register 9
	W6100_LLAR10,                                // Link-Local Address Register 10
	W6100_LLAR11,                                // Link-Local Address Register 11
	W6100_LLAR12,                                // Link-Local Address Register 12
	W6100_LLAR13,                                // Link-Local Address Register 13
	W6100_LLAR14,                                // Link-Local Address Register 14
	W6100_LLAR15,                                // Link-Local Address Register 15
	W6100_GUAR0,                                 // Global Unicast Address Register 0
	W6100_GUAR1,                                 // Global Unicast Address Register 1
	W6100_GUAR2,                                 // Global Unicast Address Register 2
	W6100_GUAR3,                                 // Global Unicast Address Register 3
	W6100_GUAR4,                                 // Global Unicast Address Register 4
	W6100_GUAR5,                                 // Global Unicast Address Register 5
	W6100_GUAR6,                                 // Global Unicast Address Register 6
	W6100_GUAR7,                                 // Global Unicast Address Register 7
	W6100_GUAR8,                                 // Global Unicast Address Register 8
	W6100_GUAR9,                                 // Global Unicast Address Register 9
	W6100_GUAR10,                                // Global Unicast Address Register 10
	W6100_GUAR11,                                // Global Unicast Address Register 11
	W6100_GUAR12,                                // Global Unicast Address Register 12
	W6100_GUAR13,                                // Global Unicast Address Register 13
	W6100_GUAR14,                                // Global Unicast Address Register 14
	W6100_GUAR15,                                // Global Unicast Address Register 15
	W6100_SUB6R0,                                // IPv6 Subnet Prefix Register 0
	W6100_SUB6R1,                                // IPv6 Subnet Prefix Register 1
	W6100_SUB6R2,                                // IPv6 Subnet Prefix Register 2
	W6100_SUB6R3,                                // IPv6 Subnet Prefix Register 3
	W6100_SUB6R4,                                // IPv6 Subnet Prefix Register 4
	W6100_SUB6R5,                                // IPv6 Subnet Prefix Register 5
	W6100_SUB6R6,                                // IPv6 Subnet Prefix Register 6
	W6100_SUB6R7,                                // IPv6 Subnet Prefix Register 7
	W6100_SUB6R8,                                // IPv6 Subnet Prefix Register 8
	W6100_SUB6R9,                                // IPv6 Subnet Prefix Register 9
	W6100_SUB6R10,                               // IPv6 Subnet Prefix Register 10
	W6100_SUB6R11,                               // IPv6 Subnet Prefix Register 11
	W6100_SUB6R12,                               // IPv6 Subnet Prefix Register 12
	W6100_SUB6R13,                               // IPv6 Subnet Prefix Register 13
	W6100_SUB6R14,                               // IPv6 Subnet Prefix Register 14
	W6100_SUB6R15,                               // IPv6 Subnet Prefix Register 15
	W6100_GA6R0,                                 // IPv6 Gateway Address Register 0
	W6100_GA6R1,                                 // IPv6 Gateway Address Register 1
	W6100_GA6R2,                                 // IPv6 Gateway Address Register 2
	W6100_GA6R3,                                 // IPv6 Gateway Address Register 3
	W6100_GA6R4,                                 // IPv6 Gateway Address Register 4
	W6100_GA6R5,                                 // IPv6 Gateway Address Register 5
	W6100_GA6R6,                                 // IPv6 Gateway Address Register 6
	W6100_GA6R7,                                 // IPv6 Gateway Address Register 7
	W6100_GA6R8,                                 // IPv6 Gateway Address Register 8
	W6100_GA6R9,                                 // IPv6 Gateway Address Register 9
	W6100_GA6R10,                                // IPv6 Gateway Address Register 10
	W6100_GA6R11,                                // IPv6 Gateway Address Register 11
	W6100_GA6R12,                                // IPv6 Gateway Address Register 12
	W6100_GA6R13,                                // IPv6 Gateway Address Register 13
	W6100_GA6R14,                                // IPv6 Gateway Address Register 14
	W6100_GA6R15,                                // IPv6 Gateway Address Register 15
	W6100_SLDIP6R0,                              // Socketless Destination IPv6 Address Register 0
	W6100_SLDIP6R1,                              // Socketless Destination IPv6 Address Register 1
	W6100_SLDIP6R2,                              // Socketless Destination IPv6 Address Register 2
	W6100_SLDIP6R3,                              // Socketless Destination IPv6 Address Register 3
	W6100_SLDIP6R4,                              // Socketless Destination IPv6 Address Register 4
	W6100_SLDIP6R5,                              // Socketless Destination IPv6 Address Register 5
	W6100_SLDIP6R6,                              // Socketless Destination IPv6 Address Register 6
	W6100_SLDIP6R7,                              // Socketless Destination IPv6 Address Register 7
	W6100_SLDIP6R8,                              // Socketless Destination IPv6 Address Register 8
	W6100_SLDIP6R9,                              // Socketless Destination IPv6 Address Register 9
	W6100_SLDIP6R10,                             // Socketless Destination IPv6 Address Register 10
	W6100_SLDIP6R11,                             // Socketless Destination IPv6 Address Register 11
	W6100_SLDIP6R12,                             // Socketless Destination IPv6 Address Register 12
	W6100_SLDIP6R13,                             // Socketless Destination IPv6 Address Register 13
	W6100_SLDIP6R14,                             // Socketless Destination IPv6 Address Register 14
	W6100_SLDIP6R15,                             // Socketless Destination IPv6 Address Register 15
	W6100_SLDIPR0           = W6100_REG(0x418C), // Socketless Destination IPv4 Address Register 0
	W6100_SLDIPR1,                               // Socketless Destination IPv4 Address Register 1
	W6100_SLDIPR2,                               // Socketless Destination IPv4 Address Register 2
	W6100_SLDIPR3,                               // Socketless Destination IPv4 Address Register 3
	W6100_SLDHAR0,                               // Socketless Destination Hardware Address Register 0
	W6100_SLDHAR1,                               // Socketless Destination Hardware Address Register 1
	W6100_SLDHAR2,                               // Socketless Destination Hardware Address Register 2
	W6100_SLDHAR3,                               // Socketless Destination Hardware Address Register 3
	W6100_SLDHAR4,                               // Socketless Destination Hardware Address Register 4
	W6100_SLDHAR5,                               // Socketless Destination Hardware Address Register 5
	W6100_PINGIDR0          = W6100_REG(0x4198), // PING Identifier Register 0
	W6100_PINGIDR1,                              // PING Identifier Register 1
	W6100_PINGSEQR0         = W6100_REG(0x419C), // PING Sequence Number Register 0
	W6100_PINGSEQR1,                             // PING Sequence Number Register 1
	W6100_UIPR0             = W6100_REG(0x41A0), // Unreachable IP Address Register 0
	W6100_UIPR1,                                 // Unreachable IP Address Register 1
	W6100_UIPR2,                                 // Unreachable IP Address Register 2
	W6100_UIPR3,                                 // Unreachable IP Address Register 3
	W6100_UPORTR0,                               // Unreachable Port Register 0
	W6100_UPORTR1,                               // Unreachable Port Register 1
	W6100_UIP6R0            = W6100_REG(0x41B0), // Unreachable IPv6 Address Register 0
	W6100_UIP6R1,                                // Unreachable IPv6 Address Register 1
	W6100_UIP6R2,                                // Unreachable IPv6 Address Register 2
	W6100_UIP6R3,                                // Unreachable IPv6 Address Register 3
	W6100_UIP6R4,                                // Unreachable IPv6 Address Register 4
	W6100_UIP6R5,                                // Unreachable IPv6 Address Register 5
	W6100_UIP6R6,                                // Unreachable IPv6 Address Register 6
	W6100_UIP6R7,                                // Unreachable IPv6 Address Register 7
	W6100_UIP6R8,                                // Unreachable IPv6 Address Register 8
	W6100_UIP6R9,                                // Unreachable IPv6 Address Register 9
	W6100_UIP6R10,                               // Unreachable IPv6 Address Register 10
	W6100_UIP6R11,                               // Unreachable IPv6 Address Register 11
	W6100_UIP6R12,                               // Unreachable IPv6 Address Register 12
	W6100_UIP6R13,                               // Unreachable IPv6 Address Register 13
	W6100_UIP6R14,                               // Unreachable IPv6 Address Register 14
	W6100_UIP6R15,                               // Unreachable IPv6 Address Register 15
	W6100_UPORT6R0,                              // Unreachable IPv6 Port Register 0
	W6100_UPORT6R1,                              // Unreachable IPv6 Port Register 1
	W6100_INTPTMR0          = W6100_REG(0x41C5), // Interrupt Pending Time Register 0
	W6100_INTPTMR1,                              // Interrupt Pending Time Register 1
	W6100_PLR               = W6100_REG(0x41D0), // Prefix Length Register
	W6100_PFR               = W6100_REG(0x41D4), // Prefix Flag Register
	W6100_VLTR0             = W6100_REG(0x41D8), // Valid Lifetime Register 0
	W6100_VLTR1,                                 // Valid Lifetime Register 1
	W6100_VLTR2,                                 // Valid Lifetime Register 2
	W6100_VLTR3,                                 // Valid Lifetime Register 3
	W6100_PLTR0,                                 // Preferred Lifetime Register 0
	W6100_PLTR1,                                 // Preferred Lifetime Register 1
	W6100_PLTR2,                                 // Preferred Lifetime Register 2
	W6100_PLTR3,                                 // Preferred Lifetime Register 3
	W6100_PAR0,                                  // Prefix Address Register 0
	W6100_PAR1,                                  // Prefix Address Register 1
	W6100_PAR2,                                  // Prefix Address Register 2
	W6100_PAR3,                                  // Prefix Address Register 3
	W6100_PAR4,                                  // Prefix Address Register 4
	W6100_PAR5,                                  // Prefix Address Register 5
	W6100_PAR6,                                  // Prefix Address Register 6
	W6100_PAR7,                                  // Prefix Address Register 7
	W6100_PAR8,                                  // Prefix Address Register 8
	W6100_PAR9,                                  // Prefix Address Register 9
	W6100_PAR10,                                 // Prefix Address Register 10
	W6100_PAR11,                                 // Prefix Address Register 11
	W6100_PAR12,                                 // Prefix Address Register 12
	W6100_PAR13,                                 // Prefix Address Register 13
	W6100_PAR14,                                 // Prefix Address Register 14
	W6100_PAR15,                                 // Prefix Address Register 15
	W6100_ICMP6BLKR,                             // ICMPv6 Block Register
	W6100_CHPLCKR           = W6100_REG(0x41F4), // Chip Lock Register
	W6100_NETLCKR,                               // Network Lock Register
	W6100_PHYLCKR,                               // PHY Lock Register
	W6100_RTR0              = W6100_REG(0x4200), // Retransmission Time Register 0
	W6100_RTR1,                                  // Retransmission Time Register 1
	W6100_RCR               = W6100_REG(0x4204), // Retransmission Count Register
	W6100_SLRTR0            = W6100_REG(0x4208), // Socketless Retransmission Time Register 0
	W6100_SLRTR1,                                // Socketless Retransmission Time Register 1
	W6100_SLRCR             = W6100_REG(0x420C), // Socketless Retransmission Count Register
	W6100_SLHOPR            = W6100_REG(0x420F), // Socketless Hop Limit Register

#define W6100_S(n) \
	W6100_S##n##_MR         = W6100_REG_S(n, Sn_MR),      \
	W6100_S##n##_PSR        = W6100_REG_S(n, Sn_PSR),     \
	W6100_S##n##_CR         = W6100_REG_S(n, Sn_CR),      \
	W6100_S##n##_IR         = W6100_REG_S(n, Sn_IR),      \
	W6100_S##n##_IMR        = W6100_REG_S(n, Sn_IMR),     \
	W6100_S##n##_IRCLR      = W6100_REG_S(n, Sn_IRCLR),   \
	W6100_S##n##_SR         = W6100_REG_S(n, Sn_SR),      \
	W6100_S##n##_ESR,                                     \
	W6100_S##n##_PNR        = W6100_REG_S(n, Sn_PNR),     \
	W6100_S##n##_TOSR       = W6100_REG_S(n, Sn_TOSR),    \
	W6100_S##n##_TTLR       = W6100_REG_S(n, Sn_TTLR),    \
	W6100_S##n##_FRGR0      = W6100_REG_S(n, Sn_FRGR),    \
	W6100_S##n##_FRGR1,                                   \
	W6100_S##n##_MSSR0      = W6100_REG_S(n, Sn_MSSR),    \
	W6100_S##n##_MSSR1,                                   \
	W6100_S##n##_PORTR0     = W6100_REG_S(n, Sn_PORTR),   \
	W6100_S##n##_PORTR1,                                  \
	W6100_S##n##_DHAR0      = W6100_REG_S(n, Sn_DHAR),    \
	W6100_S##n##_DHAR1,                                   \
	W6100_S##n##_DHAR2,                                   \
	W6100_S##n##_DHAR3,                                   \
	W6100_S##n##_DHAR4,                                   \
	W6100_S##n##_DHAR5,                                   \
	W6100_S##n##_DIPR0      = W6100_REG_S(n, Sn_DIPR),    \
	W6100_S##n##_DIPR1,                                   \
	W6100_S##n##_DIPR2,                                   \
	W6100_S##n##_DIPR3,                                   \
	W6100_S##n##_DIP6R0     = W6100_REG_S(n, Sn_DIP6R),   \
	W6100_S##n##_DIP6R1,                                  \
	W6100_S##n##_DIP6R2,                                  \
	W6100_S##n##_DIP6R3,                                  \
	W6100_S##n##_DIP6R4,                                  \
	W6100_S##n##_DIP6R5,                                  \
	W6100_S##n##_DIP6R6,                                  \
	W6100_S##n##_DIP6R7,                                  \
	W6100_S##n##_DIP6R8,                                  \
	W6100_S##n##_DIP6R9,                                  \
	W6100_S##n##_DIP6R10,                                 \
	W6100_S##n##_DIP6R11,                                 \
	W6100_S##n##_DIP6R12,                                 \
	W6100_S##n##_DIP6R13,                                 \
	W6100_S##n##_DIP6R14,                                 \
	W6100_S##n##_DIP6R15,                                 \
	W6100_S##n##_DPORTR0    = W6100_REG_S(n, Sn_DPORTR),  \
	W6100_S##n##_DPORTR1,                                 \
	W6100_S##n##_MR2        = W6100_REG_S(n, Sn_MR2),     \
	W6100_S##n##_RTR0       = W6100_REG_S(n, Sn_RTR),     \
	W6100_S##n##_RTR1,                                    \
	W6100_S##n##_RCR        = W6100_REG_S(n, Sn_RCR),     \
	W6100_S##n##_KPALVTR    = W6100_REG_S(n, Sn_KPALVTR), \
	W6100_S##n##_TX_BSR     = W6100_REG_S(n, Sn_TX_BSR),  \
	W6100_S##n##_TX_FSR0    = W6100_REG_S(n, Sn_TX_FSR),  \
	W6100_S##n##_TX_FSR1,                                 \
	W6100_S##n##_TX_RD0     = W6100_REG_S(n, Sn_TX_RD),   \
	W6100_S##n##_TX_RD1,                                  \
	W6100_S##n##_TX_WR0     = W6100_REG_S(n, Sn_TX_WR),   \
	W6100_S##n##_TX_WR1,                                  \
	W6100_S##n##_RX_BSR     = W6100_REG_S(n, Sn_RX_BSR),  \
	W6100_S##n##_RX_RSR0    = W6100_REG_S(n, Sn_RX_RSR),  \
	W6100_S##n##_RX_RSR1,                                 \
	W6100_S##n##_RX_RD0     = W6100_REG_S(n, Sn_RX_RD),   \
	W6100_S##n##_RX_RD1,                                  \
	W6100_S##n##_RX_WR0     = W6100_REG_S(n, Sn_RX_WR),   \
	W6100_S##n##_RX_WR1,                                  \

W6100_S(0)
W6100_S(1)
W6100_S(2)
W6100_S(3)
W6100_S(4)
W6100_S(5)
W6100_S(6)
W6100_S(7)
#undef W6100_S
} W6100Reg;

typedef enum {
	W6100_CIDR              = W6100_REG(0x0000), // Chip Identification Register
	W6100_VER               = W6100_REG(0x0002), // Chip Version Register
	W6100_SYCR              = W6100_REG(0x2004), // System Configuration Register
	W6100_TCNTR             = W6100_REG(0x2016), // Tick Counter Register
	W6100_PHYDIR            = W6100_REG(0x300C), // PHY Data Input Register
	W6100_PHYDOR            = W6100_REG(0x3010), // PHY Data Output Register
	W6100_PHYCR             = W6100_REG(0x301C), // PHY Control Register
	W6100_NETMR             = W6100_REG(0x4008), // Network Mode Register
	W6100_PSIDR             = W6100_REG(0x4110), // PPPoE Session ID Register
	W6100_PMRUR             = W6100_REG(0x4114), // PPPoE Maximum Receive Unit Register
	W6100_PINGIDR           = W6100_REG(0x4198), // PING Identifier Register
	W6100_PINGSEQR          = W6100_REG(0x419C), // PING Sequence Number Register
	W6100_UPORTR            = W6100_REG(0x41A4), // Unreachable Port Register
	W6100_UPORT6R           = W6100_REG(0x41C0), // Unreachable IPv6 Port Register
	W6100_INTPTMR           = W6100_REG(0x41C5), // Interrupt Pending Time Register
	W6100_RTR               = W6100_REG(0x4200), // Retransmission Time Register
	W6100_SLRTR             = W6100_REG(0x4208), // Socketless Retransmission Time Register

#define W6100_S(n) \
	W6100_S##n##_FRGR       = W6100_REG_S(n, Sn_FRGR),    \
	W6100_S##n##_MSSR       = W6100_REG_S(n, Sn_MSSR),    \
	W6100_S##n##_PORTR      = W6100_REG_S(n, Sn_PORTR),   \
	W6100_S##n##_DPORTR     = W6100_REG_S(n, Sn_DPORTR),  \
	W6100_S##n##_RTR        = W6100_REG_S(n, Sn_RTR),     \
	W6100_S##n##_TX_FSR     = W6100_REG_S(n, Sn_TX_FSR),  \
	W6100_S##n##_TX_RD      = W6100_REG_S(n, Sn_TX_RD),   \
	W6100_S##n##_TX_WR      = W6100_REG_S(n, Sn_TX_WR),   \
	W6100_S##n##_RX_RSR     = W6100_REG_S(n, Sn_RX_RSR),  \
	W6100_S##n##_RX_RD      = W6100_REG_S(n, Sn_RX_RD),   \
	W6100_S##n##_RX_WR      = W6100_REG_S(n, Sn_RX_WR),   \

W6100_S(0)
W6100_S(1)
W6100_S(2)
W6100_S(3)
W6100_S(4)
W6100_S(5)
W6100_S(6)
W6100_S(7)
#undef W6100_S
} W6100Reg16;

typedef enum {
	W6100_GAR               = W6100_REG(0x4130), // Gateway IP Address Register
	W6100_SUBR              = W6100_REG(0x4134), // Subnet Mask Register
	W6100_SIPR              = W6100_REG(0x4138), // IPv4 Source Address Register
	W6100_SLDIPR            = W6100_REG(0x418C), // Socketless Destination IPv4 Address Register
	W6100_UIPR              = W6100_REG(0x41A0), // Unreachable IP Address Register
	W6100_VLTR              = W6100_REG(0x41D8), // Valid Lifetime Register
	W6100_PLTR              = W6100_REG(0x41DC), // Preferred Lifetime Register

#define W6100_S(n) \
	W6100_S##n##_DIPR       = W6100_REG_S(n, Sn_DIPR),    \

W6100_S(0)
W6100_S(1)
W6100_S(2)
W6100_S(3)
W6100_S(4)
W6100_S(5)
W6100_S(6)
W6100_S(7)
#undef W6100_S
} W6100Reg32;

#define W6100_SYSR_CHPL                 (1 << 7) // Chip Lock Status
#define W6100_SYSR_NETL                 (1 << 6) // Network Lock Status
#define W6100_SYSR_PHYL                 (1 << 5) // PHY Lock Status
#define W6100_SYSR_IND                  (1 << 1) // Parallel Bus Interface Mode
#define W6100_SYSR_SPI                  (1 << 0) // SPI Interface Mode

#define W6100_SYCR_RST                 (1 << 15) // Software Reset
#define W6100_SYCR_IEN                 (1 <<  7) // Interrupt Enable
#define W6100_SYCR_CLKSEL              (1 <<  0) // System Operation Clock Select

#define W6100_IR_WOL                    (1 << 7) // WOL Magic Packet Interrupt
#define W6100_IR_UNR6                   (1 << 4) // Destination IPv6 Port Unreachable Interrupt
#define W6100_IR_IPCONF                 (1 << 2) // IP Conflict Interrupt
#define W6100_IR_UNR4                   (1 << 1) // Destination Port Unreachable Interrupt
#define W6100_IR_PTERM                  (1 << 0) // PPPoE Terminated Interrupt

#define W6100_SIR_S(n)                (1 << (n)) // Socket n Interrupt

#define W6100_SLIR_TOUT                 (1 << 7) // TIMEOUT Interrupt
#define W6100_SLIR_ARP4                 (1 << 6) // ARP Interrupt
#define W6100_SLIR_PING4                (1 << 5) // PING Interrupt
#define W6100_SLIR_ARP6                 (1 << 4) // IPv6 ARP Interrupt
#define W6100_SLIR_PING6                (1 << 3) // IPv6 PING Interrupt
#define W6100_SLIR_NS                   (1 << 2) // DAD NS Interrupt
#define W6100_SLIR_RS                   (1 << 1) // Autoconfiguration RS Interrupt
#define W6100_SLIR_RA                   (1 << 0) // RA Receive Interrupt

#define W6100_IMR_WOL                   (1 << 7) // WOL Magic Packet Interrupt Mask
#define W6100_IMR_UNR6                  (1 << 4) // Destination IPv6 Port Unreachable Interrupt Mask
#define W6100_IMR_IPCONF                (1 << 2) // IP Conflict Interrupt Mask
#define W6100_IMR_UNR4                  (1 << 1) // Destination Port Unreachable Interrupt Mask
#define W6100_IMR_PTERM                 (1 << 0) // PPPoE Terminated Interrupt Mask

#define W6100_IRCLR_WOL                 (1 << 7) // WOL Magic Packet Interrupt Clear
#define W6100_IRCLR_UNR6                (1 << 4) // Destination IPv6 Port Unreachable Interrupt Clear
#define W6100_IRCLR_IPCONF              (1 << 2) // IP Conflict Interrupt Clear
#define W6100_IRCLR_UNR4                (1 << 1) // Destination Port Unreachable Interrupt Clear
#define W6100_IRCLR_PTERM               (1 << 0) // PPPoE Terminated Interrupt Clear

#define W6100_SIMR_S(n)               (1 << (n)) // Socket n Interrupt Mask

#define W6100_SLIMR_TOUT                (1 << 7) // TIMEOUT Interrupt Mask
#define W6100_SLIMR_ARP4                (1 << 6) // ARP Interrupt Mask
#define W6100_SLIMR_PING4               (1 << 5) // PING Interrupt Mask
#define W6100_SLIMR_ARP6                (1 << 4) // IPv6 ARP Interrupt Mask
#define W6100_SLIMR_PING6               (1 << 3) // IPv6 PING Interrupt Mask
#define W6100_SLIMR_NS                  (1 << 2) // DAD NS Interrupt Mask
#define W6100_SLIMR_RS                  (1 << 1) // Autoconfiguration RS Interrupt Mask
#define W6100_SLIMR_RA                  (1 << 0) // RA Receive Interrupt Mask

#define W6100_SLIRCLR_TOUT              (1 << 7) // TIMEOUT Interrupt Clear
#define W6100_SLIRCLR_ARP4              (1 << 6) // ARP Interrupt Clear
#define W6100_SLIRCLR_PING4             (1 << 5) // PING Interrupt Clear
#define W6100_SLIRCLR_ARP6              (1 << 4) // IPv6 ARP Interrupt Clear
#define W6100_SLIRCLR_PING6             (1 << 3) // IPv6 PING Interrupt Clear
#define W6100_SLIRCLR_NS                (1 << 2) // DAD NS Interrupt Clear
#define W6100_SLIRCLR_RS                (1 << 1) // Autoconfiguration RS Interrupt Clear
#define W6100_SLIRCLR_RA                (1 << 0) // RA Receive Interrupt Clear

enum {
	W6100_SLPSR_AUTO        = 0x00,
	W6100_SLPSR_LLA         = 0x02,
	W6100_SLPSR_GUA,
};

#define W6100_SLCR_ARP4                 (1 << 6) // ARP Request Transmission Command
#define W6100_SLCR_PING4                (1 << 5) // PING Request Transmission Command
#define W6100_SLCR_ARP6                 (1 << 4) // IPv6 ARP Transmission Command
#define W6100_SLCR_PING6                (1 << 3) // IPv6 PING Request Transmission Command
#define W6100_SLCR_NS                   (1 << 2) // DAD NS Transmission Command
#define W6100_SLCR_RS                   (1 << 1) // Autoconfiguration RS Transmission Command
#define W6100_SLCR_UNA                  (1 << 0) // Unsolicited NA Transmission Command

#define W6100_PHYSR_CAB                 (1 << 7) // Cable Unplugged
#define W6100_PHYSR_MODE(x)   (((x) & 0x7) << 3) // PHY Operation Mode
#define W6100_PHYSR_DPX                 (1 << 2) // PHY Duplex Status
#define W6100_PHYSR_SPD                 (1 << 1) // PHY Speed Status
#define W6100_PHYSR_LNK                 (1 << 0) // PHY Link Status

#define W6100_PHYRAR_ADDR(x) (((x) & 0x1F) << 0) // PHY Register Address

enum {
	W6100_PHYACR_WRITE      = 0x01,
	W6100_PHYACR_READ,
};

enum {
	W6100_PHYDIVR_32        = 0x00,
	W6100_PHYDIVR_64,
	W6100_PHYDIVR_128,
};

#define W6100_PHYCR_MODE(x)  (((x) & 0x7) <<  8) // PHY Operation Mode
#define W6100_PHYCR_PWDN               (1 <<  5) // PHY Power Down Mode
#define W6100_PHYCR_TE                 (1 <<  3) // PHY 10BASE-Te Mode
#define W6100_PHYCR_RST                (1 <<  0) // PHY Reset

#define W6100_NET4MR_UNRB               (1 << 3) // UDP Port Unreachable Packet Block
#define W6100_NET4MR_PARP               (1 << 2) // ARP for PING Reply
#define W6100_NET4MR_RSTB               (1 << 1) // TCP RST Packet Block
#define W6100_NET4MR_PB                 (1 << 0) // PING Reply Block

#define W6100_NET6MR_UNRB               (1 << 3) // UDP Port Unreachable Packet Block
#define W6100_NET6MR_PARP               (1 << 2) // ARP for PING Reply
#define W6100_NET6MR_RSTB               (1 << 1) // TCP RST Packet Block
#define W6100_NET6MR_PB                 (1 << 0) // PING Reply Block

#define W6100_NETMR_ANB                (1 << 13) // IPv6 Allnode Block
#define W6100_NETMR_M6B                (1 << 12) // IPv6 Multicast Block
#define W6100_NETMR_WOL                (1 << 10) // Wake-on-LAN
#define W6100_NETMR_IP6B               (1 <<  9) // IPv6 Packet Block
#define W6100_NETMR_IP4B               (1 <<  8) // IPv4 Packet Block
#define W6100_NETMR_DHAS               (1 <<  7) // Destination Hardware Address Selection
#define W6100_NETMR_PPPOE              (1 <<  0) // PPPoE Mode

#define W6100_ICMP6BLKR_PING6           (1 << 4) // ICMPv6 Echo Request Block
#define W6100_ICMP6BLKR_MLD             (1 << 3) // ICMPv6 Multicast Listener Query Block
#define W6100_ICMP6BLKR_RA              (1 << 2) // ICMPv6 Router Advertisement Block
#define W6100_ICMP6BLKR_NA              (1 << 1) // ICMPv6 Neighbor Advertisement Block
#define W6100_ICMP6BLKR_NS              (1 << 0) // ICMPv6 Neighbor Solicitation Block

enum {
	W6100_CHPLCKR_UNLOCK    = 0xCE,
	W6100_CHPLCKR_LOCK      = 0x31,
};

enum {
	W6100_NETLCKR_UNLOCK    = 0x3A,
	W6100_NETLCKR_LOCK      = 0xC5,
};

enum {
	W6100_PHYLCKR_UNLOCK    = 0x53,
	W6100_PHYLCKR_LOCK      = 0xAC,
};

#define W6100_Sn_MR_MULTI               (1 << 7) // Multicast Mode
#define W6100_Sn_MR_MF                  (1 << 7) // MAC Filter Enable
#define W6100_Sn_MR_BRDB                (1 << 6) // Broadcast Block
#define W6100_Sn_MR_FPSH                (1 << 6) // Force PSH Flag
#define W6100_Sn_MR_ND                  (1 << 5) // No Delayed ACK
#define W6100_Sn_MR_MC                  (1 << 5) // Multicast IGMP Version
#define W6100_Sn_MR_SMB                 (1 << 5) // UDPv6 Solicited Multicast Block
#define W6100_Sn_MR_MMB                 (1 << 5) // UDPv4 Multicast Block in MACRAW Mode
#define W6100_Sn_MR_UNIB                (1 << 4) // Unicast Block
#define W6100_Sn_MR_MMB6                (1 << 4) // UDPv6 Multicast Block in MACRAW Mode
#define W6100_Sn_MR_P(x)      (((x) & 0xF) << 0) // Protocol Mode

enum {
	W6100_Sn_MR_CLOSE       = 0x00,
	W6100_Sn_MR_TCP4,
	W6100_Sn_MR_UDP4,
	W6100_Sn_MR_IPRAW4,
	W6100_Sn_MR_MACRAW      = 0x07,
	W6100_Sn_MR_TCP6        = 0x09,
	W6100_Sn_MR_UDP6,
	W6100_Sn_MR_IPRAW6,
	W6100_Sn_MR_TCPD        = 0x0D,
	W6100_Sn_MR_UDPD,
};

enum {
	W6100_Sn_PSR_AUTO       = 0x00,
	W6100_Sn_PSR_LLA        = 0x02,
	W6100_Sn_PSR_GUA,
};

enum {
	W6100_Sn_CR_OPEN        = 0x01,
	W6100_Sn_CR_LISTEN,
	W6100_Sn_CR_CONNECT     = 0x04,
	W6100_Sn_CR_DISCON      = 0x08,
	W6100_Sn_CR_CLOSE       = 0x10,
	W6100_Sn_CR_SEND        = 0x20,
	W6100_Sn_CR_SEND_KEEP   = 0x22,
	W6100_Sn_CR_RECV        = 0x40,
	W6100_Sn_CR_CONNECT6    = 0x84,
	W6100_Sn_CR_SEND6       = 0xA0,
};

#define W6100_Sn_IR_SENDOK              (1 << 4) // SEND OK Interrupt
#define W6100_Sn_IR_TIMEOUT             (1 << 3) // TIMEOUT Interrupt
#define W6100_Sn_IR_RECV                (1 << 2) // RECEIVED Interrupt
#define W6100_Sn_IR_DISCON              (1 << 1) // DISCONNECTED Interrupt
#define W6100_Sn_IR_CON                 (1 << 0) // CONNECTED Interrupt

#define W6100_Sn_IMR_SENDOK             (1 << 4) // SEND OK Interrupt Mask
#define W6100_Sn_IMR_TIMEOUT            (1 << 3) // TIMEOUT Interrupt Mask
#define W6100_Sn_IMR_RECV               (1 << 2) // RECEIVED Interrupt Mask
#define W6100_Sn_IMR_DISCON             (1 << 1) // DISCONNECTED Interrupt Mask
#define W6100_Sn_IMR_CON                (1 << 0) // CONNECTED Interrupt Mask

#define W6100_Sn_IRCLR_SENDOK           (1 << 4) // SEND OK Interrupt Clear
#define W6100_Sn_IRCLR_TIMEOUT          (1 << 3) // TIMEOUT Interrupt Clear
#define W6100_Sn_IRCLR_RECV             (1 << 2) // RECEIVED Interrupt Clear
#define W6100_Sn_IRCLR_DISCON           (1 << 1) // DISCONNECTED Interrupt Clear
#define W6100_Sn_IRCLR_CON              (1 << 0) // CONNECTED Interrupt Clear

enum {
	W6100_Sn_SR_CLOSED      = 0x00,
	W6100_Sn_SR_INIT        = 0x13,
	W6100_Sn_SR_LISTEN,
	W6100_Sn_SR_SYNSENT,
	W6100_Sn_SR_SYNRECV,
	W6100_Sn_SR_ESTABLISHED,
	W6100_Sn_SR_FIN_WAIT,
	W6100_Sn_SR_TIME_WAIT   = 0x1B,
	W6100_Sn_SR_CLOSE_WAIT,
	W6100_Sn_SR_LAST_ACK,
	W6100_Sn_SR_UDP         = 0x22,
	W6100_Sn_SR_IPRAW       = 0x32,
	W6100_Sn_SR_IPRAW6,
	W6100_Sn_SR_MACRAW      = 0x42,
};

#define W6100_Sn_ESR_TCPM               (1 << 2) // TCP Mode
#define W6100_Sn_ESR_TCPOP              (1 << 1) // TCP Operation Mode
#define W6100_Sn_ESR_IP6T               (1 << 0) // IPv6 Address Type

#define W6100_Sn_MR2_DHAM               (1 << 1) // Destination Hardware Address Mode
#define W6100_Sn_MR2_FARP               (1 << 0) // Force ARP

#define W6100_TX_BUFSIZE  (W6100_INIT_S0_TX_BSR * 1024)
#define W6100_TX_QUEUELEN ((W6100_TX_BUFSIZE * 27) / (1536 * 20))

#endif /* W6100_H */
