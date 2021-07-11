/****************************************************************************
* SideStep DOL Loading
*
* This module runs a DOL file from Auxilliary RAM. This removes any memory
* issues which might occur - and also means you can easily overwrite yourself!
*
* softdev March 2007
***************************************************************************/
#include <gccore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <network.h>
#include <asndlib.h>
#include <ogc/lwp_threads.h>
#include <gctypes.h>

#include "sidestep.h"
#include "ssaram.h"
#include "elf.h"
#include "patcher.h"

#define ARAMSTART 0x8000

/*** A global or two ***/
static DOLHEADER *dolhdr;
static u32 minaddress = 0;
static u32 maxaddress = 0;
static u32 _entrypoint, _dst, _src, _len;

typedef int (*BOOTSTUB) (u32 entrypoint, u32 dst, u32 src, int len, u32 invlen, u32 invaddress);

/*--- Auxilliary RAM Support ----------------------------------------------*/
/****************************************************************************
* ARAMStub
*
* This is an assembly routine and should only be called through ARAMRun
* *DO NOT CALL DIRECTLY!*
****************************************************************************/
static void ARAMStub(void)
{
	/*** The routine expects to receive
             R3 = entrypoint
             R4 = Destination in main RAM
             R5 = Source from ARAM
             R6 = Data length
             R7 = Invalidate Length / 32
             R8 = Invalidate Start Address
        ***/

    asm("mtctr 7");
    asm("Invalidate:");
    asm("dcbi 0,8");
    asm("addi 8,8,32");
    asm("bdnz Invalidate");

    asm("lis 8,0xcc00");
    asm("ori 8,8,0x3004");
    asm("lis 7,0");
    asm("stw 7,0(8)");

    asm("mfmsr 8");
    asm("rlwinm 8,8,0,17,15");
    asm("ori 8,8,8194");
    asm("mtmsr 8");

    asm("lis  7,0xcc00");
    asm("ori 7,7,0x5020");
    asm("stw 4,0(7)");		/*** Store Memory Address ***/
    asm("stw 5,4(7)");		/*** Store ARAM Address ***/
    asm("stw 6,8(7)");		/*** Store Length ***/

    asm("lis  7,0xcc00");
    asm("ori 7,7,0x500a");
    asm("WaitDMA:");
    asm("lhz 5,0(7)");
    asm("andi. 5,5,0x200");
    asm("cmpwi 5,5,0");
    asm("bne WaitDMA");		/*** Wait DMA Complete ***/

    /*** Update exceptions ***/
    asm("lis 8,0x8000");
    asm("lis 5,0x4c00");
    asm("ori 5,5,0x64");
    asm("stw 5,0x100(8)");
    asm("stw 5,0x200(8)");
    asm("stw 5,0x300(8)");
    asm("stw 5,0x400(8)");
    asm("stw 5,0x500(8)");
    asm("stw 5,0x600(8)");
    asm("stw 5,0x700(8)");
    asm("stw 5,0x800(8)");
    asm("stw 5,0x900(8)");

    /*** Flush it all again ***/
    asm("lis 7,0x30");
    asm("lis 8,0x8000");
    asm("mtctr 7");
    asm("flush:");
    asm("dcbst 0,8");
    asm("sync");
    asm("icbi 0,8");
    asm("addi 8,8,8");
    asm("bdnz flush");
    asm("isync");

    /*** Enable external control ***/
    asm("lis 8,0x8000");
    asm("ori 8,8,0x000c");
    asm("mtear 8");

    /*** Party! ***/
    asm("mtlr 3");
    asm("li 0,0");
    asm("lis 1,0x8160");
    asm("li 2,0");
    asm("li 3,0");
    asm("li 4,0");
    asm("li 5,0");
    asm("li 6,0");
    asm("li 7,0");
    asm("li 8,0");
    asm("li 9,0");
    asm("li 10,0");
    asm("li 11,0");
    asm("li 12,0");
    asm("li 13,0");
    asm("li 14,0");
    asm("li 15,0");
    asm("li 16,0");
    asm("li 17,0");
    asm("li 18,0");
    asm("li 19,0");
    asm("li 20,0");
    asm("li 21,0");
    asm("li 22,0");
    asm("li 23,0");
    asm("li 24,0");
    asm("li 25,0");
    asm("li 26,0");
    asm("li 27,0");
    asm("li 28,0");
    asm("li 29,0");
    asm("li 30,0");
    asm("li 31,0");
    asm("blr");			/*** Boot DOL ***/

}

