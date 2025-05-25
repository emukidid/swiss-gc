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

#ifndef W5500_H
#define W5500_H

#define W5500_CID (0x03000000)

#define W5500_BSB(x)         (((x) & 0x1F) << 27) // Block Select Bits
#define W5500_RWB                       (1 << 26) // Read/Write Access Mode Bit
#define W5500_OM(x)           (((x) & 0x3) << 24) // SPI Operation Mode Bits
#define W5500_ADDR(x)              ((x) & 0xFFFF) // Offset Address

#define W5500_REG(addr)        (W5500_BSB(0)         | W5500_ADDR(addr))
#define W5500_REG_S(n, addr)   (W5500_BSB(n * 4 + 1) | W5500_ADDR(addr))
#define W5500_TXBUF_S(n, addr) (W5500_BSB(n * 4 + 2) | W5500_ADDR(addr))
#define W5500_RXBUF_S(n, addr) (W5500_BSB(n * 4 + 3) | W5500_ADDR(addr))

#define W5500_INIT_S0_RXBUF_SIZE (16)
#define W5500_INIT_S0_TXBUF_SIZE (16)

enum {
	Sn_MR         = 0x00, // Socket n Mode Register
	Sn_CR,                // Socket n Command Register
	Sn_IR,                // Socket n Interrupt Register
	Sn_SR,                // Socket n Status Register
	Sn_PORT,              // Socket n Source Port Register
	Sn_DHAR       = 0x06, // Socket n Destination Hardware Address Register
	Sn_DIPR       = 0x0C, // Socket n Destination IP Address Register
	Sn_DPORT      = 0x10, // Socket n Destination Port Register
	Sn_MSSR       = 0x12, // Socket n Maximum Segment Size Register
	Sn_TOS        = 0x15, // Socket n IP TOS Register
	Sn_TTL,               // Socket n IP TTL Register
	Sn_RXBUF_SIZE = 0x1E, // Socket n RX Buffer Size Register
	Sn_TXBUF_SIZE,        // Socket n TX Buffer Size Register
	Sn_TX_FSR,            // Socket n TX Free Buffer Size Register
	Sn_TX_RD      = 0x22, // Socket n TX Read Pointer Register
	Sn_TX_WR      = 0x24, // Socket n TX Write Pointer Register
	Sn_RX_RSR     = 0x26, // Socket n RX Received Size Register
	Sn_RX_RD      = 0x28, // Socket n RX Read Pointer Register
	Sn_RX_WR      = 0x2A, // Socket n RX Write Pointer Register
	Sn_IMR        = 0x2C, // Socket n Interrupt Mask Register
	Sn_FRAG,              // Socket n Fragment Offset in IP Header Register
	Sn_KPALVTR    = 0x2F, // Socket n Keepalive Time Register
};

