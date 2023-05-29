/**
 * Wii64 - PowerPC.h
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * 
 * Defines and macros for encoding PPC instructions
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/

/* Example usage (creating add r1, r2, r3):
   	PowerPC_instr x = NEW_PPC_INSTR();
   	PPC_SET_OPCODE(x, PPC_OPCODE_X);
	PPC_SET_FUNC  (x, PPC_FUNC_ADD);
   	PPC_SET_RD(x, R1);
   	PPC_SET_RA(x, R2);
   	PPC_SET_RB(x, R3);
 */

#ifndef POWERPC_H
#define POWERPC_H

// type definitions
typedef unsigned int PowerPC_instr;

// macros
#define NEW_PPC_INSTR() 0

// fields
#define PPC_OPCODE_MASK  0x3F
#define PPC_OPCODE_SHIFT 26
#define PPC_GET_OPCODE(instr)       ((instr           >> PPC_OPCODE_SHIFT) & PPC_OPCODE_MASK)
#define PPC_SET_OPCODE(instr,opcode) (instr |= (opcode & PPC_OPCODE_MASK) << PPC_OPCODE_SHIFT)

#define PPC_REG_MASK     0x1F

#define PPC_RD_MASK      PPC_REG_MASK
#define PPC_RD_SHIFT     21
#define PPC_GET_RD(instr)           ((instr       >> PPC_RD_SHIFT) & PPC_RD_MASK)
#define PPC_SET_RD(instr,rd)         (instr |= (rd & PPC_RD_MASK) << PPC_RD_SHIFT)

#define PPC_RA_MASK      PPC_REG_MASK
#define PPC_RA_SHIFT     16
#define PPC_GET_RA(instr)           ((instr       >> PPC_RA_SHIFT) & PPC_RA_MASK)
#define PPC_SET_RA(instr,ra)         (instr |= (ra & PPC_RA_MASK) << PPC_RA_SHIFT)

#define PPC_RB_MASK      PPC_REG_MASK
#define PPC_RB_SHIFT     11
#define PPC_GET_RB(instr)           ((instr       >> PPC_RB_SHIFT) & PPC_RB_MASK)
#define PPC_SET_RB(instr,rb)         (instr |= (rb & PPC_RB_MASK) << PPC_RB_SHIFT)

#define PPC_OE_MASK      0x1
#define PPC_OE_SHIFT     10
#define PPC_GET_OE(instr)           ((instr       >> PPC_OE_SHIFT) & PPC_OE_MASK)
#define PPC_SET_OE(instr,oe)         (instr |= (oe & PPC_OE_MASK) << PPC_OE_SHIFT)

#define PPC_FUNC_MASK    0x3FF
#define PPC_FUNC_SHIFT   1
#define PPC_GET_FUNC(instr)         ((instr         >> PPC_FUNC_SHIFT) & PPC_FUNC_MASK)
#define PPC_SET_FUNC(instr,func)     (instr |= (func & PPC_FUNC_MASK) << PPC_FUNC_SHIFT)

#define PPC_CR_MASK      0x1
#define PPC_CR_SHIFT     0
#define PPC_GET_CR(instr)           ((instr       >> PPC_CR_SHIFT) & PPC_CR_MASK)
#define PPC_SET_CR(instr,cr)         (instr |= (cr & PPC_CR_MASK) << PPC_CR_SHIFT)

#define PPC_QRI_MASK     0x7
#define PPC_QRI_SHIFT    12
#define PPC_GET_QRI(instr)          ((instr        >> PPC_QRI_SHIFT) & PPC_QRI_MASK)
#define PPC_SET_QRI(instr,qri)       (instr |= (qri & PPC_QRI_MASK) << PPC_QRI_SHIFT)

#define PPC_IMM12_MASK   0xFFF
#define PPC_IMM12_SHIFT  0
#define PPC_GET_IMM12(instr)        ((instr          >> PPC_IMM12_SHIFT) & PPC_IMM12_MASK)
#define PPC_SET_IMM12(instr,imm12)   (instr |= (imm12 & PPC_IMM12_MASK) << PPC_IMM12_SHIFT)

#define PPC_IMMED_MASK   0xFFFF
#define PPC_IMMED_SHIFT  0
#define PPC_GET_IMMED(instr)        ((instr          >> PPC_IMMED_SHIFT) & PPC_IMMED_MASK)
#define PPC_SET_IMMED(instr,immed)   (instr |= (immed & PPC_IMMED_MASK) << PPC_IMMED_SHIFT)

#define PPC_LI_MASK      0xFFFFFF
#define PPC_LI_SHIFT     2
#define PPC_GET_LI(instr)           ((instr       >> PPC_LI_SHIFT) & PPC_LI_MASK)
#define PPC_SET_LI(instr,li)         (instr |= (li & PPC_LI_MASK) << PPC_LI_SHIFT)

#define PPC_AA_MASK      0x1
#define PPC_AA_SHIFT     1
#define PPC_GET_AA(instr)           ((instr       >> PPC_AA_SHIFT) & PPC_AA_MASK)
#define PPC_SET_AA(instr,aa)         (instr |= (aa & PPC_AA_MASK) << PPC_AA_SHIFT)

