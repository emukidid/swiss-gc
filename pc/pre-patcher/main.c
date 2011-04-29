#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "elf_abi.h"

// Global vars
// ELF/DOL files
static unsigned char ELFfileNameArray[1024][512];
static unsigned int  ELFfileOffsetArray[1024];
static unsigned int  ELFfileSizeArray[1024];
static unsigned int  ELFfileIsDOL[1024];
static unsigned int  ReloaderOffsetArray[1024];
static unsigned int  ReloaderSizeArray[1024];
static int ELFnumFiles = 0;
static int ReloaderNumFiles = 0;

//TGC (Embedded GCM files)
static unsigned char TGCfileNameArray[1024][512];
static unsigned int  TGCfileOffsetArray[1024];
static unsigned int  TGCfileSizeArray[1024];
static unsigned int  TGCapploaderOffsetsArray[1024];
static int TGCnumFiles = 0;

int jitterfix = 0;
int gamefixA  = 0;

static const unsigned int  sig_fwrite[8] = {
  0xD0FF2194,
  0xA602087C,
  0x34000190,
  0x140021BF,
  0x7823997C,
  0x7833DA7C,
  0x781B7B7C,
  0x782BBC7C 
};

static const unsigned int  sig_fwrite_alt[8] = {
  0xA602087C,
  0x04000190,
  0xB8FF2194,
  0x2C0021BF,
  0x0000443B,
  0x0000663B,
  0x0000833B,
  0x0000253B 
};

static const unsigned int  sig_fwrite_alt2[8] = {
  0xA602087C,
  0x04000190,
  0xD0FF2194,
  0x180041BF,
  0x0000C33B,
  0x0000E43B,
  0x30B4AD80,
  0x015A053C 
};

static const unsigned int geckoPatch[31] = {
  0xD721857C,
  0x70008140,
  0x00CCE03C,
  0x0000C038,
  0xAE18067C,
  0x16A00054,
  0x00B00864,
  0xD0000038,
  0x14680790,
  0xAC06007C,
  0x24680791,
  0xAC06007C,
  0x19000038,
  0x20680790,
  0xAC06007C,
  0x20680780,
  0xAC04007C,
  0x01000970,
  0xF4FF8240,
  0x24680780,
  0xAC04007C,
  0x00002039,
  0x14682791,
  0xAC06007C,
  0x00040974,
  0xB8FF8241,
  0x0100C638,
  0x0020867F,
  0xA0FF9E40,
  0x782BA37C,
  0x2000804E
};

static unsigned char _Read_original[46] = {
	0x7C, 0x08, 0x02, 0xA6,  //	mflr        r0
	0x90, 0x01, 0x00, 0x04,  //	stw         r0, 4 (sp)
	0x38, 0x00, 0x00, 0x00,  //	li          r0, 0
	0x94, 0x21, 0xFF, 0xD8,  //	stwu        sp, -0x0028 (sp)
	0x93, 0xE1, 0x00, 0x24,  //	stw         r31, 36 (sp)
	0x93, 0xC1, 0x00, 0x20,  //	stw         r30, 32 (sp)
	0x3B, 0xC5, 0x00, 0x00,  //	addi        r30, r5, 0
	0x93, 0xA1, 0x00, 0x1C,  //	stw         r29, 28 (sp)
	0x3B, 0xA4, 0x00, 0x00,  //	addi        r29, r4, 0
	0x93, 0x81, 0x00, 0x18,  //	stw         r28, 24 (sp)
	0x3B, 0x83, 0x00, 0x00,  //	addi        r28, r3, 0
	0x90, 0x0D               // stw r0 to someplace in (sd1) - differs on different sdk's
};

static unsigned char _Read_original_2[38] = {
	0x7C, 0x08, 0x02, 0xA6,  	//	mflr        r0               
	0x90, 0x01, 0x00, 0x04,  	//	stw         r0, 4 (sp)       
	0x94, 0x21, 0xFF, 0xE0,  	//	stwu        sp, -0x0020 (sp) 
	0x93, 0xE1, 0x00, 0x1C,  	//	stw         r31, 28 (sp)     
	0x90, 0x61, 0x00, 0x08,  	//	stw         r3, 8 (sp)       
	0x7C, 0x9F, 0x23, 0x78,  	//	mr          r31, r4          
	0x90, 0xA1, 0x00, 0x10,  	//	stw         r5, 16 (sp)      
	0x90, 0xC1, 0x00, 0x14,  	//	stw         r6, 20 (sp)      
	0x80, 0x01, 0x00, 0x14,  	//	lwz         r0, 20 (sp)      
	0x90, 0x0D					//	stw r0 to someplace in (sd1) - differs on different sdk's
};

