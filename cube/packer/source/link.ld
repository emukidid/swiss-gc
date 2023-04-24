OUTPUT_FORMAT("elf32-powerpc")
OUTPUT_ARCH(powerpc:common)
ENTRY(start)

start = _start + 0x80000000;

PHDRS
{
	stub PT_LOAD FLAGS(5);
	text PT_LOAD FLAGS(5);
	data PT_LOAD FLAGS(6);
	bss  PT_LOAD FLAGS(6);
}

SECTIONS
{
	. = 0x00003100 + executable_bin_size;
	/DISCARD/ : { executable.bin.o }

	. = ALIGN(32);
	.init : AT(ADDR(.init) + 0x80000000) { KEEP(*crt0.o(*.init)) } : text
	.text : AT(ADDR(.text) + 0x80000000)
	{
		*(.text .text.*)
		. = ALIGN(32);
	}

	. = ALIGN(32);
	.rodata : AT(ADDR(.rodata) + 0x80000000) { *(.rodata .rodata.*) } : data
	.sdata : AT(ADDR(.sdata) + 0x80000000)
	{
		PROVIDE(_SDA_BASE_ = 32768);
		*(.sdata .sdata.*)
		. = ALIGN(32);
	}

	. = ALIGN(32);
	.sbss : AT(ADDR(.sbss) + 0x80000000)
	{
		__sbss_start = .;
		*(.sbss .sbss.*)
		. = ALIGN(32);
		__sbss_end = .;
	} : bss
}

executable_bin_size = executable_bin_end - executable_bin;
executable_xz_size = executable_xz_end - executable_xz;
