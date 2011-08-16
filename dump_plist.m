/* 
 * Author: Eric Monti - Trustwave SpiderLabs
 * Date: 10/20/2010
 *
 * Compile with:
 *   gcc -o dump_plist dump_plist.m -framework Foundation
 *
 */
#include <stdlib.h>

#import <Foundation/Foundation.h>

void dump_plist(char *path) {
  NSDictionary *d = [NSDictionary dictionaryWithContentsOfFile:[NSString stringWithUTF8String:path]];
  printf("plist:%s = %s\n\n", path, [[d description] UTF8String]);
}

int main(int argc, char *argv[]) {
  int i;

  if (argc < 2) {
    fprintf(stderr, "usage: %s path/to/file.plist [...]\n", argv[0]);
    exit(1);
  }

  NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

  for(i=1; argc > i ; i++)
    dump_plist(argv[i]);

  [pool release];
  return 0;
}