/* Luigis Mansion NTSC (not Players Choice) */
static unsigned char _Read_original_3[46] = {
	0x7C, 0x08, 0x02, 0xA6,  // mflr        r0              
	0x90, 0x01, 0x00, 0x04,  // stw         r0, 4 (sp)      
	0x38, 0x00, 0x00, 0x01,  // li          r0, 1           
	0x94, 0x21, 0xFF, 0xD8,  // stwu        sp, -0x0028 (sp)
	0x93, 0xE1, 0x00, 0x24,  // stw         r31, 36 (sp)    
	0x93, 0xC1, 0x00, 0x20,  // stw         r30, 32 (sp)    
	0x3B, 0xC5, 0x00, 0x00,  // addi        r30, r5, 0      
	0x93, 0xA1, 0x00, 0x1C,  // stw         r29, 28 (sp)    
	0x3B, 0xA4, 0x00, 0x00,  // addi        r29, r4, 0      
	0x93, 0x81, 0x00, 0x18,  // stw         r28, 24 (sp)    
	0x3B, 0x83, 0x00, 0x00,  // addi        r28, r3, 0      
	0x90, 0xCD               // stw r6 to someplace in (sd1) - differs on different sdk's
};

static const unsigned int _DVDLowReadDiskID_original[8] = {
  0xA602087C,  // mflr        r0
  0x00000039,  // li          r8, 0
  0x04000190,  // stw         r0, 4 (sp)
  0x00A8A03C,  // lis         r5, 0xA800
  0x40000538,  // addi        r0, r5, 64
  0xE8FF2194,  // stwu        sp, -0x0018 (sp)
  0x2000C038,  // li          r6, 32
  0x0080A03C   // lis         r5, 0x8000
};

//aka AISetStreamPlayState
static const unsigned int _AIResetStreamSampleCount_original[9] = {
  0x00CC603C,  // lis         r3, 0xCC00
  0x006C0380,  // lwz         r0, 0x6C00 (r3)
  0x3C000054,  // rlwinm      r0, r0, 0, 0, 30
  0x78EB007C,  // or          r0, r0, r29
  0x006C0390,  // stw         r0, 0x6C00 (r3)
  0x24000180,  // lwz         r0, 36 (sp)
  0x1C00E183,  // lwz         r31, 28 (sp)
  0x1800C183,  // lwz         r30, 24 (sp)
  0xA603087C   // mtlr        r0
};

void parse_gcm(FILE *file);
void parse_tgc(FILE *file,unsigned int tgc_base);

unsigned int convert_int(unsigned int in)
{
	unsigned int out;
	char *p_in = (char *) &in;
	char *p_out = (char *) &out;
	p_out[0] = p_in[3];
	p_out[1] = p_in[2];
	p_out[2] = p_in[1];
	p_out[3] = p_in[0];  
	return out;
}

void writeBranch(unsigned int actual, unsigned int sourceAddr,unsigned int destAddr) {
	*(unsigned int*)actual = convert_int(((((destAddr)-(sourceAddr)) & 0x03FFFFFF) | 0x4B000001));	//write it to the dest
}

