#include <mach-o/fat.h>
#include <mach-o/loader.h>
#include <sys/mman.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* The encryption info struct and constants are missing from the iPhoneSimulator SDK, but not from the iPhoneOS or
 * Mac OS X SDKs. Since one doesn't ever ship a Simulator binary, we'll just provide the definitions here. */
#if TARGET_IPHONE_SIMULATOR && !defined(LC_ENCRYPTION_INFO)
#define LC_ENCRYPTION_INFO 0x21
struct encryption_info_command {
    uint32_t cmd;
    uint32_t cmdsize;
    uint32_t cryptoff;
    uint32_t cryptsize;
    uint32_t cryptid;
};
#endif

typedef union hdr_t {
    struct fat_header       fat_header;
    struct mach_header      mach_header;
    struct mach_header_64   mach_header_64;
} HDR;

#define CPUTYPE_MAX 18

int dump_mh_cryptinfo(char *pinput, int ncmds, uint32_t offset);
int dump_mach_hdr_32(HDR *mh);
int dump_mach_hdr_64(HDR *mh);
int dump_mach_header(char *input);
