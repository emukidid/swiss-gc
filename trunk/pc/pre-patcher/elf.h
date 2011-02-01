#ifndef __elf_h
#define __elf_h

int valid_elf_image(unsigned long addr);
unsigned long load_elf_image(unsigned long addr);

#endif