int patchRead(char* buffer,int size,unsigned int dst)
{
	int count = 0;
	int patched = 0;
	unsigned int newbranch = 0;
	while(count<size) {
		if(memcmp((unsigned char*)(buffer+count),_Read_original,sizeof(_Read_original))==0) {
  			if(dst+count & 0x80000000)
  			{
  				printf("Patching Read %08X\n",dst+count);
  				//writeBranch((unsigned int)(buffer + count + 0x04), dst + count + 0x04, 0x80001800);  			
  				*(unsigned int*)(buffer + count + 8) = convert_int(0x3C008000); // lis		0, 0x8000   
  				*(unsigned int*)(buffer + count + 12) = convert_int(0x60001800); // ori		0, 0, 0x1800
  				*(unsigned int*)(buffer + count + 16) = convert_int(0x7C0903A6); // mtctr	0          
  				*(unsigned int*)(buffer + count + 20) = convert_int(0x4E800421); // bctrl  
  				*(unsigned int*)(buffer + count + 92) = convert_int(0x3C60AB00); // lis         r3, 0xAB00 (make it a seek)
  				*(unsigned int*)(buffer + count + 112) = convert_int(0x38000001);//  li          r0, 1 (IMM not DMA)
  				patched = 1;
			}
		}
		if(memcmp(buffer+count,_Read_original_2,sizeof(_Read_original_2))==0) {
  			if(dst+count & 0x80000000)
  			{
  				printf("Patching Read_v2 %08X\n",dst+count);
  				*(unsigned int*)(buffer + count + 8) = convert_int(0x3C008000); // lis		0, 0x8000   
  				*(unsigned int*)(buffer + count + 12) = convert_int(0x60001810); // ori		0, 0, 0x1810
  				*(unsigned int*)(buffer + count + 16) = convert_int(0x7C0903A6); // mtctr	0          
  				*(unsigned int*)(buffer + count + 20) = convert_int(0x4E800421); // bctrl   
  				*(unsigned int*)(buffer + count + 68) = convert_int(0x3C00AB00);	//  lis         r0, 0xAB00 (make it a seek)
  				*(unsigned int*)(buffer + count + 128) = convert_int(0x38000001);//  li          r0, 1 (IMM not DMA)         
  				patched = 1;
			}
		}
		if(memcmp(buffer+count,_Read_original_3,sizeof(_Read_original_3))==0) {
  			if(dst+count & 0x80000000)
  			{
  				printf("Patching Read_v3 %08X\n",dst+count);
  				*(unsigned int*)(buffer + count + 8) = convert_int(0x3C008000); // lis		0, 0x8000   
  				*(unsigned int*)(buffer + count + 12) = convert_int(0x60001814); // ori		0, 0, 0x1814
  				*(unsigned int*)(buffer + count + 16) = convert_int(0x7C0903A6); // mtctr	0          
  				*(unsigned int*)(buffer + count + 20) = convert_int(0x4E800421); // bctrl            
  				*(unsigned int*)(buffer + count + 84) = convert_int(0x3C60AB00); // lis         r3, 0xAB00 (make it a seek)
  				*(unsigned int*)(buffer + count + 104) = convert_int(0x38000001);//  li          r0, 1 (IMM not DMA)
  				patched = 1;
			}
		}
		count+=4;		
	}
	return patched;
}

int patchDVDLowReadDiskID(char* buffer,int size,unsigned int dst)
{
	int count = 0;
	int patched = 0;
	unsigned int newbranch = 0;
	while(count<size) {
		if(memcmp(buffer+count,_DVDLowReadDiskID_original,sizeof(_DVDLowReadDiskID_original))==0) {
  		if(dst+count & 0x80000000)
  		{
  			printf("Patching DVDLowReadDiskID %08X\n",dst+count);
  			writeBranch((unsigned int)(buffer + count + 0x08), dst + count + 0x08, 0x80001800);  			
  			patched = 1;
			}
		}
		count+=4;
	}
	return patched;
}

int patchAISampleCount(char* buffer,int size,unsigned int dst)
{
	int count = 0;
	int patched = 0;
	unsigned int newbranch = 0;
	while(count<size) {
		if(memcmp(buffer+count,_AIResetStreamSampleCount_original,sizeof(_AIResetStreamSampleCount_original))==0) {
  		if(dst+count & 0x80000000)
  		{
  			printf("Patching AIResetStreamSampleCount\n%08X ",dst+count);
  			*(unsigned long*)(buffer+count + 0x0C) = convert_int(0x60000000); //NOP
  			patched = 1;
			}
		}
		count+=4;
	}
	return patched;
}

int patchDebug(char* buffer,int size,unsigned int dst)
{
	int count = 0;
	int patched = 0;
	unsigned int newbranch = 0;
	while(count<size) {
		if	((memcmp(buffer+count,sig_fwrite,sizeof(sig_fwrite))==0) 	||
			(memcmp(buffer+count,sig_fwrite_alt,sizeof(sig_fwrite_alt))==0) ||
			(memcmp(buffer+count,sig_fwrite_alt2,sizeof(sig_fwrite_alt2))==0)) {
				printf("Patching in Debug Printing\n%08X ",dst+count);
				memcpy(buffer+count, geckoPatch, sizeof(geckoPatch));
				patched = 1;
        }
		count+=4;
	}
	return patched;
}

int do_patches(char* buffer,int size,unsigned int dst) {
  int resp = 0;
  
  resp += patchRead(buffer, size, dst);
  resp += patchDVDLowReadDiskID(buffer, size, dst);
  resp += patchAISampleCount(buffer, size, dst);
  //resp += patchDebug(buffer, size, dst);
  
  return resp;
}

