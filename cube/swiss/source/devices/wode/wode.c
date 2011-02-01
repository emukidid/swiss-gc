

#include <gccore.h>        // Wrapper to include common libogc headers
#include <ogcsys.h>
#include <stdio.h>
#include <string.h>
#include <ogc\dvd.h>
#include <ogc\system.h>
#include <sys\unistd.h>
#include "wodeinterface.h"


#define WODE_MAGIC				(0xFF574F44)		/* 0xFF,WOD */
#define WODE_EXIT_REMOTE		(0xFF000000)
#define WODE_GET_NUM_PARTS		(0x10000000)
#define WODE_GET_PART(P)		(0x11000000 | ((P & 0xFF) << 16))
#define WODE_GET_NUM_ISOS(P)	(0x12000000 | ((P & 0xFF) << 16))
#define WODE_GET_ISO(P1,P2)		(0x13000000 | ((P1 & 0xFF) << 16) | ((P2 & 0xFF) << 8))

#define WODE_GET_FAVORITE_INFO(P)		(0x14000000 | ((P & 0xFF) << 16))
#define WODE_ERASE_FAVORITE(P)			(0x15000000 | ((P & 0xFF) << 16))
#define WODE_GET_NUM_FAVORITES			(0x16000000)
#define WODE_SET_FAVORITE_FAVE(P)		(0x17000000 |  ((P & 0xFF) 	 << 16))
#define WODE_SET_FAVORITE_PART(P)		(0x18000000 |  ((P & 0xFF)   << 16))
#define WODE_SET_FAVORITE_ISO(P)		(0x19000000 |  ((P & 0xFFFF) << 8))

#define WODE_SET_REGION_HACK(P)			(0x20000000 | ((P & 0xFF) << 16))
#define WODE_SET_UPDATE_BLOCK_HACK(P)	(0x21000000 | ((P & 0xFF) << 16))
#define WODE_SET_AUTOBOOT_HACK(P)		(0x22000000 | ((P & 0xFF) << 16))
#define WODE_SET_RELOAD_HACK(P)			(0x23000000 | ((P & 0xFF) << 16))
#define WODE_SET_WII_REGION(P)			(0x24000000 | ((P & 0xFF) << 16))

#define WODE_WRITE_SETTINGS				(0x25000000)


#define WODE_GET_SETTINGS		(0x30000000)
#define WODE_GET_VERSIONS		(0x31000000)

//#define WODE_GET_JSTICK			(0x40000000)


#define WODE_GOTO_FLAT_MODE				(0x80000000)
#define WODE_LAUNCH_GAME_PART(P)		(0x81000000 |  ((P & 0xFF) 	 << 16))
#define WODE_LAUNCH_GAME_ISO(P)			(0x82000000 |  ((P & 0xFFFF) <<  8))



static u8 dvdbuffer[0x8000] ATTRIBUTE_ALIGN (32);    // One Sector



//--------------------------------------------------------------------------
int InitDVD() 
{
	DVD_Init ();		
	return DVD_Mount();
}

unsigned long OpenWode( void )
{
	dvdcmdblk cmdblk;

	if(InitDVD() < 0)
		return 0;

	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x20, WODE_MAGIC, 2) <= 0){
    return 0;
	}

	if(	dvdbuffer[0] == 'W' &&
		dvdbuffer[1] == 'O' &&
		dvdbuffer[2] == 'D' &&
		dvdbuffer[3] == 'E')
		return 1;
	return 0;
}

void CloseWode( void )
{
	dvdcmdblk cmdblk;
	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x20, WODE_EXIT_REMOTE, 2) <= 0){
	}
}
/*
unsigned long GetJoystick( void )
{
	dvdcmdblk cmdblk;
	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x20 , WODE_GET_JSTICK, 2) <= 0){
		printf ("Error -> GetJoystick\n");
		ExitToWii();
	}
	return *((unsigned long*)dvdbuffer); 
}
*/

unsigned long GetNumPartitions( void )
{
	dvdcmdblk cmdblk;
	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x20 , WODE_GET_NUM_PARTS, 2) <= 0){
	}
	return *(unsigned long*)&dvdbuffer[0]; 
}

