OUTPUT_FORMAT("elf32-powerpc")
OUTPUT_ARCH(powerpc:common)
ENTRY(_start)

SECTIONS
{
	. = 0x00001800;
}