typedef enum {
	W5500_MR                = W5500_REG(0x00), // Mode Register
	W5500_GAR0,                                // Gateway IP Address Register 0
	W5500_GAR1,                                // Gateway IP Address Register 1
	W5500_GAR2,                                // Gateway IP Address Register 2
	W5500_GAR3,                                // Gateway IP Address Register 3
	W5500_SUBR0,                               // Subnet Mask Register 0
	W5500_SUBR1,                               // Subnet Mask Register 1
	W5500_SUBR2,                               // Subnet Mask Register 2
	W5500_SUBR3,                               // Subnet Mask Register 3
	W5500_SHAR0,                               // Source Hardware Address Register 0
	W5500_SHAR1,                               // Source Hardware Address Register 1
	W5500_SHAR2,                               // Source Hardware Address Register 2
	W5500_SHAR3,                               // Source Hardware Address Register 3
	W5500_SHAR4,                               // Source Hardware Address Register 4
	W5500_SHAR5,                               // Source Hardware Address Register 5
	W5500_SIPR0,                               // Source IP Address Register 0
	W5500_SIPR1,                               // Source IP Address Register 1
	W5500_SIPR2,                               // Source IP Address Register 2
	W5500_SIPR3,                               // Source IP Address Register 3
	W5500_INTLEVEL0,                           // Interrupt Low-Level Timer Register 0
	W5500_INTLEVEL1,                           // Interrupt Low-Level Timer Register 1
	W5500_IR,                                  // Interrupt Register
	W5500_IMR,                                 // Interrupt Mask Register
	W5500_SIR,                                 // Socket Interrupt Register
	W5500_SIMR,                                // Socket Interrupt Mask Register
	W5500_RTR0,                                // Retransmission Time Register 0
	W5500_RTR1,                                // Retransmission Time Register 1
	W5500_RCR,                                 // Retransmission Count Register
	W5500_PTIMER,                              // PPP LCP Request Timer Register
	W5500_PMAGIC,                              // PPP LCP Magic Number Register
	W5500_PHAR0,                               // PPPoE Server Hardware Address Register 0
	W5500_PHAR1,                               // PPPoE Server Hardware Address Register 1
	W5500_PHAR2,                               // PPPoE Server Hardware Address Register 2
	W5500_PHAR3,                               // PPPoE Server Hardware Address Register 3
	W5500_PHAR4,                               // PPPoE Server Hardware Address Register 4
	W5500_PHAR5,                               // PPPoE Server Hardware Address Register 5
	W5500_PSID0,                               // PPPoE Session ID Register 0
	W5500_PSID1,                               // PPPoE Session ID Register 1
	W5500_PMRU0,                               // PPPoE Maximum Receive Unit Register 0
	W5500_PMRU1,                               // PPPoE Maximum Receive Unit Register 1
	W5500_UIPR0,                               // Unreachable IP Address Register 0
	W5500_UIPR1,                               // Unreachable IP Address Register 1
	W5500_UIPR2,                               // Unreachable IP Address Register 2
	W5500_UIPR3,                               // Unreachable IP Address Register 3
	W5500_UPORTR0,                             // Unreachable Port Register 0
	W5500_UPORTR1,                             // Unreachable Port Register 1
	W5500_PHYCFGR,                             // PHY Configuration Register
	W5500_VERSIONR          = W5500_REG(0x39), // Chip Version Register

#define W5500_S(n) \
	W5500_S##n##_MR         = W5500_REG_S(n, Sn_MR),         \
	W5500_S##n##_CR,                                         \
	W5500_S##n##_IR,                                         \
	W5500_S##n##_SR,                                         \
	W5500_S##n##_PORT0,                                      \
	W5500_S##n##_PORT1,                                      \
	W5500_S##n##_DHAR0,                                      \
	W5500_S##n##_DHAR1,                                      \
	W5500_S##n##_DHAR2,                                      \
	W5500_S##n##_DHAR3,                                      \
	W5500_S##n##_DHAR4,                                      \
	W5500_S##n##_DHAR5,                                      \
	W5500_S##n##_DIPR0,                                      \
	W5500_S##n##_DIPR1,                                      \
	W5500_S##n##_DIPR2,                                      \
	W5500_S##n##_DIPR3,                                      \
	W5500_S##n##_DPORT0,                                     \
	W5500_S##n##_DPORT1,                                     \
	W5500_S##n##_MSSR0,                                      \
	W5500_S##n##_MSSR1,                                      \
	W5500_S##n##_TOS        = W5500_REG_S(n, Sn_TOS),        \
	W5500_S##n##_TTL,                                        \
	W5500_S##n##_RXBUF_SIZE = W5500_REG_S(n, Sn_RXBUF_SIZE), \
	W5500_S##n##_TXBUF_SIZE,                                 \
	W5500_S##n##_TX_FSR0,                                    \
	W5500_S##n##_TX_FSR1,                                    \
	W5500_S##n##_TX_RD0,                                     \
	W5500_S##n##_TX_RD1,                                     \
	W5500_S##n##_TX_WR0,                                     \
	W5500_S##n##_TX_WR1,                                     \
	W5500_S##n##_RX_RSR0,                                    \
	W5500_S##n##_RX_RSR1,                                    \
	W5500_S##n##_RX_RD0,                                     \
	W5500_S##n##_RX_RD1,                                     \
	W5500_S##n##_RX_WR0,                                     \
	W5500_S##n##_RX_WR1,                                     \
	W5500_S##n##_IMR,                                        \
	W5500_S##n##_FRAG0,                                      \
	W5500_S##n##_FRAG1,                                      \
	W5500_S##n##_KPALVTR,                                    \

W5500_S(0)
W5500_S(1)
W5500_S(2)
W5500_S(3)
W5500_S(4)
W5500_S(5)
W5500_S(6)
W5500_S(7)
#undef W5500_S
} W5500Reg;

