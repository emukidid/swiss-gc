#ifndef __ELF_H
#define __ELF_H

#include "elf_abi.h"

int valid_elf_image(unsigned int addr);
unsigned int load_elf_image(unsigned int addr);

#endif
