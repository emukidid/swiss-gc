OUTPUT_FORMAT("elf32-powerpc")
OUTPUT_ARCH(powerpc:common)
ENTRY(_start)

SECTIONS
{
	. = 0x00001800;
}

EXI = 0x0C006800;