int patch_ELF(FILE *file, unsigned int size, unsigned int offset)
{
	char *elf = malloc(size);
	int modified = 0;
	
	if(!elf)
	{
		printf("Can't malloc enough space for ELF.\n");
		fclose(file);
		exit(0);
	}
	
	fseek(file,offset,SEEK_SET); 
	fread(elf, 1, size, file);
	
	Elf32_Ehdr* ehdr;
	Elf32_Shdr* shdr;
	unsigned char* strtab = 0;
	unsigned char* image;
	int i;

	ehdr = (Elf32_Ehdr *)elf;
	unsigned short numSections = ehdr->e_shnum>>8;
//	printf("Section header table offset: %08X\n",convert_int(ehdr->e_shoff));
	
	/* Load each appropriate section */
	for (i = 0; i < numSections; ++i)
	{
		shdr = (Elf32_Shdr *)(elf + convert_int(ehdr->e_shoff) + (i * sizeof(Elf32_Shdr)));

		if (!(convert_int(shdr->sh_flags) & SHF_ALLOC) || shdr->sh_addr == 0 || shdr->sh_size == 0)
		{
			continue;
		}
		unsigned int curDest = convert_int(shdr->sh_addr);
		unsigned int curSize = convert_int(shdr->sh_size);
		unsigned int curPosInELF = convert_int(shdr->sh_offset);
		if (curPosInELF+curSize > size)
			break;
		//printf("Section to go at dst: %08X size: %08X\n",curDest,curSize);
		if(do_patches((char*)(elf + curPosInELF),curSize,curDest))
			modified = 1;

	}
	if(modified)
	{
		printf("Writing patch\n");
		fseek(file,offset,SEEK_SET); 
		fwrite(elf, 1, size, file);
	}
	else
		printf("Doesn't require patching or couldn't patch!!\n");
	free(elf);
	return modified;
}

int patch_loader(FILE *file, unsigned int offset, int is_reloader, int size)
{
  if(size<0)
  	size = 2*1024*1024;
  if(!is_reloader)
  	offset+=0x20; // skip the header
  char *appldr = malloc(size);
	int modified = 0;
	
	if(!appldr)
	{
		printf("Can't malloc enough space for %s.\n", is_reloader ? "reloader":"apploader");
		fclose(file);
		exit(0);
	}
	
	fseek(file,offset,SEEK_SET); 
	fread(appldr, 1, size, file);
	
	if(do_patches((char*)(appldr),size,is_reloader ? 0x81300000 : 0x81200000))
			modified = 1;
	if(modified)
	{
		printf("Writing %s patch\n", is_reloader ? "reloader":"apploader");
		fseek(file,offset,SEEK_SET); 
		fwrite(appldr, 1, size, file);
	}
	else
		printf("%s doesn't require patching or couldn't patch!!\n",is_reloader ? "reloader":"apploader");
	free(appldr);
	return modified;
}
  
int patch_DOL(FILE *file, unsigned int size, unsigned int offset)
{
	char *dol = malloc(size);
	int modified = 0;
	
	if(!dol)
	{
		printf("Can't malloc enough space for DOL.\n");
		fclose(file);
		exit(0);
	}
	
	fseek(file,offset,SEEK_SET); 
	fread(dol, 1, size, file);

	struct dol_s
	{
		unsigned long sec_pos[18];
		unsigned long sec_address[18];
		unsigned long sec_size[18];
		unsigned long bss_address, bss_size, entry_point;
	} *d = (struct dol_s *)dol;
	int i;

	for (i = 0; i < 18; ++i)
	{
		if (!d->sec_size[i])
			continue;
		unsigned int curDest = convert_int(d->sec_address[i]);
		unsigned int curSize = convert_int(d->sec_size[i]);
		unsigned int curPosInDOL = convert_int(d->sec_pos[i]);
	//	printf("Section to go at dst: %08X size: %08X\n",curDest,curSize);
		if(do_patches((char*)(dol + curPosInDOL),curSize,curDest))
			modified = 1;
	}
	if(modified)
	{
		printf("Writing patch\n");
		fseek(file,offset,SEEK_SET); 
		fwrite(dol, 1, size, file);
	}
	else
		printf("Doesn't require patching or couldn't patch!!\n");

	free(dol);
	return modified;
}