unsigned long GetNumISOsInSelectedPartition( int partition )
{
	dvdcmdblk cmdblk;
	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x20 , WODE_GET_NUM_ISOS(partition), 2) <= 0){
	}
	return *(unsigned long*)&dvdbuffer[0]; 
}

int GetPartitionInfo(unsigned long partition_idx, PartitionInfo_t* PartitionInfo)
{
	dvdcmdblk cmdblk;
	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x80 , WODE_GET_PART(partition_idx), 2) <= 0){
	}
	memcpy(PartitionInfo->name,dvdbuffer,64);
	PartitionInfo->NumISOs 			= *((unsigned long*)&dvdbuffer[64]);
	PartitionInfo->partition_type 	= *((unsigned long*)&dvdbuffer[68]);
	PartitionInfo->partition_mode 	= *((unsigned long*)&dvdbuffer[72]);
	return 1;	
}

int GetISOInfo(unsigned long partition_idx, unsigned long iso_idx, ISOInfo_t * ISOInfo)
{
	dvdcmdblk cmdblk;
	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x80 , WODE_GET_ISO(partition_idx,iso_idx), 2) <= 0){
	}
	memcpy(ISOInfo->name,dvdbuffer,64);
	ISOInfo->iso_type 		= *((unsigned long*)&dvdbuffer[64]);
	ISOInfo->iso_region 	= *((unsigned long*)&dvdbuffer[68]);
	return 1;	
}

