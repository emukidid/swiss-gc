#ifndef __WODE_INTERFACE_H__
#define __WODE_INTERFACE_H__

#define MAX_FAVORITES 8


typedef enum {
	pt_unknown,
	pt_ntfs,
	pt_vfat,
	pt_wbfs,
	pt_extfs,
	pt_hfs,
	pt_cifs,
	pt_wode
}partition_type_t;

typedef enum {
	pm_unknown,
	pm_read_only,
	pm_read_write
}partition_mode_t;

typedef struct {
	unsigned char s_region_hack;
	unsigned char s_update_blocker;
	unsigned char s_autoboot;

	unsigned char s_wii_region;
	unsigned char s_autoload;
}s_user_settings;

typedef struct {
	unsigned short loader_version;
	unsigned short wode_version;
	unsigned short fpga_version;
	unsigned char  hw_version;
}device_versions;

typedef struct
{
	char name[64];
	partition_type_t partition_type;
	partition_mode_t partition_mode;
	unsigned long NumISOs;
}PartitionInfo_t;

typedef struct
{
	char name[64];
	unsigned long  iso_type;
	unsigned long  iso_region;
}ISOInfo_t;

/* Favorite States */
#define FAV_EMPTY 		0
#define FAV_NOT_FOUND 	1
#define FAV_OK	  		2

typedef struct
{
//#if GC GUI
	char name[64];
	unsigned long	iso_type;
	unsigned long	iso_region;
//#endif	
	unsigned long state; 		// 0 (empty), 1 (not found), 2 (ok)
	unsigned long partition;
	unsigned long iso; 
}FavoriteInfo_t;




unsigned long OpenWode( void );
void CloseWode( void );

unsigned long GetNumPartitions( void );
unsigned long GetNumISOsInSelectedPartition( int partition );
int GetPartitionInfo(unsigned long partition_idx, PartitionInfo_t* PartitionInfo);
int GetISOInfo(unsigned long partition_idx, unsigned long iso_idx, ISOInfo_t * ISOInfo);

unsigned long GetMaxFavorites( void );

int  GetNumFavorites( void );
int GetFavoriteInfo(unsigned long index, FavoriteInfo_t * favoriteInfo);
int EraseFavorite(unsigned long idx);
int InsertFavorite(unsigned long IsoIndex);


unsigned long SaveSettings( void );
int GetSettings( s_user_settings * settings);
int GetVersions( device_versions * versions );

int GetTotalISOs( void );

unsigned long GetSelectedISO( void );

void SetISO(unsigned long Partition, unsigned long Iso);
unsigned long GotoFlatMode( void );

//unsigned long GetJoystick( void );

#endif