#define PPC_LK_MASK      0x1
#define PPC_LK_SHIFT     0
#define PPC_GET_LK(instr)           ((instr       >> PPC_LK_SHIFT) & PPC_LK_MASK)
#define PPC_SET_LK(instr,lk)         (instr |= (lk & PPC_LK_MASK) << PPC_LK_SHIFT)

#define PPC_GET_BO PPC_GET_RD
#define PPC_SET_BO PPC_SET_RD
#define PPC_GET_BI PPC_GET_RA
#define PPC_SET_BI PPC_SET_RA

#define PPC_BD_MASK      0x3FFF
#define PPC_BD_SHIFT     2
#define PPC_GET_BD(instr)           ((instr       >> PPC_BD_SHIFT) & PPC_BD_MASK)
#define PPC_SET_BD(instr,bd)         (instr |= (bd & PPC_BD_MASK) << PPC_BD_SHIFT)

#define PPC_CRF_MASK     0x7
#define PPC_CRF_SHIFT    23
#define PPC_GET_CRF(instr)          ((instr        >> PPC_CRF_SHIFT) & PPC_CRF_MASK)
#define PPC_SET_CRF(instr,crf)       (instr |= (crf & PPC_CRF_MASK) << PPC_CRF_SHIFT)

#define PPC_SPR_MASK     0x3FF
#define PPC_SPR_SHIFT    11
#define PPC_GET_SPR(instr)          ((instr        >> PPC_SPR_SHIFT) & PPC_SPR_MASK)
#define PPC_SET_SPR(instr,spr)       (instr |= (spr & PPC_SPR_MASK) << PPC_SPR_SHIFT)

#define PPC_MB_MASK      0x1F
#define PPC_MB_SHIFT     6
#define PPC_GET_MB(instr)           ((instr       >> PPC_MB_SHIFT) & PPC_MB_MASK)
#define PPC_SET_MB(instr,mb)         (instr |= (mb & PPC_MB_MASK) << PPC_MB_SHIFT)

#define PPC_ME_MASK      0x1F
#define PPC_ME_SHIFT     1
#define PPC_GET_ME(instr)           ((instr       >> PPC_ME_SHIFT) & PPC_ME_MASK)
#define PPC_SET_ME(instr,me)         (instr |= (me & PPC_ME_MASK) << PPC_ME_SHIFT)

#define PPC_FM_MASK      0xFF
#define PPC_FM_SHIFT     17
#define PPC_GET_FM(instr)           ((instr       >> PPC_FM_SHIFT) & PPC_FM_MASK)
#define PPC_SET_FM(instr,fm)         (instr |= (fm & PPC_FM_MASK) << PPC_FM_SHIFT)

#define PPC_GET_RS PPC_GET_RD
#define PPC_SET_RS PPC_SET_RD
#define PPC_GET_SH PPC_GET_RB
#define PPC_SET_SH PPC_SET_RB
#define PPC_GET_RC PPC_GET_MB
#define PPC_SET_RC PPC_SET_MB

// Opcodes
#define PPC_OPCODE_X           31
#define PPC_OPCODE_XL          19
#define PPC_OPCODE_WTF         4

#define PPC_OPCODE_ADDI        14
#define PPC_OPCODE_ADDIC       12
#define PPC_OPCODE_ADDIC_      13
#define PPC_OPCODE_ADDIS       15

#define PPC_OPCODE_ANDI        28
#define PPC_OPCODE_ANDIS       29

#define PPC_OPCODE_CMPI        11
#define PPC_OPCODE_CMPLI       10

#define PPC_OPCODE_LBZ         34
#define PPC_OPCODE_LBZU        35
#define PPC_OPCODE_LHA         42
#define PPC_OPCODE_LHAU        43
#define PPC_OPCODE_LHZ         40
#define PPC_OPCODE_LHZU        41
#define PPC_OPCODE_LMW         46
#define PPC_OPCODE_LWZ         32
#define PPC_OPCODE_LWZU        33

#define PPC_OPCODE_LFS         48
#define PPC_OPCODE_LFSU        49
#define PPC_OPCODE_LFD         50
#define PPC_OPCODE_LFDU        51
#define PPC_OPCODE_STFS        52
#define PPC_OPCODE_STFSU       53
#define PPC_OPCODE_STFD        54
#define PPC_OPCODE_STFDU       55
#define PPC_OPCODE_PSQ_L       56

#define PPC_OPCODE_FPD         63
#define PPC_OPCODE_FPS         59

#define PPC_OPCODE_MULLI       7

#define PPC_OPCODE_ORI         24
#define PPC_OPCODE_ORIS        25

#define PPC_OPCODE_STB         38
#define PPC_OPCODE_STBU        39
#define PPC_OPCODE_STBUX       31
#define PPC_OPCODE_STH         44
#define PPC_OPCODE_STHU        45
#define PPC_OPCODE_STMW        47
#define PPC_OPCODE_STW         36
#define PPC_OPCODE_STWU        37

#define PPC_OPCODE_SUBFIC      8

#define PPC_OPCODE_TWI         3

#define PPC_OPCODE_XORI        26
#define PPC_OPCODE_XORIS       27