void ARAMRunStub(void)
{
	BOOTSTUB stub;
	char *p;
	char *s = (char *) ARAMStub;

	/*** Copy ARAMStub to 81300000 ***/
	if (_dst + _len < 0x81300000) p = (void *) 0x81300000;
	else p = (void *) 0x80003100;
	memcpy(p, s, ARAMRunStub - ARAMStub);

	/*** Round length to 32 bytes ***/
	if (_len & 0x1f) _len = (_len & ~0x1f) + 0x20;

	/*** Flush everything! ***/
	DCFlushRange((void *) 0x80000000, 0x1800000);

	/*** Boot the bugger :D ***/
	stub = (BOOTSTUB) p;
	stub((u32) _entrypoint, _dst, _src, _len | 0x80000000, _len >> 5, _dst);
}

/****************************************************************************
* ARAMRun
*
* This actually runs the new DOL ... eventually ;)
****************************************************************************/
void ARAMRun(u32 entrypoint, u32 dst, u32 src, u32 len)
{
	_entrypoint = entrypoint;
	_dst = dst;
	_src = src;
	_len = len;
	
	/*** Shutdown libOGC ***/
	GX_AbortFrame();
	ASND_End();
	u32 bi2Addr = *(volatile u32*)0x800000F4;
	SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);
	*(volatile u32*)0x800000F4 = bi2Addr;
	install_code(1);	// Must happen here because libOGC likes to write over all exception handlers.
	/*** Shutdown all threads and exit to this method ***/
	__lwp_thread_stopmultitasking(ARAMRunStub);
}

/****************************************************************************
* ARAMClear
*
* To make life easy, just clear out the Auxilliary RAM completely.
****************************************************************************/
static void ARAMClear(void)
{
  int i;
  unsigned char *buffer = memalign(32, 2048); /*** A little 2k buffer ***/

  memset(buffer, 0, 2048);
  DCFlushRange(buffer, 2048);

  for (i = ARAMSTART; i < 0x1000000; i += 2048)
  {
    ARAMPut(buffer, (char *) i, 2048);
    while (AR_GetDMAStatus());
  }

  free(buffer);
}

/*--- DOL Decoding functions -----------------------------------------------*/
/****************************************************************************
* DOLMinMax
*
* Calculate the DOL minimum and maximum memory addresses
****************************************************************************/
static void DOLMinMax(DOLHEADER * dol)
{
  int i;

  maxaddress = 0;
  minaddress = 0x87100000;

  /*** Go through DOL sections ***/
  /*** Text sections ***/
  for (i = 0; i < MAXTEXTSECTION; i++)
  {
    if (dol->textAddress[i] && dol->textLength[i])
    {
      if (dol->textAddress[i] < minaddress)
        minaddress = dol->textAddress[i];
      if ((dol->textAddress[i] + dol->textLength[i]) > maxaddress) 
        maxaddress = dol->textAddress[i] + dol->textLength[i];
    }
  }

  /*** Data sections ***/
  for (i = 0; i < MAXDATASECTION; i++)
  {
    if (dol->dataAddress[i] && dol->dataLength[i])
    {
      if (dol->dataAddress[i] < minaddress)
        minaddress = dol->dataAddress[i];
      if ((dol->dataAddress[i] + dol->dataLength[i]) > maxaddress)
        maxaddress = dol->dataAddress[i] + dol->dataLength[i];
    }
  }

  /*** And of course, any BSS section ***/
  if (dol->bssAddress)
  {
    if ((dol->bssAddress + dol->bssLength) > maxaddress)
      maxaddress = dol->bssAddress + dol->bssLength;
  }

  /*** Some OLD dols, Xrick in particular, require ~128k clear memory ***/
  maxaddress += 0x20000;
}