void show_usage(const char *strng) {
  printf("\tUsage example:\n\t\t\t%s file.gcm\n",strng);
	exit(0);
}

int main(int argc, char **argv) {
  FILE *in;
	int i = 0;
    int patched = 0;
	printf("\nSwiss - Pre-Patcher version 0.1\n");
	
  if(argc!=2)
    show_usage(argv[0]);	  
  
  in = fopen(argv[1],"r+b");
  if(!in)
  {
      printf("Error opening %s\n",argv[1]);
      exit(1);
  }
    
	printf("Scanning file for DVDRead functions .. \n\n");
  parse_gcm(in);
  
  //add in the main DOL executable to our list
  printf("Patching Main DOL\n");
  unsigned int offset = 0;
  fseek(in,0x0420,SEEK_SET);
  fread(&offset, 1, 4, in);
  offset=convert_int(offset);    
  patched = patch_DOL(in,10*1024*1024,offset);	//assume 10mb is the largest DOL
  printf("\nPatching Apploader\n");
  patched |= patch_loader(in, 0x2440, 0, -1);

  // collect all .dol/.elf from embedded GCM files
  if(TGCnumFiles>0) {
	for(i = 0; i < TGCnumFiles; i++) {
	  parse_tgc(in,TGCfileOffsetArray[i]);
	}
  }

  if(ELFnumFiles>0)
  {
	  printf("\nFound %i additional file(s) requiring patching\n",ELFnumFiles);
	  for(i = 0; i < ELFnumFiles; i++)
	  {
	    printf("Patching %s Size: %08X Offset: %08X..\n",ELFfileNameArray[i],ELFfileSizeArray[i],ELFfileOffsetArray[i]);
	    if(ELFfileIsDOL[i])
	    	patched |= patch_DOL(in,ELFfileSizeArray[i],ELFfileOffsetArray[i]);
	    else
	    	patched |= patch_ELF(in,ELFfileSizeArray[i],ELFfileOffsetArray[i]);
	    printf("\n");
	  }
	}
	
	for(i = 0; i < TGCnumFiles; i++) {
	  printf("\nPatching TGC Apploader\n");
	  patched |= patch_loader(in, TGCapploaderOffsetsArray[i], 0, -1);
    }	
    
    for(i = 0; i < ReloaderNumFiles; i++) {
	    printf("\nPatching execD.img reloader\n");
	    patched |= patch_loader(in, ReloaderOffsetArray[i], 1, ReloaderSizeArray[i]);
    }
	
	if(patched) {
		unsigned int magic_words[3] = {0x50617463,0x68656421,0x00000002};	//'Patched!' followed by patcher version number
		for(i = 0; i < 3; i++)
    		magic_words[i] = convert_int(magic_words[i]);
		fseek(in,0x100,SEEK_SET); 
		fwrite(&magic_words[0], 1, 12, in);
	}
  fclose(in);

    
  return 0;
}