#define PPC_OPCODE_B           18

#define PPC_OPCODE_BC          16

#define PPC_OPCODE_RLWIMI      20
#define PPC_OPCODE_RLWINM      21
#define PPC_OPCODE_RLWNM       23

#define PPC_OPCODE_SC          17

// Function codes
// X-Form
#define PPC_FUNC_ADD             266
#define PPC_FUNC_ADDC            10
#define PPC_FUNC_ADDE            138
#define PPC_FUNC_ADDME           234
#define PPC_FUNC_ADDZE           202

#define PPC_FUNC_AND             28
#define PPC_FUNC_ANDC            60

// XL-Form
#define PPC_FUNC_BCCTR           528
#define PPC_FUNC_BCLR            16

// X-Form
#define PPC_FUNC_CMP             0
#define PPC_FUNC_CMPL            32

#define PPC_FUNC_CNTLZW          26

// XL-Form
#define PPC_FUNC_CRAND           257
#define PPC_FUNC_CRANDC          129
#define PPC_FUNC_CREQV           289
#define PPC_FUNC_CRNAND          225
#define PPC_FUNC_CRNOR           33
#define PPC_FUNC_CROR            449
#define PPC_FUNC_CRORC           417
#define PPC_FUNC_CRXOR           193

// X-Form
#define PPC_FUNC_DCBA            758
#define PPC_FUNC_DCBF            86
#define PPC_FUNC_DCBI            470
#define PPC_FUNC_DCBST           54
#define PPC_FUNC_DCBT            278
#define PPC_FUNC_DCBTST          246
#define PPC_FUNC_DCBZ            1014
#define PPC_FUNC_DCCCI           454
#define PPC_FUNC_DCREAD          486

// XO-Form
#define PPC_FUNC_DIVW            491
#define PPC_FUNC_DIVWU           459

// X-Form
#define PPC_FUNC_EIEIO           854

#define PPC_FUNC_EQV             284

#define PPC_FUNC_EXTSB           954
#define PPC_FUNC_EXTSH           922

// F-Form
#define PPC_FUNC_FABS            264
#define PPC_FUNC_FADD            21
#define PPC_FUNC_FCFID           846
#define PPC_FUNC_FCMPO           32
#define PPC_FUNC_FCMPU           0
#define PPC_FUNC_FCTID           814
#define PPC_FUNC_FCTIDZ          815
#define PPC_FUNC_FCTIW           14
#define PPC_FUNC_FCTIWZ          15
#define PPC_FUNC_FDIV            18
#define PPC_FUNC_FMADD           29
#define PPC_FUNC_FMR             72
#define PPC_FUNC_FMSUB           28
#define PPC_FUNC_FMUL            25
#define PPC_FUNC_FNABS           136
#define PPC_FUNC_FNEG            40
#define PPC_FUNC_FNMADD          31
#define PPC_FUNC_FNMSUB          30
#define PPC_FUNC_FRES            24
#define PPC_FUNC_FRSP            12
#define PPC_FUNC_FRSQRTE         26
#define PPC_FUNC_FSEL            23
#define PPC_FUNC_FSQRT           22
#define PPC_FUNC_FSUB            20
#define PPC_FUNC_MTFSB0          70
#define PPC_FUNC_MTFSB1          38
#define PPC_FUNC_MTFSFI          134
#define PPC_FUNC_MTFSF           711

// X-Form
#define PPC_FUNC_ICBI            982
#define PPC_FUNC_ICBT            262
#define PPC_FUNC_ICCCI           966
#define PPC_FUNC_ICREAD          998
#define PPC_FUNC_ISYNC           150

#define PPC_FUNC_LBZUX           119
#define PPC_FUNC_LBZX            87
#define PPC_FUNC_LFDUX           631
#define PPC_FUNC_LFDX            599
#define PPC_FUNC_LFSUX           567
#define PPC_FUNC_LFSX            535
#define PPC_FUNC_LHAUX           375
#define PPC_FUNC_LHAX            343
#define PPC_FUNC_LHBRX           790
#define PPC_FUNC_LHZUX           311
#define PPC_FUNC_LHZX            279
#define PPC_FUNC_LSWI            597
#define PPC_FUNC_LSWX            533
#define PPC_FUNC_LWARX           20
#define PPC_FUNC_LWBRX           534
#define PPC_FUNC_LWZUX           55
#define PPC_FUNC_LWZX            23

// XO-Form
// These use the WTF opcode
#define PPC_FUNC_MACCHW          172
#define PPC_FUNC_MACCHWS         236
#define PPC_FUNC_MACCHWSU        204
#define PPC_FUNC_MACCHWU         140
#define PPC_FUNC_MACHHW          44
#define PPC_FUNC_MACHHWS         108
#define PPC_FUNC_MACHHWSU        76
#define PPC_FUNC_MACHHWU         12
#define PPC_FUNC_MACLHW          428
#define PPC_FUNC_MACLHWS         492
#define PPC_FUNC_MACLHWSU        460
#define PPC_FUNC_MACLHWU         396

// XL-Form
#define PPC_FUNC_MCRF            0

