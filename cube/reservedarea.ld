VAR_AREA			= 0x80000000;

VAR_JUMP_TABLE		= VAR_AREA | 0x00A0;	/* Dolphin OS jump table */
VAR_TVMODE			= VAR_AREA | 0x00CC;	/* TV format */

VAR_CLIENT_MAC		= VAR_AREA | 0x09EA;	/* client MAC address */
VAR_CLIENT_IP		= VAR_AREA | 0x09F0;	/* client IPv4 address */
VAR_SERVER_MAC		= VAR_AREA | 0x09F4;	/* server MAC address */
VAR_SERVER_IP		= VAR_AREA | 0x09FA;	/* server IPv4 address */
VAR_SERVER_PORT		= VAR_AREA | 0x09FE;	/* server UDP port */

VAR_SECTOR_CUR		= VAR_AREA | 0x09FC;	/* is the currently buffered disk sector */
VAR_SECTOR_BUF		= VAR_AREA | 0x0A00;	/* 0x200 of read data */

VAR_DISC_1_FNLEN	= VAR_AREA | 0x0A00;	/* disc 1 filename length */
VAR_DISC_1_FN		= VAR_AREA | 0x0A01;	/* disc 1 filename */
VAR_DISC_2_FNLEN	= VAR_AREA | 0x0B00;	/* disc 2 filename length */
VAR_DISC_2_FN		= VAR_AREA | 0x0B01;	/* disc 2 filename */

VAR_DRIVE_RESETTING	= VAR_AREA | 0x09FE;	/* disc drive resetting */
VAR_DRIVE_PATCHED	= VAR_AREA | 0x09FF;	/* disc drive patched */
VAR_DISC_1_ID		= VAR_AREA | 0x0A00;	/* disc 1 header */
VAR_DISC_2_ID		= VAR_AREA | 0x0B00;	/* disc 2 header */

VAR_PATCHES_BASE	= VAR_AREA | 0x2D00;	/* Patches get copied to below this area. */

