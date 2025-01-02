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
 **/

/* Example usage (creating add r1, r2, r3):
    PowerPC_instr x = NEW_PPC_INSTR();
    PPC_SET_OPCODE(x, PPC_OPCODE_X);
    PPC_SET_FUNC(x, PPC_FUNC_ADD);
    PPC_SET_RD(x, R1);
    PPC_SET_RA(x, R2);
    PPC_SET_RB(x, R3);
 */

#ifndef POWERPC_H
#define POWERPC_H

#include <assert.h> // For assertions

// Type definitions
typedef unsigned int PowerPC_instr;

// Macros
#define NEW_PPC_INSTR() 0

// Fields
#define PPC_OPCODE_MASK  0x3F
#define PPC_OPCODE_SHIFT 26
#define PPC_GET_OPCODE(instr)       (((instr) >> PPC_OPCODE_SHIFT) & PPC_OPCODE_MASK)
#define PPC_SET_OPCODE(instr, opcode) ((instr) |= ((opcode) & PPC_OPCODE_MASK) << PPC_OPCODE_SHIFT)

#define PPC_REG_MASK     0x1F

#define PPC_RD_MASK      PPC_REG_MASK
#define PPC_RD_SHIFT     21
#define PPC_GET_RD(instr)           (((instr) >> PPC_RD_SHIFT) & PPC_RD_MASK)
#define PPC_SET_RD(instr, rd)         ((instr) |= ((rd) & PPC_RD_MASK) << PPC_RD_SHIFT)

#define PPC_RA_MASK      PPC_REG_MASK
#define PPC_RA_SHIFT     16
#define PPC_GET_RA(instr)           (((instr) >> PPC_RA_SHIFT) & PPC_RA_MASK)
#define PPC_SET_RA(instr, ra)         ((instr) |= ((ra) & PPC_RA_MASK) << PPC_RA_SHIFT)

#define PPC_RB_MASK      PPC_REG_MASK
#define PPC_RB_SHIFT     11
#define PPC_GET_RB(instr)           (((instr) >> PPC_RB_SHIFT) & PPC_RB_MASK)
#define PPC_SET_RB(instr, rb)         ((instr) |= ((rb) & PPC_RB_MASK) << PPC_RB_SHIFT)

#define PPC_OE_MASK      0x1
#define PPC_OE_SHIFT     10
#define PPC_GET_OE(instr)           (((instr) >> PPC_OE_SHIFT) & PPC_OE_MASK)
#define PPC_SET_OE(instr, oe)         ((instr) |= ((oe) & PPC_OE_MASK) << PPC_OE_SHIFT)

#define PPC_FUNC_MASK    0x3FF
#define PPC_FUNC_SHIFT   1
#define PPC_GET_FUNC(instr)         (((instr) >> PPC_FUNC_SHIFT) & PPC_FUNC_MASK)
#define PPC_SET_FUNC(instr, func)     ((instr) |= ((func) & PPC_FUNC_MASK) << PPC_FUNC_SHIFT)

#define PPC_CR_MASK      0x1
#define PPC_CR_SHIFT     0
#define PPC_GET_CR(instr)           (((instr) >> PPC_CR_SHIFT) & PPC_CR_MASK)
#define PPC_SET_CR(instr, cr)         ((instr) |= ((cr) & PPC_CR_MASK) << PPC_CR_SHIFT)

#define PPC_QRI_MASK     0x7
#define PPC_QRI_SHIFT    12
#define PPC_GET_QRI(instr)          (((instr) >> PPC_QRI_SHIFT) & PPC_QRI_MASK)
#define PPC_SET_QRI(instr, qri)       ((instr) |= ((qri) & PPC_QRI_MASK) << PPC_QRI_SHIFT)

#define PPC_IMM12_MASK   0xFFF
#define PPC_IMM12_SHIFT  0
#define PPC_GET_IMM12(instr)        (((instr) >> PPC_IMM12_SHIFT) & PPC_IMM12_MASK)
#define PPC_SET_IMM12(instr, imm12)   ((instr) |= ((imm12) & PPC_IMM12_MASK) << PPC_IMM12_SHIFT)

