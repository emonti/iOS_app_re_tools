#include "macho-tools.h"

int main(int argc, char **argv) {
    assert(argc > 1);

    int fd, ncmds, ret;
    off_t fsize;
    char *input;
    uint32_t i=0, nfat_arch;
    HDR *header;

    fd = open(argv[1], O_RDONLY);
    assert(fd > 0);

    fsize = lseek(fd, 0, SEEK_END);
    assert(fsize != 0);

    input = mmap(NULL, fsize, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(input != MAP_FAILED);

    header = (HDR *) input;

    if (header->fat_header.magic == FAT_MAGIC || 
        header->fat_header.magic == FAT_CIGAM) {

        char *fat_input = input;
        struct fat_arch *fat_arch;

        nfat_arch = OSSwapBigToHostInt32(header->fat_header.nfat_arch);
        fprintf(stderr, "FAT binary with %i architectures\n", nfat_arch);

        fat_arch = (struct fat_arch *) (fat_input + sizeof(struct fat_header));
        for(;nfat_arch-- > 0; fat_arch++) {
            uint32_t offset, size, align, cputype, cpusubtype;

            offset     = OSSwapBigToHostInt32(fat_arch->offset);
            size       = OSSwapBigToHostInt32(fat_arch->size);
            align      = OSSwapBigToHostInt32(fat_arch->align);
            cputype    = OSSwapBigToHostInt32(fat_arch->cputype);
            cpusubtype = OSSwapBigToHostInt32(fat_arch->cpusubtype);

            fprintf(stderr, "\nfat_arch %i = {\n", i+=1);
            fprintf(stderr, "  cputype    0x%.8x\n", cputype);
            fprintf(stderr, "  cpusubtype 0x%.8x\n", cpusubtype);
            fprintf(stderr, "  offset     0x%.8x\n", offset);
            fprintf(stderr, "  size       0x%.8x\n", size);
            fprintf(stderr, "  align      0x%.8x\n", align);
            fprintf(stderr, "}\n");

            ret=dump_mach_header(input + offset);
        }

    } else {
        ret=dump_mach_header(input);
    }
    return(0);
}


