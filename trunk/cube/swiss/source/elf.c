/*
 * Copyright (c) 2001 William L. Pitts
 * Modifications 2013 emu_kidid
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are freely
 * permitted provided that the above copyright notice and this
 * paragraph and the following disclaimer are duplicated in all
 * such forms.
 *
 * This software is provided "AS IS" and without any express or
 * implied warranties, including, without limitation, the implied
 * warranties of merchantability and fitness for a particular
 * purpose.
 */

#include <gccore.h>
#include "elf.h"

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif


/* ======================================================================
 * Determine if a valid ELF image exists at the given memory location.
 * First looks at the ELF header magic field, the makes sure that it is
 * executable and makes sure that it is for a PowerPC.
 * ====================================================================== */
int valid_elf_image(unsigned int addr)
{
	Elf32_Ehdr* ehdr;		/* Elf header structure pointer */

	/* -------------------------------------------------- */

	ehdr = (Elf32_Ehdr *)addr;

	if (!IS_ELF(*ehdr))
	{
	//	printf("## No elf image at address 0x%08lx\n", addr);
		return 0;
	}

	if (ehdr->e_type != ET_EXEC)
	{
		//printf("## Not a 32-bit elf image at address 0x%08lx\n", addr);
		return 0;
	}

/*
	if (ehdr->e_machine != EM_PPC) {
		//printf ("## Not a PowerPC elf image at address 0x%08lx\n",
			addr);
		return 0;
	}
*/
	return 1;
}

#if 0
/* ======================================================================
 * A very simple elf loader, assumes the image is valid, returns the
 * entry point address.
 * ====================================================================== */
unsigned int load_elf_image(unsigned int addr)
{
	Elf32_Ehdr* ehdr;
	Elf32_Shdr* shdr;
	unsigned char* image;
	int i;

	ehdr = (Elf32_Ehdr *)addr;

	/* Load each appropriate section */
	for (i = 0; i < ehdr->e_shnum; ++i)
	{
		shdr = (Elf32_Shdr *)(addr + ehdr->e_shoff + (i * sizeof(Elf32_Shdr)));

		if (!(shdr->sh_flags & SHF_ALLOC) || shdr->sh_addr == 0 || shdr->sh_size == 0)
		{
			continue;
		}

		shdr->sh_addr &= 0x3FFFFFFF;
		shdr->sh_addr |= 0x80000000;

		if (shdr->sh_type == SHT_NOBITS)
		{
			memset((void*)shdr->sh_addr, 0, shdr->sh_size);
		}
		else
		{
			image = (unsigned char*)addr + shdr->sh_offset;
			memcpy((void*)shdr->sh_addr, (const void*)image, shdr->sh_size);
		}
		//flush_code((void*)shdr->sh_addr, shdr->sh_size);
	}

	//printf("loading elf done!\n");

	return (ehdr->e_entry & 0x3FFFFFFF) | 0x80000000;
}
#endif
