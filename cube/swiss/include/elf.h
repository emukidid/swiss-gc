#ifndef __ELF_H
#define __ELF_H

#include "elf_abi.h"

int valid_elf_image(void *addr);
unsigned int load_elf_image(void *addr);

#endif
