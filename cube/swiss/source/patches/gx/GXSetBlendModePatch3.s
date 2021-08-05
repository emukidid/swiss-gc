#include "../asm.h"
#define _LANGUAGE_ASSEMBLY
#include "../../../../reservedarea.h"

.globl GXSetBlendModePatch3
GXSetBlendModePatch3:
	lwz		%r9, 560 (%r13)
	cmpwi	%cr0, %r3, 1
	cmpwi	%cr1, %r3, 2
	cmpwi	%cr5, %r3, 3
	cror	4*%cr0+so, 4*%cr0+eq, 4*%cr5+eq
	insrwi	%r9, %r4, 3, 21
	cmpwi	%cr6, %r4, 0
	insrwi	%r9, %r5, 3, 24
	cmpwi	%cr7, %r5, 0
	crand	4*%cr6+so, 4*%cr6+gt, 4*%cr0+eq
	crand	4*%cr7+so, 4*%cr7+gt, 4*%cr6+so
	insrwi	%r9, %r6, 4, 16
	mfcr	%r0
	rlwimi	%r9, %r0, 4, 31, 31
	rlwimi	%r9, %r0, 8, 30, 30
	rlwimi	%r9, %r0, 2, 20, 20
	stw		%r9, 560 (%r13)
	rlwinm	%r0, %r0, 2, 29, 29
	andc	%r9, %r9, %r0
	lis		%r8, 0xCC01
	li		%r7, 0x61
	stb		%r7, -0x8000 (%r8)
	stw		%r9, -0x8000 (%r8)
	sth		%r8, 2 (%r13)
	blr

.globl GXSetBlendModePatch3_length
GXSetBlendModePatch3_length:
.long (GXSetBlendModePatch3_length - GXSetBlendModePatch3)