// X-Form
#define PPC_FUNC_MCRXR           512
#define PPC_FUNC_MFCR            19
#define PPC_FUNC_MFDCR           323
#define PPC_FUNC_MFMSR           83
#define PPC_FUNC_MFSPR           339
#define PPC_FUNC_MFTB            371
#define PPC_FUNC_MTCRF           144
#define PPC_FUNC_MTDCR           451
#define PPC_FUNC_MTMSR           146
#define PPC_FUNC_MTSPR           467

// WTF-Op
#define PPC_FUNC_MULCHW          168
#define PPC_FUNC_MULCHWU         136
#define PPC_FUNC_MULHHW          40
#define PPC_FUNC_MULHHWU         8

// X-Op
#define PPC_FUNC_MULHW           75
#define PPC_FUNC_MULHWU          11

// WTF-Op
#define PPC_FUNC_MULLHW          424
#define PPC_FUNC_MULLHWU         392

// X-Op
#define PPC_FUNC_MULLW           235
#define PPC_FUNC_NAND            476
#define PPC_FUNC_NEG             104

// WTF-Op
#define PPC_FUNC_NMACCHW         174
#define PPC_FUNC_NMACCHWS        238
#define PPC_FUNC_NMACHHW         46
#define PPC_FUNC_NMACHHWS        110
#define PPC_FUNC_NMACLHW         430
#define PPC_FUNC_NMACLHWS        494

// X-Op
#define PPC_FUNC_NOR             124
#define PPC_FUNC_OR              444
#define PPC_FUNC_ORC             412

// XL-Op
#define PPC_FUNC_RFCI            51
#define PPC_FUNC_RFI             50

// X-Op
#define PPC_FUNC_SLW             24
#define PPC_FUNC_SRAW            792
#define PPC_FUNC_SRAWI           824
#define PPC_FUNC_SRW             536

#define PPC_FUNC_STBUX           247
#define PPC_FUNC_STBX            215
#define PPC_FUNC_STFDUX          759
#define PPC_FUNC_STFDX           727
#define PPC_FUNC_STFSUX          695
#define PPC_FUNC_STFSX           663
#define PPC_FUNC_STFIWX          983
#define PPC_FUNC_STHBRX          918
#define PPC_FUNC_STHUX           439
#define PPC_FUNC_STHX            407
#define PPC_FUNC_STSWI           725
#define PPC_FUNC_STSWX           661
#define PPC_FUNC_STWBRX          662
#define PPC_FUNC_STWCX           150
#define PPC_FUNC_STWUX           183
#define PPC_FUNC_STWX            151

#define PPC_FUNC_SUBF            40
#define PPC_FUNC_SUBFC           8
#define PPC_FUNC_SUBFE           136
#define PPC_FUNC_SUBFME          232
#define PPC_FUNC_SUBFZE          200

#define PPC_FUNC_SYNC            598

#define PPC_FUNC_TLBIA           370
#define PPC_FUNC_TLBRE           946
#define PPC_FUNC_TLBSX           914
#define PPC_FUNC_TLBSYNC         566
#define PPC_FUNC_TLBWE           978

#define PPC_FUNC_TW              4

#define PPC_FUNC_WRTEE           131
#define PPC_FUNC_WRTEEI          163

#define PPC_FUNC_XOR             316

// Registers
#define R0  0
#define R1  1
#define R2  2
#define R3  3
#define R4  4
#define R5  5
#define R6  6
#define R7  7
#define R8  8
#define R9  9
#define R10 10
#define R11 11
#define R12 12
#define R13 13
#define R14 14
#define R15 15
#define R16 16
#define R17 17
#define R18 18
#define R19 19
#define R20 20
#define R21 21
#define R22 22
#define R23 23
#define R24 24
#define R25 25
#define R26 26
#define R27 27
#define R28 28
#define R29 29
#define R30 30
#define R31 31

#define CR0 0
#define CR1 1
#define CR2 2
#define CR3 3
#define CR4 4
#define CR5 5
#define CR6 6
#define CR7 7

#define F0  0
#define F1  1
#define F2  2
#define F3  3
#define F4  4
#define F5  5
#define F6  6
#define F7  7
#define F8  8
#define F9  9
#define F10 10
#define F11 11
#define F12 12
#define F13 13
#define F14 14
#define F15 15
#define F16 16
#define F17 17
#define F18 18
#define F19 19
#define F20 20
#define F21 21
#define F22 22
#define F23 23
#define F24 24
#define F25 25
#define F26 26
#define F27 27
#define F28 28
#define F29 29
#define F30 30
#define F31 31

#define QR0 0
#define QR1 1
#define QR2 2
#define QR3 3
#define QR4 4
#define QR5 5
#define QR6 6
#define QR7 7

#define PPC_NOP (0x60000000)

// Let's make this easier: define a macro for each instruction

#define GEN_B(ppc,dst,aa,lk) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_B); \
	  PPC_SET_LI    (ppc, (dst)); \
	  PPC_SET_AA    (ppc, (aa)); \
	  PPC_SET_LK    (ppc, (lk)); }

#define GEN_MTCTR(ppc,rs) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_MTSPR); \
	  PPC_SET_SPR   (ppc, 0x120); \
	  PPC_SET_RS    (ppc, (rs)); }

