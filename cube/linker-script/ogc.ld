/*--------------------------------------------------------------------------------
	** Linker script for GameCube homebrew **
--------------------------------------------------------------------------------*/

SECTIONS {

	/* Default base address */
	/* Use -Wl,--section-start,.init=0xADDRESS to change */
	. = 0x80003100;

}

PROVIDE_HIDDEN( __isIPL = 1 );

INCLUDE libogc_common.ld