u32 DOLSize(DOLHEADER *dol)
{
  u32 sizeinbytes;
  int i;

  sizeinbytes = DOLHDRLENGTH;

  /*** Go through DOL sections ***/
  /*** Text sections ***/
  for (i = 0; i < MAXTEXTSECTION; i++)
  {
    if (dol->textOffset[i])
    {
      if ((dol->textOffset[i] + dol->textLength[i]) > sizeinbytes)
        sizeinbytes = dol->textOffset[i] + dol->textLength[i];
    }
  }

  /*** Data sections ***/
  for (i = 0; i < MAXDATASECTION; i++)
  {
    if (dol->dataOffset[i])
    {
      if ((dol->dataOffset[i] + dol->dataLength[i]) > sizeinbytes)
        sizeinbytes = dol->dataOffset[i] + dol->dataLength[i];
    }
  }

  /*** Return DOL size ***/
  return sizeinbytes;
}

/****************************************************************************
* DOLtoARAM
*
* Moves the DOL from main memory to ARAM, positioning as it goes
*
* Pass in a memory pointer to a previously loaded DOL
****************************************************************************/
int DOLtoARAM(unsigned char *dol, int argc, char *argv[])
{
  u32 sizeinbytes;
  int i;
  struct __argv dolargs;

  /*** Make sure ARAM subsystem is alive! ***/
  AR_Init(NULL, 0); /*** No stack - we need it all ***/
  ARAMClear();

  /*** Get DOL header ***/
  dolhdr = (DOLHEADER *) dol;

  /*** First, does this look like a DOL? ***/
  if (dolhdr->textOffset[0] != DOLHDRLENGTH && dolhdr->textOffset[0] != 0x0620)	// DOLX style 
    return 0;

  /*** Get DOL stats ***/
  DOLMinMax(dolhdr);
  sizeinbytes = maxaddress - minaddress;

  /*** Move all DOL sections into ARAM ***/
  /*** Move text sections ***/
  for (i = 0; i < MAXTEXTSECTION; i++)
  {
    /*** This may seem strange, but in developing d0lLZ we found some with section addresses with zero length ***/
    if (dolhdr->textAddress[i] && dolhdr->textLength[i])
    {
      ARAMPut(dol + dolhdr->textOffset[i], (char *) ((dolhdr->textAddress[i] - minaddress) + ARAMSTART),
              dolhdr->textLength[i]);
    }
  }

  /*** Move data sections ***/
  for (i = 0; i < MAXDATASECTION; i++)
  {
    if (dolhdr->dataAddress[i] && dolhdr->dataLength[i])
    {
      ARAMPut(dol + dolhdr->dataOffset[i], (char *) ((dolhdr->dataAddress[i] - minaddress) + ARAMSTART),
              dolhdr->dataLength[i]);
    }
  }

  /*** Pass a command line ***/
  if (argc)
  {
    dolargs.argvMagic = ARGV_MAGIC;
    dolargs.argc = argc;
    dolargs.length = 1;

    for (i = 0; i < argc; i++)
    {
      size_t argLength = strlen(argv[i]) + 1;
      dolargs.length += argLength;
    }
    dolargs.commandLine = malloc(dolargs.length);

    unsigned int position = 0;
    for (i = 0; i < argc; i++)
    {
      size_t argLength = strlen(argv[i]) + 1;
      memcpy(dolargs.commandLine + position, argv[i], argLength);
      position += argLength;
    }
    dolargs.commandLine[dolargs.length - 1] = '\0';
    DCStoreRange(dolargs.commandLine, dolargs.length);

    ARAMPut((unsigned char *) &dolargs, (char *) (dolhdr->entryPoint - minaddress + 8 + ARAMSTART), sizeof(struct __argv));
  }

  /*** Now go run it ***/
  ARAMRun(dolhdr->entryPoint, minaddress, ARAMSTART, sizeinbytes);

  /*** Will never return ***/
  return 1;
}