#define GEN_MFCTR(ppc,rd) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_MFSPR); \
	  PPC_SET_SPR   (ppc, 0x120); \
	  PPC_SET_RD    (ppc, (rd)); }

#define GEN_ADDIS(ppc,rd,ra,immed) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDIS); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_LIS(ppc,rd,immed) \
	GEN_ADDIS(ppc,rd,0,immed)

#define GEN_LI(ppc,rd,immed) \
	GEN_ADDI(ppc,rd,0,immed)

#define GEN_LWZ(ppc,rd,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_LWZ); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_LWZU(ppc,rd,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_LWZU); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_LWZX(ppc,rd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_LWZX); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_LWZUX(ppc,rd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_LWZUX); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_LHZ(ppc,rd,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_LHZ); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_LHZU(ppc,rd,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_LHZU); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_LHZX(ppc,rd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_LHZX); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_LHZUX(ppc,rd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_LHZUX); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_LHA(ppc,rd,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_LHA); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_LHAU(ppc,rd,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_LHAU); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_LHAX(ppc,rd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_LHAX); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_LHAUX(ppc,rd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_LHAUX); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_LBZ(ppc,rd,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_LBZ); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_LBZU(ppc,rd,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_LBZU); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_LBZX(ppc,rd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_LBZX); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_LBZUX(ppc,rd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_LBZUX); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_EXTSB(ppc,ra,rs) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_EXTSB); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RS    (ppc, (rs)); }

#define GEN_EXTSH(ppc,ra,rs) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_EXTSH); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RS    (ppc, (rs)); }

#define GEN_STB(ppc,rs,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_STB); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_STBU(ppc,rs,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_STBU); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_STBX(ppc,rs,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_STBX); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_STBUX(ppc,rs,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_STBUX); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_STH(ppc,rs,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_STH); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_STHU(ppc,rs,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_STHU); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_STHX(ppc,rs,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC (ppc, PPC_FUNC_STHX); \
	  PPC_SET_RS   (ppc, (rs)); \
	  PPC_SET_RA   (ppc, (ra)); \
	  PPC_SET_RB   (ppc, (rb)); }

#define GEN_STHUX(ppc,rs,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_STHUX); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_STW(ppc,rs,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_STW); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_STWU(ppc,rs,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_STWU); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_STWX(ppc,rs,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_STWX); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_STWUX(ppc,rs,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_STWUX); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_BCTR(ppc) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_XL); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_BCCTR); \
	  PPC_SET_BO    (ppc, 0x14); }

#define GEN_BCTRL(ppc) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_XL); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_BCCTR); \
	  PPC_SET_BO    (ppc, 0x14); \
	  PPC_SET_LK    (ppc, 0x1); }

#define GEN_CMP(ppc,field,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_CMP); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); \
	  PPC_SET_CRF   (ppc, (field)); }

#define GEN_CMPL(ppc,field,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_CMPL); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); \
	  PPC_SET_CRF   (ppc, (field)); }

#define GEN_CMPI(ppc,field,ra,immed) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_CMPI); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); \
	  PPC_SET_CRF   (ppc, (field)); }

#define GEN_CMPLI(ppc,field,ra,immed) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_CMPLI); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); \
	  PPC_SET_CRF   (ppc, (field)); }

#define GEN_BC(ppc,dst,aa,lk,bo,bi) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_BC); \
	  PPC_SET_BD    (ppc, (dst)); \
	  PPC_SET_BO    (ppc, (bo)); \
	  PPC_SET_BI    (ppc, (bi)); \
	  PPC_SET_AA    (ppc, (aa)); \
	  PPC_SET_LK    (ppc, (lk)); }

#define GEN_BNU(ppc,cr,dst,aa,lk) \
	/* FIXME: The docs didn't seem consistant on the BO */ \
	/* BO: Branch if CR bit is 0 */ \
	/* BI: Check UN bit in CR specified */ \
	GEN_BC(ppc, dst, aa, lk, 0x4, (((cr)<<2)+3))

#define GEN_BUN(ppc,cr,dst,aa,lk) \
	/* FIXME: The docs didn't seem consistant on the BO */ \
	/* BO: Branch if CR bit is 1 */ \
	/* BI: Check UN bit in CR specified */ \
	GEN_BC(ppc, dst, aa, lk, 0xc, (((cr)<<2)+3))

#define GEN_BNE(ppc,cr,dst,aa,lk) \
	/* FIXME: The docs didn't seem consistant on the BO */ \
	/* BO: Branch if CR bit is 0 */ \
	/* BI: Check EQ bit in CR specified */ \
	GEN_BC(ppc, dst, aa, lk, 0x4, (((cr)<<2)+2))

#define GEN_BEQ(ppc,cr,dst,aa,lk) \
	/* FIXME: The docs didn't seem consistant on the BO */ \
	/* BO: Branch if CR bit is 1 */ \
	/* BI: Check EQ bit in CR specified */ \
	GEN_BC(ppc, dst, aa, lk, 0xc, (((cr)<<2)+2))

#define GEN_BGT(ppc,cr,dst,aa,lk) \
	/* FIXME: The docs didn't seem consistant on the BO */ \
	/* BO: Branch if CR bit is 1 */ \
	/* BI: Check GT bit in CR specified */ \
	GEN_BC(ppc, dst, aa, lk, 0xc, (((cr)<<2)+1))