typedef enum {
	W5500_INTLEVEL          = W5500_REG(0x13), // Interrupt Low-Level Timer Register
	W5500_RTR               = W5500_REG(0x19), // Retransmission Time Register
	W5500_PSID              = W5500_REG(0x24), // PPPoE Session ID Register
	W5500_PMRU              = W5500_REG(0x26), // PPPoE Maximum Receive Unit Register
	W5500_UPORTR            = W5500_REG(0x2C), // Unreachable Port Register

#define W5500_S(n) \
	W5500_S##n##_PORT       = W5500_REG_S(n, Sn_PORT),       \
	W5500_S##n##_DPORT      = W5500_REG_S(n, Sn_DPORT),      \
	W5500_S##n##_MSSR       = W5500_REG_S(n, Sn_MSSR),       \
	W5500_S##n##_TX_FSR     = W5500_REG_S(n, Sn_TX_FSR),     \
	W5500_S##n##_TX_RD      = W5500_REG_S(n, Sn_TX_RD),      \
	W5500_S##n##_TX_WR      = W5500_REG_S(n, Sn_TX_WR),      \
	W5500_S##n##_RX_RSR     = W5500_REG_S(n, Sn_RX_RSR),     \
	W5500_S##n##_RX_RD      = W5500_REG_S(n, Sn_RX_RD),      \
	W5500_S##n##_RX_WR      = W5500_REG_S(n, Sn_RX_WR),      \
	W5500_S##n##_FRAG       = W5500_REG_S(n, Sn_FRAG),       \

W5500_S(0)
W5500_S(1)
W5500_S(2)
W5500_S(3)
W5500_S(4)
W5500_S(5)
W5500_S(6)
W5500_S(7)
#undef W5500_S
} W5500Reg16;

typedef enum {
	W5500_GAR               = W5500_REG(0x01), // Gateway IP Address Register
	W5500_SUBR              = W5500_REG(0x05), // Subnet Mask Register
	W5500_SIPR              = W5500_REG(0x0F), // Source IP Address Register
	W5500_UIPR              = W5500_REG(0x28), // Unreachable IP Address Register

#define W5500_S(n) \
	W5500_S##n##_DIPR       = W5500_REG_S(n, Sn_DIPR),       \

W5500_S(0)
W5500_S(1)
W5500_S(2)
W5500_S(3)
W5500_S(4)
W5500_S(5)
W5500_S(6)
W5500_S(7)
#undef W5500_S
} W5500Reg32;

#define W5500_MR_RST                     (1 << 7) // Software Reset
#define W5500_MR_WOL                     (1 << 5) // Wake-on-LAN
#define W5500_MR_PB                      (1 << 4) // Ping Reply Block
#define W5500_MR_PPPOE                   (1 << 3) // PPPoE Mode
#define W5500_MR_FARP                    (1 << 1) // Force ARP

#define W5500_IR_CONFLICT                (1 << 7) // IP Conflict Interrupt
#define W5500_IR_UNREACH                 (1 << 6) // Destination Port Unreachable Interrupt
#define W5500_IR_PPPOE                   (1 << 5) // PPPoE Terminated Interrupt
#define W5500_IR_MP                      (1 << 4) // WOL Magic Packet Interrupt

#define W5500_IMR_CONFLICT               (1 << 7) // IP Conflict Interrupt Mask
#define W5500_IMR_UNREACH                (1 << 6) // Destination Port Unreachable Interrupt Mask
#define W5500_IMR_PPPOE                  (1 << 5) // PPPoE Terminated Interrupt Mask
#define W5500_IMR_MP                     (1 << 4) // WOL Magic Packet Interrupt Mask

#define W5500_SIR_S(n)                 (1 << (n)) // Socket n Interrupt

#define W5500_SIMR_S(n)                (1 << (n)) // Socket n Interrupt Mask

