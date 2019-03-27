#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl VIConfigure540p60
VIConfigure540p60:
	lhz			%r0, 2 (%r3)
	srwi		%r5, %r0, 2
	cmpwi		%r5, 1
	beq			1f
	cmpwi		%r5, 4
	bne			2f
1:	subi		%r5, %r5, 1
	insrwi		%r0, %r5, 14, 16
2:	lis			%r4, VAR_AREA
	stw			%r5, VAR_TVMODE (%r4)
	insrwi		%r0, %r5, 14, 0
	li			%r8, 0
	li			%r7, 0
	lhz			%r6, 8 (%r3)
	slwi		%r6, %r6, 1
	cmpwi		%r6, 480
	ble			6f
	cmpwi		%r6, 574
	ble			4f
	oris		%r0, %r0, 2
	li			%r8, 1
	lhz			%r6, 8 (%r3)
	lhz			%r5, 6 (%r3)
	cmpw		%r5, %r6
	bgt			3f
	cmpwi		%r5, 264
	ble			3f
	clrrwi		%r5, %r5, 1
	sth			%r5, 8 (%r3)
	mr			%r6, %r5
3:	subfic		%r5, %r6, 540
	srawi		%r5, %r5, 1
	b			7f
4:	lhz			%r6, 6 (%r3)
	cmpwi		%r6, 240
	ble			5f
	subi		%r6, %r6, 264
	slwi		%r6, %r6, 1
	addi		%r6, %r6, 240
	sth			%r6, 6 (%r3)
5:	sth			%r6, 8 (%r3)
	slwi		%r6, %r6, 1
6:	subfic		%r5, %r6, 480
	srwi		%r5, %r5, 1
7:	sth			%r5, 12 (%r3)
	sth			%r6, 16 (%r3)
	stb			%r7, 22 (%r3)
	stb			%r8, VAR_VFILTER_ON (%r4)
	stw			%r0, 0 (%r3)

.globl VIConfigure540p60_length
VIConfigure540p60_length:
.long (VIConfigure540p60_length - VIConfigure540p60)