#define GEN_BLE(ppc,cr,dst,aa,lk) \
	/* FIXME: The docs didn't seem consistant on the BO */ \
	/* BO: Branch if CR bit is 0 */ \
	/* BI: Check GT bit in CR specified */ \
	GEN_BC(ppc, dst, aa, lk, 0x4, (((cr)<<2)+1))

#define GEN_BGE(ppc,cr,dst,aa,lk) \
	/* FIXME: The docs didn't seem consistant on the BO */ \
	/* BO: Branch if CR bit is 0 */ \
	/* BI: Check LT bit in CR specified */ \
	GEN_BC(ppc, dst, aa, lk, 0x4, (((cr)<<2)+0))

#define GEN_BLT(ppc,cr,dst,aa,lk) \
	/* FIXME: The docs didn't seem consistant on the BO */ \
	/* BO: Branch if CR bit is 1 */ \
	/* BI: Check LT bit in CR specified */ \
	GEN_BC(ppc, dst, aa, lk, 0xc, (((cr)<<2)+0))

#define GEN_ADDI(ppc,rd,ra,immed) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDI); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_SUBI(ppc,rd,ra,immed) \
	GEN_ADDI(ppc,rd,ra,-immed)

#define GEN_RLWIMI(ppc,ra,rs,sh,mb,me) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_RLWIMI); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_SH    (ppc, (sh)); \
	  PPC_SET_MB    (ppc, (mb)); \
	  PPC_SET_ME    (ppc, (me)); }

#define GEN_RLWINM(ppc,ra,rs,sh,mb,me) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_RLWINM); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_SH    (ppc, (sh)); \
	  PPC_SET_MB    (ppc, (mb)); \
	  PPC_SET_ME    (ppc, (me)); }

#define GEN_RLWINM_(ppc,ra,rs,sh,mb,me) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_RLWINM); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_SH    (ppc, (sh)); \
	  PPC_SET_MB    (ppc, (mb)); \
	  PPC_SET_ME    (ppc, (me)); \
	  PPC_SET_CR    (ppc, 0x1); }

#define GEN_CLRLWI(ppc,ra,rs,n) \
	GEN_RLWINM(ppc, ra, rs, 0, n, 31)

#define GEN_CLRRWI(ppc,ra,rs,n) \
	GEN_RLWINM(ppc, ra, rs, 0, 0, 31-n)

#define GEN_CLRLSLWI(ppc,ra,rs,b,n) \
	GEN_RLWINM(ppc, ra, rs, n, b-n, 31-n)

#define GEN_SRWI(ppc,ra,rs,sh) \
	GEN_RLWINM(ppc, ra, rs, 32-sh, sh, 31)

#define GEN_SLWI(ppc,ra,rs,sh) \
	GEN_RLWINM(ppc, ra, rs, sh, 0, 31-sh)

#define GEN_SRAWI(ppc,ra,rs,sh) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_SRAWI); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_SH    (ppc, (sh)); }

#define GEN_SLW(ppc,ra,rs,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_SLW); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_SRW(ppc,ra,rs,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_SRW); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_SRAW(ppc,ra,rs,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_SRAW); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_ANDI(ppc,ra,rs,immed) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_ANDI); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_ORI(ppc,ra,rs,immed) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_ORI); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_XORI(ppc,ra,rs,immed) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_XORI); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_MULLI(ppc,rd,ra,immed) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_MULLI); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_MULLW(ppc,rd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_MULLW); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_MULHW(ppc,rd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_MULHW); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_MULHWU(ppc,rd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_MULHWU); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_DIVW(ppc,rd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_DIVW); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_DIVWU(ppc,rd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_DIVWU); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_ADD(ppc,rd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_ADD); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_SUBF(ppc,rd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_SUBF); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_SUB(ppc,rd,ra,rb) \
	GEN_SUBF(ppc,rd,rb,ra)

#define GEN_SUBFC(ppc,rd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_SUBFC); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_SUBFE(ppc,rd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_SUBFE); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_ADDIC(ppc,rd,ra,immed) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDIC); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_ADDIC_(ppc,rd,ra,immed) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_ADDIC_); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_SUBIC(ppc,rd,ra,immed) \
	GEN_ADDIC(ppc,rd,ra,-immed)

#define GEN_SUBIC_(ppc,rd,ra,immed) \
	GEN_ADDIC_(ppc,rd,ra,-immed)

#define GEN_AND(ppc,ra,rs,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_AND); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_ANDC(ppc,ra,rs,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_ANDC); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_NOR(ppc,ra,rs,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_NOR); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_NOT(ppc,rd,rs) \
	GEN_NOR(ppc,rd,rs,rs)

#define GEN_OR(ppc,ra,rs,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_OR); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_MR(ppc,ra,rs) \
	GEN_OR(ppc,ra,rs,rs)

#define GEN_XOR(ppc,ra,rs,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_XOR); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_BLR(ppc,lk) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_XL); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_BCLR); \
	  PPC_SET_BO    (ppc, 0x14); \
	  PPC_SET_LK    (ppc, (lk)); }