#define W5500_PHYCFGR_RST                (1 << 7) // PHY Reset
#define W5500_PHYCFGR_OPMD               (1 << 6) // Configure PHY Operation Mode
#define W5500_PHYCFGR_OPMDC(x) (((x) & 0x7) << 3) // PHY Operation Mode Configuration
#define W5500_PHYCFGR_DPX                (1 << 2) // PHY Duplex Status
#define W5500_PHYCFGR_SPD                (1 << 1) // PHY Speed Status
#define W5500_PHYCFGR_LNK                (1 << 0) // PHY Link Status

#define W5500_Sn_MR_MULTI                (1 << 7) // Multicast Mode
#define W5500_Sn_MR_MFEN                 (1 << 7) // MAC Filter Enable
#define W5500_Sn_MR_BCASTB               (1 << 6) // Broadcast Block
#define W5500_Sn_MR_ND                   (1 << 5) // No Delayed ACK
#define W5500_Sn_MR_MC                   (1 << 5) // Multicast IGMP Version
#define W5500_Sn_MR_MMB                  (1 << 5) // Multicast Block in MACRAW Mode
#define W5500_Sn_MR_UCASTB               (1 << 4) // Unicast Block
#define W5500_Sn_MR_MIP6B                (1 << 4) // IPv6 Block in MACRAW Mode
#define W5500_Sn_MR_P(x)       (((x) & 0xF) << 0) // Protocol Mode

enum {
	W5500_Sn_MR_CLOSE       = 0x00,
	W5500_Sn_MR_TCP,
	W5500_Sn_MR_UDP,
	W5500_Sn_MR_MACRAW      = 0x04,
};

enum {
	W5500_Sn_CR_OPEN        = 0x01,
	W5500_Sn_CR_LISTEN,
	W5500_Sn_CR_CONNECT     = 0x04,
	W5500_Sn_CR_DISCON      = 0x08,
	W5500_Sn_CR_CLOSE       = 0x10,
	W5500_Sn_CR_SEND        = 0x20,
	W5500_Sn_CR_SEND_MAC,
	W5500_Sn_CR_SEND_KEEP,
	W5500_Sn_CR_RECV        = 0x40,
};

#define W5500_Sn_IR_SENDOK               (1 << 4) // SEND OK Interrupt
#define W5500_Sn_IR_TIMEOUT              (1 << 3) // TIMEOUT Interrupt
#define W5500_Sn_IR_RECV                 (1 << 2) // RECEIVED Interrupt
#define W5500_Sn_IR_DISCON               (1 << 1) // DISCONNECTED Interrupt
#define W5500_Sn_IR_CON                  (1 << 0) // CONNECTED Interrupt

enum {
	W5500_Sn_SR_CLOSED      = 0x00,
	W5500_Sn_SR_INIT        = 0x13,
	W5500_Sn_SR_LISTEN,
	W5500_Sn_SR_SYNSENT,
	W5500_Sn_SR_SYNRECV,
	W5500_Sn_SR_ESTABLISHED,
	W5500_Sn_SR_FIN_WAIT,
	W5500_Sn_SR_CLOSING     = 0x1A,
	W5500_Sn_SR_TIME_WAIT,
	W5500_Sn_SR_CLOSE_WAIT,
	W5500_Sn_SR_LAST_ACK,
	W5500_Sn_SR_UDP         = 0x22,
	W5500_Sn_SR_MACRAW      = 0x42,
};

#define W5500_Sn_IMR_SENDOK              (1 << 4) // SEND OK Interrupt Mask
#define W5500_Sn_IMR_TIMEOUT             (1 << 3) // TIMEOUT Interrupt Mask
#define W5500_Sn_IMR_RECV                (1 << 2) // RECEIVED Interrupt Mask
#define W5500_Sn_IMR_DISCON              (1 << 1) // DISCONNECTED Interrupt Mask
#define W5500_Sn_IMR_CON                 (1 << 0) // CONNECTED Interrupt Mask

#define W5500_TX_BUFSIZE  (W5500_INIT_S0_TXBUF_SIZE * 1024)
#define W5500_TX_QUEUELEN ((W5500_TX_BUFSIZE * 27) / (1536 * 20))

#endif /* W5500_H */