#define PPC_IMMED_MASK   0xFFFF
#define PPC_IMMED_SHIFT  0
#define PPC_GET_IMMED(instr)        (((instr) >> PPC_IMMED_SHIFT) & PPC_IMMED_MASK)
#define PPC_SET_IMMED(instr, immed)   ((instr) |= ((immed) & PPC_IMMED_MASK) << PPC_IMMED_SHIFT)

#define PPC_LI_MASK      0xFFFFFF
#define PPC_LI_SHIFT     2
#define PPC_GET_LI(instr)           (((instr) >> PPC_LI_SHIFT) & PPC_LI_MASK)
#define PPC_SET_LI(instr, li)         ((instr) |= ((li) & PPC_LI_MASK) << PPC_LI_SHIFT)

#define PPC_AA_MASK      0x1
#define PPC_AA_SHIFT     1
#define PPC_GET_AA(instr)           (((instr) >> PPC_AA_SHIFT) & PPC_AA_MASK)
#define PPC_SET_AA(instr, aa)         ((instr) |= ((aa) & PPC_AA_MASK) << PPC_AA_SHIFT)

#define PPC_LK_MASK      0x1
#define PPC_LK_SHIFT     0
#define PPC_GET_LK(instr)           (((instr) >> PPC_LK_SHIFT) & PPC_LK_MASK)
#define PPC_SET_LK(instr, lk)         ((instr) |= ((lk) & PPC_LK_MASK) << PPC_LK_SHIFT)

#define PPC_GET_BO PPC_GET_RD
#define PPC_SET_BO PPC_SET_RD
#define PPC_GET_BI PPC_GET_RA
#define PPC_SET_BI PPC_SET_RA

#define PPC_BD_MASK      0x3FFF
#define PPC_BD_SHIFT     2
#define PPC_GET_BD(instr)           (((instr) >> PPC_BD_SHIFT) & PPC_BD_MASK)
#define PPC_SET_BD(instr, bd)         ((instr) |= ((bd) & PPC_BD_MASK) << PPC_BD_SHIFT)

#define PPC_CRF_MASK     0x7
#define PPC_CRF_SHIFT    23
#define PPC_GET_CRF(instr)          (((instr) >> PPC_CRF_SHIFT) & PPC_CRF_MASK)
#define PPC_SET_CRF(instr, crf)       ((instr) |= ((crf) & PPC_CRF_MASK) << PPC_CRF_SHIFT)

#define PPC_SPR_MASK     0x3FF
#define PPC_SPR_SHIFT    11
#define PPC_GET_SPR(instr)          (((instr) >> PPC_SPR_SHIFT) & PPC_SPR_MASK)
#define PPC_SET_SPR(instr, spr)       ((instr) |= ((spr) & PPC_SPR_MASK) << PPC_SPR_SHIFT)

#define PPC_MB_MASK      0x1F
#define PPC_MB_SHIFT     6
#define PPC_GET_MB(instr)           (((instr) >> PPC_MB_SHIFT) & PPC_MB_MASK)
#define PPC_SET_MB(instr, mb)         ((instr) |= ((mb) & PPC_MB_MASK) << PPC_MB_SHIFT)

#define PPC_ME_MASK      0x1F
#define PPC_ME_SHIFT     1
#define PPC_GET_ME(instr)           (((instr) >> PPC_ME_SHIFT) & PPC_ME_MASK)
#define PPC_SET_ME(instr, me)         ((instr) |= ((me) & PPC_ME_MASK) << PPC_ME_SHIFT)

#define PPC_FM_MASK      0xFF
#define PPC_FM_SHIFT     17
#define PPC_GET_FM(instr)           (((instr) >> PPC_FM_SHIFT) & PPC_FM_MASK)
#define PPC_SET_FM(instr, fm)         ((instr) |= ((fm) & PPC_FM_MASK) << PPC_FM_SHIFT)

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
#define R7