#define GEN_MTLR(ppc,rs) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_MTSPR); \
	  PPC_SET_SPR   (ppc, 0x100); \
	  PPC_SET_RS    (ppc, (rs)); }

#define GEN_MFLR(ppc,rd) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_MFSPR); \
	  PPC_SET_SPR   (ppc, 0x100); \
	  PPC_SET_RD    (ppc, (rd)); }

#define GEN_MTCR(ppc,rs) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_MTCRF); \
	  ppc |= 0xFF << 12; /* Set CRM so it copies all */ \
	  PPC_SET_RS    (ppc, (rs)); }

#define GEN_MFCR(ppc,rd) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_MFCR); \
	  PPC_SET_RD    (ppc, (rd)); }

#define GEN_NEG(ppc,rd,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_NEG); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); }

#define GEN_EQV(ppc,ra,rs,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_EQV); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_ADDME(ppc,rd,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_ADDME); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); }

#define GEN_ADDZE(ppc,rd,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_ADDZE); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); }

#define GEN_ADDC(ppc,rd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_ADDC); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_ADDE(ppc,rd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_ADDE); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_SUBFIC(ppc,rd,ra,immed) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_SUBFIC); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_SUBFME(ppc,rd,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_SUBFME); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); }

#define GEN_SUBFZE(ppc,rd,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_SUBFZE); \
	  PPC_SET_RD    (ppc, (rd)); \
	  PPC_SET_RA    (ppc, (ra)); }

#define GEN_STFD(ppc,fs,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_STFD); \
	  PPC_SET_RS    (ppc, (fs)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_STFDU(ppc,fs,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_STFDU); \
	  PPC_SET_RS    (ppc, (fs)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_STFDX(ppc,fs,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_STFDX); \
	  PPC_SET_RS    (ppc, (fs)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_STFDUX(ppc,fs,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_STFDUX); \
	  PPC_SET_RS    (ppc, (fs)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_STFS(ppc,fs,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_STFS); \
	  PPC_SET_RS    (ppc, (fs)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_STFSU(ppc,fs,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_STFSU); \
	  PPC_SET_RS    (ppc, (fs)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_STFSX(ppc,fs,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_STFSX); \
	  PPC_SET_RS    (ppc, (fs)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_STFSUX(ppc,fs,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_STFSUX); \
	  PPC_SET_RS    (ppc, (fs)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_LFD(ppc,fd,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_LFD); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_LFDU(ppc,fd,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_LFDU); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_LFDX(ppc,fd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_LFDX); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_LFDUX(ppc,fd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_LFDUX); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_LFS(ppc,fd,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_LFS); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_LFSU(ppc,fd,immed,ra) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_LFSU); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_LFSX(ppc,fd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_LFSX); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_LFSUX(ppc,fd,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_LFSUX); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_PSQ_L(ppc,fd,imm12,ra,qri) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_PSQ_L); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  ppc |= 0x8000; \
	  PPC_SET_QRI   (ppc, (qri)); \
	  PPC_SET_IMM12 (ppc, (imm12)); }

#define GEN_FADD(ppc,fd,fa,fb,dbl) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, ((dbl) ? PPC_OPCODE_FPD : PPC_OPCODE_FPS)); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_FADD); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RA    (ppc, (fa)); \
	  PPC_SET_RB    (ppc, (fb)); }

#define GEN_FSUB(ppc,fd,fa,fb,dbl) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, ((dbl) ? PPC_OPCODE_FPD : PPC_OPCODE_FPS)); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_FSUB); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RA    (ppc, (fa)); \
	  PPC_SET_RB    (ppc, (fb)); }

#define GEN_FMUL(ppc,fd,fa,fc,dbl) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, ((dbl) ? PPC_OPCODE_FPD : PPC_OPCODE_FPS)); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_FMUL); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RA    (ppc, (fa)); \
	  PPC_SET_RC    (ppc, (fc)); }

#define GEN_FDIV(ppc,fd,fa,fb,dbl) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, ((dbl) ? PPC_OPCODE_FPD : PPC_OPCODE_FPS)); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_FDIV); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RA    (ppc, (fa)); \
	  PPC_SET_RB    (ppc, (fb)); }

#define GEN_FABS(ppc,fd,fb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_FABS); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RB    (ppc, (fb)); }

#define GEN_FMR(ppc,fd,fb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_FMR); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RB    (ppc, (fb)); }

#define GEN_FNEG(ppc,fd,fb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_FNEG); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RB    (ppc, (fb)); }

#define GEN_FRSP(ppc,fd,fb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_FRSP); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RB    (ppc, (fb)); }

#define GEN_FCTIW(ppc,fd,fb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_FCTIW); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RB    (ppc, (fb)); }

#define GEN_FCTIWZ(ppc,fd,fb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_FCTIWZ); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RB    (ppc, (fb)); }

#define GEN_STFIWX(ppc,fs,ra,rb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_X); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_STFIWX); \
	  PPC_SET_RS    (ppc, (fs)); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RB    (ppc, (rb)); }

#define GEN_MTFSFI(ppc,field,immed) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_MTFSFI); \
	  PPC_SET_CRF   (ppc, (field)); \
	  PPC_SET_RB    (ppc, ((immed)<<1)); }

