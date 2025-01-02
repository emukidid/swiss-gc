#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>

using namespace std;

#ifndef O_BINARY
#define O_BINARY 0
#endif

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;

const u8 dol_icon[] = {
    0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
    0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x10, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x10, 0x10,
    // ... [rest of the dol_icon data]
};

int load(const string &name, void **data) {
    size_t size = 0;
    *data = NULL;
    int fd = open(name.c_str(), O_RDONLY | O_BINARY, 0);
    if (fd >= 0) {
        struct stat sb;
        if (fstat(fd, &sb) >= 0 && sb.st_size > 0) {
            void *tmp = malloc(sb.st_size);
            if (tmp != NULL) {
                if (read(fd, tmp, sb.st_size) == sb.st_size) {
                    *data = tmp;
                    size = sb.st_size;
                } else {
                    free(tmp);
                }
            }
        }
        close(fd);
    }
    return size;
}

void save(const string &name, const void *data, int size) {
    int fd = open(name.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0666);
    if (fd >= 0) {
        write(fd, data, size);
        close(fd);
    }
}

u32 get_u32be(const void *buf) {
    return ((u8 *)buf)[3] | (((u8 *)buf)[2] << 8) | (((u8 *)buf)[1] << 16) | (((u8 *)buf)[0] << 24);
}

void set_u16be(void *buf, u16 v) {
    ((u8 *)buf)[1] = v & 0xFF;
    ((u8 *)buf)[0] = (v >> 8) & 0xFF;
}

void set_u32be(void *buf, u32 v) {
    ((u8 *)buf)[3] = v & 0xFF;
    ((u8 *)buf)[2] = (v >> 8) & 0xFF;
    ((u8 *)buf)[1] = (v >> 16) & 0xFF;
    ((u8 *)buf)[0] = (v >> 24) & 0xFF;
}

int main(int argc, char *const argv[]) {
    if (argc != 3 && argc != 4) {
        fprintf(stderr, "Usage: dol2gci <dolfile> <gcifile> [<filename>]\n");
        return -1;
    }

    string name;
    if (argc == 4) {
        name = argv[3];
    } else {
        name = argv[1];
        size_t pos = name.find_last_of("/\\");
        if (pos != string::npos) {
            name = name.substr(pos + 1);
        }
    }

    u8 *dol_data;
    int dol_size = load(argv[1], (void **)&dol_data);
    if (dol_size < 256) {
        fprintf(stderr, "Error: Can't read DOL file\n");
        return -1;
    }

    int icon_size = ((sizeof(dol_icon) + 31) & ~31);
    int data_size = 64 + icon_size + dol_size;
    int data_blocks = (data_size + 8191) / 8192;

    int gci_size = 64 + data_blocks * 8192;
    u8 *gci_data = (u8 *)malloc(gci_size);
    if (!gci_data) {
        free(dol_data);
        fprintf(stderr, "Error: Memory allocation failed\n");
        return -1;
    }

    memcpy(gci_data, "DOLX", 4);
    memcpy(gci_data + 4, "00", 2);
    gci_data[6] = 0xFF;
    gci_data[7] = 0x00;
    memset(gci_data + 8, 0, 32);
    strncpy((char *)(gci_data + 8), name.c_str(), 31);
    set_u32be(gci_data + 0x28, 0);
    set_u32be(gci_data + 0x2C, 0x140);
    set_u16be(gci_data + 0x30, 0x0001);
    set_u16be(gci_data + 0x32, 0x02);
    gci_data[0x34] = 0x04;
    gci_data[0x35] = 0x00;
    set_u16be(gci_data + 0x36, 0x0000);
    set_u16be(gci_data + 0x38, data_blocks);
    set_u16be(gci_data + 0x3a, 0xffff);
    set_u32be(gci_data + 0x3c, 0x100);

    memcpy(gci_data + 0x40, dol_data, 256);
    for (unsigned int i = 0; i < 18; i++) {
        u32 adr = 0x40 + i * 4;
        u32 filepos = get_u32be(gci_data + adr);
        if (filepos >= 0x100) {
            set_u32be(gci_data + adr, filepos + 0x40 + icon_size);
        }
    }

    memset(gci_data + 0x140, 0, 64);
    strcpy((char *)(gci_data + 0x140), "Dolphin Application");
    strncpy((char *)(gci_data + 0x160), name.c_str(), 31);
    memcpy(gci_data + 0x180, dol_icon, sizeof(dol_icon));
    memcpy(gci_data + 0x180 + icon_size, dol_data + 256, dol_size - 256);

    save(argv[2], gci_data, gci_size);

    free(gci_data);
    free(dol_data);

    return 0;
}