VAR_FRAG_LIST		= VAR_AREA | 0x2D00;	/* 0x1C8 of fragments (40 frags max) (u32 offset, u32 size, u32 rawsector) */
VAR_TIMER_START		= VAR_AREA | 0x2EC8;	/* Use this as a timer start (tbu,tb) */
VAR_DISC_CHANGING	= VAR_AREA | 0x2ED0;	/* Are we changing discs? */
VAR_NEXT_FIELD		= VAR_AREA | 0x2ED4;	/* next video field */
VAR_IGR_EXIT_TYPE	= VAR_AREA | 0x2ED5;	/* IGR exit type */
VAR_IPV4_ID			= VAR_AREA | 0x2ED6;	/* IPv4 fragment identifier */
VAR_FSP_KEY			= VAR_AREA | 0x2ED8;	/* FSP session key */
VAR_FSP_DATA_LENGTH	= VAR_AREA | 0x2EDA;	/* FSP payload size */
VAR_EMU_READ_SPEED	= VAR_AREA | 0x2EDC;	/* emulate read speed */
VAR_IGR_DOL_SIZE	= VAR_AREA | 0x2EE0;	/* IGR DOL Size */
VAR_READS_IN_AS		= VAR_AREA | 0x2EE4;	/* How many times have we tried to read while streaming is on? */
VAR_AS_ENABLED		= VAR_AREA | 0x2EE8;	/* Is Audio streaming enabled by the user? */
VAR_RMODE			= VAR_AREA | 0x2EEC;	/* render mode */
VAR_VFILTER			= VAR_AREA | 0x2EF0;	/* vertical filter */
VAR_VFILTER_ON		= VAR_AREA | 0x2EF7;	/* vertical filter on */
VAR_FAKE_IRQ_SET	= VAR_AREA | 0x2EF8;	/* flag to say we are ready to fake irq. */
VAR_SAR_WIDTH		= VAR_AREA | 0x2EFC;	/* sample aspect ratio width */
VAR_SAR_HEIGHT		= VAR_AREA | 0x2EFE;	/* sample aspect ratio height */
VAR_CURRENT_FIELD	= VAR_AREA | 0x2EFF;	/* current video field */
VAR_CURRENT_DISC	= VAR_AREA | 0x2F00;	/* current disc number */
VAR_SECOND_DISC		= VAR_AREA | 0x2F04;	/* second disc present */
VAR_SD_LBA			= VAR_AREA | 0x2F0C;	/* is the SD Card sector being read */
VAR_LAST_OFFSET		= VAR_AREA | 0x2F10;	/* the last offset a read was simulated from */
VAR_EXECD_OFFSET	= VAR_AREA | 0x2F14;	/* offset of execD.bin on multi-dol discs */
VAR_SD_SHIFT		= VAR_AREA | 0x2F18;	/* is the SD Card shift amount when issueing read cmds */
VAR_EXI_FREQ		= VAR_AREA | 0x2F19;	/* is the EXI frequency (4 = 16mhz, 5 = 32mhz) */
VAR_EXI_SLOT		= VAR_AREA | 0x2F1A;	/* is the EXI slot (0 = slot a, 1 = slot b) */
VAR_ATA_LBA48		= VAR_AREA | 0x2F1B;	/* Is the HDD in use a 48 bit LBA supported HDD? */
VAR_TMP1			= VAR_AREA | 0x2F1C;	/* space for a variable if required */
VAR_TMP2			= VAR_AREA | 0x2F20;	/* space for a variable if required */
VAR_FLOAT9_16		= VAR_AREA | 0x2F24;	/* constant 9/16 */
VAR_FLOAT1_6		= VAR_AREA | 0x2F28;	/* constant 1/6 */
VAR_FLOAT3_4		= VAR_AREA | 0x2F2C;	/* constant 3/4 */
VAR_FLOATM_1		= VAR_AREA | 0x2F30;	/* constant -1 */
VAR_DI_REGS			= VAR_AREA | 0x2F34;	/* DI Regs are mapped to here... */
VAR_INTERRUPT_TIMES	= VAR_AREA | 0x2F58;	/* how many times have we called the dvd queue */
VAR_DEVICE_SPEED	= VAR_AREA | 0x2F5C;	/* How long in usec does it take to read 1024 bytes */
VAR_STREAM_START	= VAR_AREA | 0x2F60;	/* AS Start */
VAR_STREAM_CUR		= VAR_AREA | 0x2F64;	/* AS Current Location */
VAR_STREAM_END		= VAR_AREA | 0x2F68;	/* AS End */
VAR_STREAM_SIZE		= VAR_AREA | 0x2F6C;	/* AS Size */
VAR_STREAM_UPDATE	= VAR_AREA | 0x2F70;	/* AS Update Request */
VAR_STREAM_ENDING	= VAR_AREA | 0x2F71;	/* AS Ending */
VAR_STREAM_LOOPING	= VAR_AREA | 0x2F72;	/* AS Looping */
VAR_AS_SAMPLECNT	= VAR_AREA | 0x2F73;	/* AS Sample Counter */
VAR_STREAM_CURBUF	= VAR_AREA | 0x2F74;	/* AS Current Main Buffer */
VAR_STREAM_DI		= VAR_AREA | 0x2F75;	/* AS DI Status */
VAR_STREAM_BUFLOC	= VAR_AREA | 0x2F78;
VAR_AS_HIST_0		= VAR_AREA | 0x2F7C;
VAR_AS_HIST_1		= VAR_AREA | 0x2F80;
VAR_AS_HIST_2		= VAR_AREA | 0x2F84;
VAR_AS_HIST_3		= VAR_AREA | 0x2F88;
VAR_AS_TMP_LSAMPLE	= VAR_AREA | 0x2F8C;
VAR_AS_TMP_RSAMPLE	= VAR_AREA | 0x2F8E;
VAR_AS_OUTL			= VAR_AREA | 0x2F90;
VAR_AS_OUTR			= VAR_AREA | 0x2FC8;
