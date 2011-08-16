#include "macho-tools.h"

char *cputypes[19] = {
    "NULL",     // 0
    "VAX",      // 1
    "UNK2",     // 2
    "UNK3",     // 3
    "UNK4",     // 4
    "UNK5",     // 5
    "MC680x0",  // 6
    "X86",      // 7
    "MIPS",     // 8
    "UNK9",     // 9
    "MC98000",  // 10
    "HPPA",     // 11
    "ARM",      // 12
    "MC88000",  // 13
    "SPARC",    // 14
    "I860",     // 15
    "ALPHA",    // 16
    "UNK17",    // 17
    "PPC",      // 18
};

int dump_mh_cryptinfo(char *pinput, int ncmds, uint32_t offset) {
    int i=0;
    char *ptr = pinput + offset;

    for(i=0; i < ncmds; i++) {
        struct load_command *lc = (struct load_command *) ptr;

        /* look for Encryption info segment */
        if(lc->cmd == LC_ENCRYPTION_INFO) {
            struct encryption_info_command *crypt_cmd = (struct encryption_info_command *) lc;

            printf("  LC_ENCRYPTION_INFO at offset 0x%.8x => {\n", ptr - pinput);
            printf("    cryptid   %d\n", crypt_cmd->cryptid);
            printf("    cryptoff  %d\n", crypt_cmd->cryptoff);
            printf("    cryptsize %d\n", crypt_cmd->cryptsize);
            printf("  }\n");

            return(1);
        }
        ptr += lc->cmdsize;
    }
    fprintf(stderr, "    Didn't find a LC_ENCRYPTION_INFO command\n");
    return(0);
}

int dump_mach_hdr_32(HDR *mh) {
   //struct mach_header {
   //         uint32_t        magic;          /* mach magic number identifier */
   //         cpu_type_t      cputype;        /* cpu specifier */
   //         cpu_subtype_t   cpusubtype;     /* machine specifier */
   //         uint32_t        filetype;       /* type of file */
   //         uint32_t        ncmds;          /* number of load commands */
   //         uint32_t        sizeofcmds;     /* the size of all the load commands */
   //         uint32_t        flags;          /* flags */
   // };
    char *      cputype_str;
    cpu_type_t  cputype = (mh->mach_header_64.cputype & ~CPU_ARCH_MASK);

    if (cputype > CPUTYPE_MAX || (cputype_str = cputypes[ cputype ]) == NULL)
        cputype_str = "INVALID";

    fprintf(stderr, "- 32-bit MACH header cputype=%s%s(0x%.2x) cpusubtype=%d ncmds=%d\n", 
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

    fprintf(stderr, "- 64-bit MACH header cputype=%s%s(0x%.2x) cpusubtype=%d ncmds=%d\n", 
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
