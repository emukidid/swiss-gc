/*--------------------------------------------------------------------------------
	** Linker script for GameCube homebrew **
--------------------------------------------------------------------------------*/

PROVIDE_HIDDEN( __isIPL = 1 );

SECTIONS {

	/* Default base address */
	/* Use -Wl,--section-start,.init=0xADDRESS to change */
	. = 0x80003100;

}

PROVIDE_HIDDEN( __Arena1Hi = 0x81700000 );

INCLUDE libogc_common.ld
