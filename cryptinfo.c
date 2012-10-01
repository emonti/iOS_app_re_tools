#include <mach-o/fat.h>
#include <mach-o/loader.h>
#include <sys/mman.h>
#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

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

int do_patch=0;

char *cputypes[19] = {
    "NULL",     // 0
    "VAX",
    "UNK2",
    "UNK3",
    "UNK4",
    "UNK5",
    "MC680x0",
    "X86",
    "MIPS",
    "UNK9",
    "MC98000",
    "HPPA",
    "ARM",
    "MC88000",
    "SPARC",
    "I860",
    "ALPHA",
    "UNK17",
    "PPC",      // 18
};

int dump_mh_cryptinfo(char *pinput, int ncmds, uint32_t offset) {
    int i=0;
    char *ptr = pinput + offset;

    for(i=0; i < ncmds; i++) {
        struct load_command *lc = (struct load_command *) ptr;

        /* find the encryption info segment */
        if(lc->cmd == LC_ENCRYPTION_INFO) {
            struct encryption_info_command *crypt_cmd = (struct encryption_info_command *) lc;

            printf("  LC_ENCRYPTION_INFO at offset 0x%x => {\n",  (uint32_t)(ptr - pinput));
            printf("    cryptid   %d\n", crypt_cmd->cryptid);
            printf("    cryptoff  %d\n", crypt_cmd->cryptoff);
            printf("    cryptsize %d\n", crypt_cmd->cryptsize);
            printf("  }\n");

            if (do_patch) {
              uint32_t newid = (! crypt_cmd->cryptid);
              printf("!! Patching cryptid at 0x%x to: %d\n", (uint32_t)(ptr - pinput), newid);
              crypt_cmd->cryptid = newid;
            }

            return(1);
        }
        ptr += lc->cmdsize;
    }
    printf("    Didn't find a LC_ENCRYPTION_INFO command\n");
    return(0);
}

int dump_mach_hdr_32(HDR *mh) {

    char *      cputype_str;
    cpu_type_t  cputype = (mh->mach_header_64.cputype & ~CPU_ARCH_MASK);

    if (cputype > CPUTYPE_MAX || (cputype_str = cputypes[ cputype ]) == NULL)
        cputype_str = "INVALID";

    printf("- 32-bit MACH header cputype=%s%s(0x%.2x) cpusubtype=%d ncmds=%d\n", 
            cputype_str, 
            (((mh->mach_header.cputype & CPU_ARCH_ABI64) == CPU_ARCH_ABI64)? "_64" : ""),
            cputype,
            (mh->mach_header.cpusubtype & ~CPU_SUBTYPE_MASK),
            mh->mach_header.ncmds );

    return(0);
}

int dump_mach_hdr_64(HDR *mh) {
    char *      cputype_str;
    cpu_type_t  cputype = (mh->mach_header_64.cputype & ~CPU_ARCH_MASK);

    if (cputype > CPUTYPE_MAX || (cputype_str = cputypes[ cputype ]) == NULL)
        cputype_str = "INVALID";

    printf("- 64-bit MACH header cputype=%s%s(0x%.2x) cpusubtype=%d ncmds=%d\n", 
            cputype_str, 
            (((mh->mach_header_64.cputype & CPU_ARCH_ABI64) == CPU_ARCH_ABI64)? "_64" : ""),
            cputype,
            (mh->mach_header_64.cpusubtype & ~CPU_SUBTYPE_MASK),
            mh->mach_header_64.ncmds );

    return(0);
}

int dump_mach_header(char *input) {
    HDR *header = (HDR *) input;

    if (header->mach_header.magic == MH_MAGIC || 
        header->mach_header.magic == MH_CIGAM) {

        dump_mach_hdr_32(header);
        return dump_mh_cryptinfo( input, header->mach_header.ncmds, sizeof(struct mach_header));

    } else if (header->mach_header_64.magic == MH_MAGIC_64 || 
               header->mach_header_64.magic == MH_CIGAM_64) {

        dump_mach_hdr_64(header);
        return dump_mh_cryptinfo(input, header->mach_header_64.ncmds, sizeof(struct mach_header_64));

    } 
}

void usage(char *progname) {
    fprintf(stderr, "Usage: %s [?hp] filename\n", progname);
    fprintf(stderr, "  -?/-h  Show this help message\n");
    fprintf(stderr, "  -p     Patch (flip) cryptid in LC_ENCRYPTION_INFO\n");
    exit(1);
}

int main(int argc, char **argv) {
    int fd, ncmds, ret, ch;
    off_t fsize;
    char *progname = argv[0];
    char *input;
    uint32_t i=0, nfat_arch;
    HDR *header;
    char *fname = argv[1];

    while((ch = getopt(argc,argv, "?hp")) != -1) {
      switch(ch) {
        case 'p':
          printf("!! Patch-mode enabled\n");
          do_patch = 1;
          break;
        case '?':
        case 'h':
        default:
          usage(progname);
      }
    }
    argc -= optind;
    argv += optind;

    if (argc != 1)
      usage(progname);

    fd = open(fname, O_RDWR);
    assert(fd > -1);

    fsize = lseek(fd, 0, SEEK_END);
    assert(fsize != 0);

    assert(lseek(fd, 0, SEEK_SET) == 0);

    if (write(fd, "", 0) != 0) {
        close(fd);
        perror("Error can't write to file");
        exit(EXIT_FAILURE);
    }

    input = mmap(NULL, fsize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (input < 0) {
        close(fd);
        perror("mmap() error");
        exit(EXIT_FAILURE);
    }

    header = (HDR *) input;

    if (header->fat_header.magic == FAT_MAGIC || 
        header->fat_header.magic == FAT_CIGAM) {

        char *fat_input = input;
        struct fat_arch *fat_arch;

        nfat_arch = OSSwapBigToHostInt32(header->fat_header.nfat_arch);
        printf("FAT binary with %i architectures\n", nfat_arch);

        fat_arch = (struct fat_arch *) (fat_input + sizeof(struct fat_header));
        for(;nfat_arch-- > 0; fat_arch++) {
            uint32_t offset, size, align, cputype, cpusubtype;

            offset     = OSSwapBigToHostInt32(fat_arch->offset);
            size       = OSSwapBigToHostInt32(fat_arch->size);
            align      = OSSwapBigToHostInt32(fat_arch->align);
            cputype    = OSSwapBigToHostInt32(fat_arch->cputype);
            cpusubtype = OSSwapBigToHostInt32(fat_arch->cpusubtype);

            printf("\nfat_arch %i = {\n", i+=1);
            printf("  cputype    0x%.8x\n", cputype);
            printf("  cpusubtype 0x%.8x\n", cpusubtype);
            printf("  offset     0x%.8x\n", offset);
            printf("  size       0x%.8x\n", size);
            printf("  align      0x%.8x\n", align);
            printf("}\n");

            ret=dump_mach_header(input + offset);
        }

    } else {
        ret=dump_mach_header(input);
    }

    if (do_patch) {
        printf("!! Writing changes to: %s\n", argv[0]);
        if(msync(input, fsize, MS_SYNC) == -1)
            perror("msync error");
    }

    if (munmap(input, fsize) == -1)
        perror("munmap error");

    close(fd);
    return(ret);
}