static void ELFMinMax(Elf32_Ehdr *ehdr, Elf32_Phdr *phdr)
{
  int i;

  maxaddress = 0;
  minaddress = 0x87100000;

  /*** Go through ELF segments ***/
  for (i = 0; i < ehdr->e_phnum; i++)
  {
    if (phdr[i].p_type == PT_LOAD)
    {
      if (phdr[i].p_vaddr < minaddress)
        minaddress = phdr[i].p_vaddr;
      if ((phdr[i].p_vaddr + phdr[i].p_memsz) > maxaddress)
        maxaddress = phdr[i].p_vaddr + phdr[i].p_memsz;
    }
  }
}

int ELFtoARAM(unsigned char *elf, int argc, char *argv[])
{
  Elf32_Ehdr *ehdr;
  Elf32_Phdr *phdr;
  u32 sizeinbytes;
  int i;
  struct __argv args;

  /*** Make sure ARAM subsystem is alive! ***/
  AR_Init(NULL, 0); /*** No stack - we need it all ***/
  ARAMClear();

  /*** First, does this look like an ELF? ***/
  if (!valid_elf_image(elf))
    return 0;

  /*** Get ELF headers ***/
  ehdr = (Elf32_Ehdr *) elf;
  phdr = (Elf32_Phdr *) (elf + ehdr->e_phoff);

  /*** Get ELF stats ***/
  ELFMinMax(ehdr, phdr);
  sizeinbytes = maxaddress - minaddress;

  /*** Move all ELF segments into ARAM ***/
  for (i = 0; i < ehdr->e_phnum; i++)
  {
    if (phdr[i].p_type == PT_LOAD)
    {
      ARAMPut(elf + phdr[i].p_offset, (char *) ((phdr[i].p_vaddr - minaddress) + ARAMSTART), phdr[i].p_filesz);
    }
  }

  /*** Pass a command line ***/
  if (argc)
  {
    args.argvMagic = ARGV_MAGIC;
    args.argc = argc;
    args.length = 1;

    for (i = 0; i < argc; i++)
    {
      size_t argLength = strlen(argv[i]) + 1;
      args.length += argLength;
    }
    args.commandLine = malloc(args.length);

    unsigned int position = 0;
    for (i = 0; i < argc; i++)
    {
      size_t argLength = strlen(argv[i]) + 1;
      memcpy(args.commandLine + position, argv[i], argLength);
      position += argLength;
    }
    args.commandLine[args.length - 1] = '\0';
    DCStoreRange(args.commandLine, args.length);

    ARAMPut((unsigned char *) &args, (char *) (ehdr->e_entry - minaddress + 8 + ARAMSTART), sizeof(struct __argv));
  }

  /*** Now go run it ***/
  ARAMRun(ehdr->e_entry, minaddress, ARAMSTART, sizeinbytes);

  /*** Will never return ***/
  return 1;
}

int BINtoARAM(unsigned char *bin, int len, unsigned int entrypoint)
{
  /*** Make sure ARAM subsystem is alive! ***/
  AR_Init(NULL, 0); /*** No stack - we need it all ***/
  ARAMClear();

  /*** Move BIN into ARAM ***/
  ARAMPut(bin, (char *) ARAMSTART, len);

  /*** Now go run it ***/
  ARAMRun(entrypoint, entrypoint, ARAMSTART, len);

  /*** Will never return ***/
  return 1;
}
