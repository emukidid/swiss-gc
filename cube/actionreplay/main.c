#define DOLHDRLENGTH 256
#define MAXTEXTSECTION 7
#define MAXDATASECTION 11

typedef struct {
    unsigned int textOffset[MAXTEXTSECTION];
    unsigned int dataOffset[MAXDATASECTION];

    unsigned int textAddress[MAXTEXTSECTION];
    unsigned int dataAddress[MAXDATASECTION];

    unsigned int textLength[MAXTEXTSECTION];
    unsigned int dataLength[MAXDATASECTION];

    unsigned int bssAddress;
    unsigned int bssLength;

    unsigned int entryPoint;
    unsigned int unused[MAXTEXTSECTION];
} DOLHEADER;

extern unsigned char swiss_lz_dol[];
extern unsigned long swiss_lz_dol_size;

void _memcpy(void* dest, const void* src, int count) {
	char* tmp = (char*)dest,* s = (char*)src;
	while (count--)
		*tmp++ = *s++;
}

void _memset(void* s, int c, int count) {
	char* xs = (char*)s;
	while (count--)
		*xs++ = c;
}

void boot_dol() 
{
	int i;
	DOLHEADER *hdr = (DOLHEADER *) swiss_lz_dol;

	// Inspect text sections to see if what we found lies in here
	for (i = 0; i < MAXTEXTSECTION; i++) {
		if (hdr->textAddress[i] && hdr->textLength[i]) {
			_memcpy((void*)hdr->textAddress[i], ((unsigned char*)swiss_lz_dol) + hdr->textOffset[i], hdr->textLength[i]);
		}
	}

	// Inspect data sections (shouldn't really need to unless someone was sneaky..)
	for (i = 0; i < MAXDATASECTION; i++) {
		if (hdr->dataAddress[i] && hdr->dataLength[i]) {
			_memcpy((void*)hdr->dataAddress[i], ((unsigned char*)swiss_lz_dol) + hdr->dataOffset[i], hdr->dataLength[i]);
		}
	}
	
	// Clear BSS
	_memset((void*)hdr->bssAddress, 0, hdr->bssLength);
	
	void (*entrypoint)();
	entrypoint = (void(*)())hdr->entryPoint;
	entrypoint();
}