void parse_gcm(FILE *file) {
 
	unsigned char   buffer[256]; 
	unsigned char   *FST; 
	unsigned long   fst_offset=0,filename_offset=0,entries=0,string_table_offset=0,fst_size=0; 
	unsigned int    found=0; 
	unsigned long   i=0,j=0,k=0,offset=0,parent_offset=0,next_offset=0,name_offset=0; 
	unsigned char   *filename=malloc(256),*path=malloc(256),*temp=malloc(256),*current_path=malloc(256); 
	
	// Initialise the vars
	memset(ELFfileNameArray,0,sizeof(ELFfileNameArray));
	memset(ELFfileOffsetArray,0,sizeof(ELFfileOffsetArray));
	memset(ELFfileSizeArray,0,sizeof(ELFfileSizeArray));
	ELFnumFiles = 0;
	strcpy(filename,""); 
	strcpy(path,""); 
	strcpy(current_path,"/"); 
	
	
	// Load all the FST in memory : 
	fseek(file,0x424,SEEK_SET); 
	fread(buffer, 1, 8, file);
	fst_offset=(unsigned int)buffer[0]*256*256*256+(unsigned int)buffer[1]*256*256+(unsigned int)buffer[2]*256+(unsigned int)buffer[3]; 
//	printf("Fst offset=%08X - ",fst_offset); 
	fst_size=(unsigned int)buffer[4]*256*256*256+(unsigned int)buffer[5]*256*256+(unsigned int)buffer[6]*256+(unsigned int)buffer[7]; 
	FST=malloc(fst_size); 
	fseek(file,fst_offset,SEEK_SET); 
	fread(FST, 1, fst_size, file);

	
	entries=(unsigned int)FST[8]*256*256*256+(unsigned int)FST[9]*256*256+(unsigned int)FST[10]*256+(unsigned int)FST[11]; 
	string_table_offset=12*entries; 
//	printf("Number of FST entries : %i\n",entries); 
	
	for (i=1;i<entries;i++) 
	{ 
		offset=i*0x0c; 
		if(FST[offset]==0) 
		{ 
			filename_offset=(unsigned int)FST[offset+1]*256*256+(unsigned int)FST[offset+2]*256+(unsigned int)FST[offset+3]; 
			strcpy(filename,&FST[string_table_offset+filename_offset]); 
			
			// Looking for the full path of the file : 
			strcpy(path,""); 
			strcpy(temp,""); 
			j=0x0c; 
			found=0; 
			while ( (j<offset) && (found==0) ) 
			{ 
				// Does the file appear in a "next offset" reference ? 
				// (search starts from the begining of the FST) 
				if(FST[j]==1) 
				{ 
					next_offset=(unsigned int)FST[j+8]*256*256*256+(unsigned int)FST[j+9]*256*256+(unsigned int)FST[j+10]*256+(unsigned int)FST[j+11]; 
					if (next_offset==i) 
					{ 
						parent_offset=(unsigned int)FST[j+4]*256*256*256+(unsigned int)FST[j+5]*256*256+(unsigned int)FST[j+6]*256+(unsigned int)FST[j+7]; 
						while(parent_offset!=0) 
						{ 
							// Folder's name : 
							name_offset=(unsigned int)FST[parent_offset*0x0c+1]*256*256+(unsigned int)FST[parent_offset*0x0c+2]*256+(unsigned int)FST[parent_offset*0x0c+3]; 
							strcpy(temp,&FST[string_table_offset+name_offset]); 
							strcat(temp,"/"); 
							strcat(temp,path); 
							strcpy(path,temp); 
							
							parent_offset=(unsigned int)FST[parent_offset*0x0c+4]*256*256*256+(unsigned int)FST[parent_offset*0x0c+5]*256*256+(unsigned int)FST[parent_offset*0x0c+6]*256+(unsigned int)FST[parent_offset*0x0c+7]; 
						} 
						strcpy(temp,"/"); 
						strcat(temp,path); 
						strcpy(path,temp); 
						found=1; 
					} 
				} 
				j=j+0x0c; 
			} 
			// If the file is not in a "next offset" reference 
			//      check the if the previous entry is not a directory 
			//      if YES path=path of the previous entry (which is a folder) 
			//      if NO path=curent path (=the path of the previous file) 
			// else 
			//      path=path previously found 
			if(found==1) 
			{ 
				strcpy(current_path,path); 
			} 
			else 
			{ 
				if ((offset>0x0c) && (FST[offset-0x0c]==1)) 
				{ 
					parent_offset=i-1; 
					while(parent_offset!=0) 
					{ 
						// Folder's name : 
						name_offset=(unsigned int)FST[parent_offset*0x0c+1]*256*256+(unsigned int)FST[parent_offset*0x0c+2]*256+(unsigned int)FST[parent_offset*0x0c+3]; 
						strcpy(temp,&FST[string_table_offset+name_offset]); 
						strcat(temp,"/"); 
						strcat(temp,path); 
						strcpy(path,temp); 
						
						parent_offset=(unsigned int)FST[parent_offset*0x0c+4]*256*256*256+(unsigned int)FST[parent_offset*0x0c+5]*256*256+(unsigned int)FST[parent_offset*0x0c+6]*256+(unsigned int)FST[parent_offset*0x0c+7]; 
					} 
					strcpy(temp,"/"); 
					strcat(temp,path); 
					strcpy(path,temp); 
					strcpy(current_path,path); 
				} 
			} 
			unsigned char *tmp;
			unsigned int loc = 0;
			unsigned int len = 0;
			tmp = malloc(4);
			strcpy(tmp,filename+(strlen(filename)-4));
			memcpy(&loc,&FST[offset+4],4);
			memcpy(&len,&FST[offset+8],4);
			//printf("file: %s%s 0x%08X\n",current_path,filename,convert_int(loc));
			if((!stricmp(tmp,".elf")) || (!stricmp(tmp,".ELF")) || (!stricmp(tmp,".dol")) || (!stricmp(tmp,".DOL"))) 
			{
				strcpy(&ELFfileNameArray[ELFnumFiles][0],current_path);
				strcat(&ELFfileNameArray[ELFnumFiles][0],filename);
				ELFfileOffsetArray[ELFnumFiles] = convert_int(loc);
				ELFfileSizeArray[ELFnumFiles] = convert_int(len);
				if((!stricmp(tmp,".dol")) || (!stricmp(tmp,".DOL")) )
					ELFfileIsDOL[ELFnumFiles] = 1;
				ELFnumFiles++;
			}
			if((!stricmp(tmp,".tgc")) || (!stricmp(tmp,".TGC")))
			{
				strcpy(&TGCfileNameArray[TGCnumFiles][0],current_path);
				strcat(&TGCfileNameArray[TGCnumFiles][0],filename);
				TGCfileOffsetArray[TGCnumFiles] = convert_int(loc);
				TGCfileSizeArray[TGCnumFiles] = convert_int(len);
				TGCapploaderOffsetsArray[TGCnumFiles] = TGCfileOffsetArray[TGCnumFiles];
				TGCnumFiles++;
			}
			if(!stricmp(filename,"execD.img")) {
				ReloaderOffsetArray[ReloaderNumFiles] = convert_int(loc);
				ReloaderSizeArray[ReloaderNumFiles] = convert_int(len);
				ReloaderNumFiles++;
			}

			free (tmp);
		} 
	} 
	
}