unsigned long GotoFlatMode( void )
{
	dvdcmdblk cmdblk;
	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x20, WODE_GOTO_FLAT_MODE, 2) <= 0){
	}
	CloseWode();
	return 0;
}
/*
unsigned long GetMaxFavorites( void )
{
	return MAX_FAVORITES;
}

int GetFavoriteInfo(unsigned long index, FavoriteInfo_t * favoriteInfo)
{
	dvdcmdblk cmdblk;
	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x80 , WODE_GET_FAVORITE_INFO(index), 2) <= 0){
		ExitToWii();
	}

	favoriteInfo->state     = *((unsigned long*)&dvdbuffer[0]);
	favoriteInfo->partition = *((unsigned long*)&dvdbuffer[4]);
	favoriteInfo->iso       = *((unsigned long*)&dvdbuffer[8]);

//#if GC GUI
	memcpy(favoriteInfo->name,&dvdbuffer[12],64);
	favoriteInfo->iso_type 	 = *((unsigned long*)&dvdbuffer[76]);
	favoriteInfo->iso_region = *((unsigned long*)&dvdbuffer[80]);
//#endif	

	return 1;
}

int EraseFavorite(unsigned long idx)
{
	dvdcmdblk cmdblk;

	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x20 , WODE_ERASE_FAVORITE(idx), 2) <= 0){
		printf ("Error -> EraseFavorite\n");
		ExitToWii();
	}
	return 1;
}

int  GetNumFavorites( void )
{
	dvdcmdblk cmdblk;
	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x20 , WODE_GET_NUM_FAVORITES, 2) <= 0){
		printf ("Error -> GetNumFavorites\n");
		ExitToWii();
	}
	return *((unsigned long*)dvdbuffer);	
}

int InsertFavorite(unsigned long IsoIndex)
{
	dvdcmdblk cmdblk;
	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x20 , WODE_SET_FAVORITE_FAVE(GetSelectedFavorite()), 2) <= 0){
		printf ("Error -> WODE_SET_FAVORITE_FAVE\n");
		ExitToWii();
	}

	// 256 partitions should be enough
	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x20 , WODE_SET_FAVORITE_PART(GetSelectedPartition()),	2) <= 0){
		printf ("Error -> WODE_SET_FAVORITE_PART\n");
		ExitToWii();
	}

	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x20 , WODE_SET_FAVORITE_ISO(IsoIndex),	2) <= 0){
		printf ("Error -> WODE_SET_FAVORITE_PART\n");
		ExitToWii();
	}

	return 1;	
}



extern MenuElement_t SettingsMenuElements[];

unsigned long SaveSettings( void )
{
	dvdcmdblk cmdblk;

	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x20 , WODE_SET_REGION_HACK(MENU_GetRegion()), 2) <= 0){	//20
		printf ("Error -> SetRegion\n");
		ExitToWii();
	}

	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x20 , WODE_SET_UPDATE_BLOCK_HACK(MENU_GetBlockUpdates()), 2) <= 0){  //21
		printf ("Error -> SetBlockUpdates\n");
		ExitToWii();
	}

	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x20 , WODE_SET_AUTOBOOT_HACK(MENU_GetAutoStart()), 2) <= 0){	//22
		printf ("Error -> SetAutoStart\n");
		ExitToWii();
	}

	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x20 , WODE_SET_RELOAD_HACK(MENU_GetAutoload()), 2) <= 0){	//23
		printf ("Error -> SetWiiRegion\n");
		ExitToWii();
	}

	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x20 , WODE_SET_WII_REGION(MENU_GetWiiRegion()), 2) <= 0){		//24
		printf ("Error -> SetAutoload\n");
		ExitToWii();
	}

	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x20 , WODE_WRITE_SETTINGS, 2) <= 0){
		printf ("Error -> Wite Settings\n");
		ExitToWii();
	}

	if(	(MENU_GetRegion() 		== dvdbuffer[0]) &&
		(MENU_GetBlockUpdates() == dvdbuffer[1]) &&
		(MENU_GetAutoStart() 	== dvdbuffer[2]) &&
		(MENU_GetWiiRegion() 	== dvdbuffer[3]) &&
		(MENU_GetAutoload() 	== dvdbuffer[4]))
		{
			DrawBitmap(160, 150, 7);	
			sleep(2);
			
		}
	else
		{
			DrawBitmap(160, 150, 8);	
			sleep(2);
		}


	return 1;
}

int GetSettings( s_user_settings * settings )
{
	dvdcmdblk cmdblk;
	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x20 , WODE_GET_SETTINGS, 2) <= 0){
		ExitToWii();
		return 0;
	}

	settings->s_region_hack		= dvdbuffer[0];
	settings->s_update_blocker	= dvdbuffer[1];
	settings->s_autoboot		= dvdbuffer[2];
	settings->s_wii_region		= dvdbuffer[3];
	settings->s_autoload		= dvdbuffer[4];

	return 1;
}
*/
int GetVersions( device_versions * versions )
{
	dvdcmdblk cmdblk;
	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x20 , WODE_GET_VERSIONS, 2) <= 0){
		return 0;
	}

	versions->loader_version   = dvdbuffer[0];
	versions->loader_version <<= 8;
	versions->loader_version  |= dvdbuffer[1];

	versions->wode_version   = dvdbuffer[2];
	versions->wode_version <<= 8;
	versions->wode_version  |= dvdbuffer[3];

	versions->fpga_version   = dvdbuffer[4];
	versions->fpga_version <<= 8;
	versions->fpga_version  |= dvdbuffer[5];

	versions->hw_version	 = dvdbuffer[6];

	return 1;
}

void SetISO(unsigned long Partition, unsigned long Iso)
{
	dvdcmdblk cmdblk;

	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x20, WODE_LAUNCH_GAME_PART(Partition), 2) <= 0){
	}
	if(DVD_ReadPrio (&cmdblk, dvdbuffer, 0x20, WODE_LAUNCH_GAME_ISO(Iso), 2) <= 0){
	}
	CloseWode();
}

unsigned long GetSelectedISO( void )
{
	return 0;
}

int GetTotalISOs( void )
{
	PartitionInfo_t PartitionInfo;
	unsigned long i;	
	unsigned long NumPartitions = GetNumPartitions( );
	int TotalISOs = 0;
	
	for(i=0;i<NumPartitions;i++)
	{
		GetPartitionInfo(i, &PartitionInfo);
		TotalISOs += PartitionInfo.NumISOs;
	}
	return TotalISOs;
}

