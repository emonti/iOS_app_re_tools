/* archinfo by eric monti
 * a utility for displaying the effective mach-o architecture */

#include <mach-o/arch.h>
#include <stdio.h>

void print_arch(const NXArchInfo *ai) {
  char * byteorders[3] = {"unknown", "little-endian", "big-endian"};
  printf("  name=%s, desc='%s', cputype=%i, subtype=%i, byteorder=%s(%i)\n",
      ai->name, ai->description, ai->cputype, ai->cpusubtype, 
      byteorders[(ai->byteorder > 2 || ai->byteorder < 0 ) ? 0 : ai->byteorder], 
      ai->byteorder
  );
}

int main(int argc, char ** argv) {
  const NXArchInfo *ai_p;

  printf("Local Architecture:\n");
  ai_p = NXGetLocalArchInfo();
  print_arch(ai_p);
  printf("\n");

#ifdef DUMP_ALL_KNOWN
  printf("Known Architectures:\n");
  ai_p = NXGetAllArchInfos();
  while(ai_p && ai_p->name) {
    print_arch(ai_p);
    printf("\n");
    ai_p++;
  }
#endif
}