void parse_tgc(FILE *file,unsigned int tgc_base) {
 
	unsigned char   buffer[256]; 
	unsigned char   *FST; 
	unsigned long   fst_offset=0,filename_offset=0,entries=0,string_table_offset=0,fst_size=0; 
	unsigned int    found=0; 
	unsigned long   i=0,j=0,k=0,offset=0,parent_offset=0,next_offset=0,name_offset=0; 
	unsigned char   *filename=malloc(256),*path=malloc(256),*temp=malloc(256),*current_path=malloc(256); 


	// add this embedded GCM's main DOL
	unsigned int mainDOLoffset=0,mainDOLsize=0,fakeAmount=0,fileAreaStart=0;
	fseek(file,tgc_base+0x1C,SEEK_SET); 
	fread(&mainDOLoffset, 1, 4, file);
	fread(&mainDOLsize, 1, 4, file);
	fread(&fileAreaStart, 1, 4, file);
	fseek(file,tgc_base+0x34,SEEK_SET); 
	fread(&fakeAmount, 1, 4, file);
	fakeAmount=convert_int(fakeAmount);
	fileAreaStart=convert_int(fileAreaStart);
	mainDOLoffset=convert_int(mainDOLoffset);
	mainDOLsize=convert_int(mainDOLsize);
	strcpy(&ELFfileNameArray[ELFnumFiles][0],"TGC Main DOL");
	ELFfileOffsetArray[ELFnumFiles] = mainDOLoffset+tgc_base;
	ELFfileSizeArray[ELFnumFiles] = mainDOLsize;
	ELFfileIsDOL[ELFnumFiles] = 1;
	ELFnumFiles++;
	
	strcpy(filename,""); 
	strcpy(path,""); 
	strcpy(current_path,"/"); 
	
	
	// Load all the FST in memory : 
	fseek(file,tgc_base+0x10,SEEK_SET); 
	fread(buffer, 1, 8, file);
	fst_offset=(unsigned int)buffer[0]*256*256*256+(unsigned int)buffer[1]*256*256+(unsigned int)buffer[2]*256+(unsigned int)buffer[3]; 
//	printf("Fst offset=%08X - ",fst_offset); 
	fst_size=(unsigned int)buffer[4]*256*256*256+(unsigned int)buffer[5]*256*256+(unsigned int)buffer[6]*256+(unsigned int)buffer[7]; 
	FST=malloc(fst_size); 
	fseek(file,fst_offset+tgc_base,SEEK_SET); 
	fread(FST, 1, fst_size, file);
	
	entries=(unsigned int)FST[8]*256*256*256+(unsigned int)FST[9]*256*256+(unsigned int)FST[10]*256+(unsigned int)FST[11]; 
	string_table_offset=12*entries; 
//	printf("Number of FST entries : %i\n",entries); 
	
	for (i=1;i<entries;i++) 
	{ 
		offset=i*0x0c; 
		if(FST[offset]==0) 
		{ 
			filename_offset=(unsigned int)FST[offset+1]*256*256+(unsigned int)FST[offset+2]*256+(unsigned int)FST[offset+3]; 
			strcpy(filename,&FST[string_table_offset+filename_offset]); 
			
			// Looking for the full path of the file : 
			strcpy(path,""); 
			strcpy(temp,""); 
			j=0x0c; 
			found=0; 
			while ( (j<offset) && (found==0) ) 
			{ 
				// Does the file appear in a "next offset" reference ? 
				// (search starts from the begining of the FST) 
				if(FST[j]==1) 
				{ 
					next_offset=(unsigned int)FST[j+8]*256*256*256+(unsigned int)FST[j+9]*256*256+(unsigned int)FST[j+10]*256+(unsigned int)FST[j+11]; 
					if (next_offset==i) 
					{ 
						parent_offset=(unsigned int)FST[j+4]*256*256*256+(unsigned int)FST[j+5]*256*256+(unsigned int)FST[j+6]*256+(unsigned int)FST[j+7]; 
						while(parent_offset!=0) 
						{ 
							// Folder's name : 
							name_offset=(unsigned int)FST[parent_offset*0x0c+1]*256*256+(unsigned int)FST[parent_offset*0x0c+2]*256+(unsigned int)FST[parent_offset*0x0c+3]; 
							strcpy(temp,&FST[string_table_offset+name_offset]); 
							strcat(temp,"/"); 
							strcat(temp,path); 
							strcpy(path,temp); 
							
							parent_offset=(unsigned int)FST[parent_offset*0x0c+4]*256*256*256+(unsigned int)FST[parent_offset*0x0c+5]*256*256+(unsigned int)FST[parent_offset*0x0c+6]*256+(unsigned int)FST[parent_offset*0x0c+7]; 
						} 
						strcpy(temp,"/"); 
						strcat(temp,path); 
						strcpy(path,temp); 
						found=1; 
					} 
				} 
				j=j+0x0c; 
			} 
			// If the file is not in a "next offset" reference 
			//      check the if the previous entry is not a directory 
			//      if YES path=path of the previous entry (which is a folder) 
			//      if NO path=curent path (=the path of the previous file) 
			// else 
			//      path=path previously found 
			if(found==1) 
			{ 
				strcpy(current_path,path); 
			} 
			else 
			{ 
				if ((offset>0x0c) && (FST[offset-0x0c]==1)) 
				{ 
					parent_offset=i-1; 
					while(parent_offset!=0) 
					{ 
						// Folder's name : 
						name_offset=(unsigned int)FST[parent_offset*0x0c+1]*256*256+(unsigned int)FST[parent_offset*0x0c+2]*256+(unsigned int)FST[parent_offset*0x0c+3]; 
						strcpy(temp,&FST[string_table_offset+name_offset]); 
						strcat(temp,"/"); 
						strcat(temp,path); 
						strcpy(path,temp); 
						
						parent_offset=(unsigned int)FST[parent_offset*0x0c+4]*256*256*256+(unsigned int)FST[parent_offset*0x0c+5]*256*256+(unsigned int)FST[parent_offset*0x0c+6]*256+(unsigned int)FST[parent_offset*0x0c+7]; 
					} 
					strcpy(temp,"/"); 
					strcat(temp,path); 
					strcpy(path,temp); 
					strcpy(current_path,path); 
				} 
			} 
			
			unsigned char *tmp;
			unsigned int loc = 0;
			unsigned int len = 0;
			tmp = malloc(4);
			strcpy(tmp,filename+(strlen(filename)-4));
			memcpy(&loc,&FST[offset+4],4);
			memcpy(&len,&FST[offset+8],4);
			if((!stricmp(tmp,".elf")) || (!stricmp(tmp,".ELF")) || (!stricmp(tmp,".dol")) || (!stricmp(tmp,".DOL"))) {
				strcpy(&ELFfileNameArray[ELFnumFiles][0],"TGC:");
				strcat(&ELFfileNameArray[ELFnumFiles][0],current_path);
				strcat(&ELFfileNameArray[ELFnumFiles][0],filename);
				ELFfileOffsetArray[ELFnumFiles] = (convert_int(loc)-fakeAmount)+(tgc_base+fileAreaStart);
				ELFfileSizeArray[ELFnumFiles] = convert_int(len);
				if((!stricmp(tmp,".dol")) || (!stricmp(tmp,".DOL")))
					ELFfileIsDOL[ELFnumFiles] = 1;
				ELFnumFiles++;
			}

			free (tmp);
		} 
	} 
	
}