#define GEN_MTFSF(ppc,fields,fb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_MTFSF); \
	  PPC_SET_FM    (ppc, (fields)); \
	  PPC_SET_RB    (ppc, (fb)); }

#define GEN_FCMPU(ppc,field,fa,fb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_FCMPU); \
	  PPC_SET_RA    (ppc, (fa)); \
	  PPC_SET_RB    (ppc, (fb)); \
	  PPC_SET_CRF   (ppc, (field)); }

#define GEN_FRSQRTE(ppc,fd,fb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_FRSQRTE); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RB    (ppc, (fb)); }

#define GEN_FSEL(ppc,fd,fa,fc,fb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_FPD); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_FSEL); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RA    (ppc, (fa)); \
	  PPC_SET_RB    (ppc, (fb)); \
	  PPC_SET_RC    (ppc, (fc)); }

#define GEN_FRES(ppc,fd,fb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_FPS); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_FRES); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RB    (ppc, (fb)); }

#define GEN_FMADD(ppc,fd,fa,fc,fb,dbl) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, ((dbl) ? PPC_OPCODE_FPD : PPC_OPCODE_FPS)); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_FMADD); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RA    (ppc, (fa)); \
	  PPC_SET_RB    (ppc, (fb)); \
	  PPC_SET_RC    (ppc, (fc)); }

#define GEN_FMSUB(ppc,fd,fa,fc,fb,dbl) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, ((dbl) ? PPC_OPCODE_FPD : PPC_OPCODE_FPS)); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_FMSUB); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RA    (ppc, (fa)); \
	  PPC_SET_RB    (ppc, (fb)); \
	  PPC_SET_RC    (ppc, (fc)); }

#define GEN_FNMADD(ppc,fd,fa,fc,fb,dbl) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, ((dbl) ? PPC_OPCODE_FPD : PPC_OPCODE_FPS)); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_FNMADD); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RA    (ppc, (fa)); \
	  PPC_SET_RB    (ppc, (fb)); \
	  PPC_SET_RC    (ppc, (fc)); }

#define GEN_FNMSUB(ppc,fd,fa,fc,fb,dbl) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, ((dbl) ? PPC_OPCODE_FPD : PPC_OPCODE_FPS)); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_FNMSUB); \
	  PPC_SET_RD    (ppc, (fd)); \
	  PPC_SET_RA    (ppc, (fa)); \
	  PPC_SET_RB    (ppc, (fb)); \
	  PPC_SET_RC    (ppc, (fc)); }

#define GEN_BCLR(ppc,lk,bo,bi) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_XL); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_BCLR); \
	  PPC_SET_BO    (ppc, (bo)); \
	  PPC_SET_BI    (ppc, (bi)); \
	  PPC_SET_LK    (ppc, (lk)); }

#define GEN_BNELR(ppc,cr,lk) \
	/* NOTE: This branch is marked unlikely to be taken */ \
	/* BO: Branch if CR bit is 0 */ \
	/* BI: Check EQ bit in CR specified */ \
	GEN_BCLR(ppc, lk, 0x4, (((cr)<<2)+2))

#define GEN_BLELR(ppc,cr,lk) \
	/* NOTE: This branch is marked unlikely to be taken */ \
	/* BO: Branch if CR bit is 0 */ \
	/* BI: Check GT bit in CR specified */ \
	GEN_BCLR(ppc, lk, 0x4, (((cr)<<2)+1))

#define GEN_ANDIS(ppc,ra,rs,immed) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_ANDIS); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_ORIS(ppc,ra,rs,immed) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_ORIS); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_XORIS(ppc,ra,rs,immed) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_XORIS); \
	  PPC_SET_RA    (ppc, (ra)); \
	  PPC_SET_RS    (ppc, (rs)); \
	  PPC_SET_IMMED (ppc, (immed)); }

#define GEN_CRNOR(ppc,cd,ca,cb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_XL); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_CRNOR); \
	  PPC_SET_RD    (ppc, (cd)); \
	  PPC_SET_RA    (ppc, (ca)); \
	  PPC_SET_RB    (ppc, (cb)); }

#define GEN_CROR(ppc,cd,ca,cb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_XL); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_CROR); \
	  PPC_SET_RD    (ppc, (cd)); \
	  PPC_SET_RA    (ppc, (ca)); \
	  PPC_SET_RB    (ppc, (cb)); }

#define GEN_CRORC(ppc,cd,ca,cb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_XL); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_CRORC); \
	  PPC_SET_RD    (ppc, (cd)); \
	  PPC_SET_RA    (ppc, (ca)); \
	  PPC_SET_RB    (ppc, (cb)); }

#define GEN_CRXOR(ppc,cd,ca,cb) \
	{ ppc = NEW_PPC_INSTR(); \
	  PPC_SET_OPCODE(ppc, PPC_OPCODE_XL); \
	  PPC_SET_FUNC  (ppc, PPC_FUNC_CRXOR); \
	  PPC_SET_RD    (ppc, (cd)); \
	  PPC_SET_RA    (ppc, (ca)); \
	  PPC_SET_RB    (ppc, (cb)); }

#endif
