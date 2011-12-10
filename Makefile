include iOS_env.mk

IPHONE_ADDR=iphone
IPHONE_DIR=/var/mobile

LIBLDFLAGS=-framework Foundation -undefined warning -flat_namespace -DDEBUG 

PROGS=phoshizzle.dylib\
      phoshizzle2.dylib\
      archinfo\
      cryptinfo\
      dump_keychain\
      dump_plist

PLISTS=phoshizzle.plist phoshizzle2.plist

all: $(PROGS)

phoshizzle.dylib: phoshizzle.m
	$(CC) -o $@ $(CFLAGS) $< -dynamiclib -init _hook_setup $(LIBLDFLAGS)

phoshizzle2.dylib: phoshizzle2.m
	$(CC) -o $@ $(CFLAGS) $< -dynamiclib -init _hook_setup $(LIBLDFLAGS)

# dump keychain is built with armv7 since ldid only supports this arch
dump_keychain: dump_keychain.m
	$(CC) -o $@ $(CFLAGS) $< -framework Foundation -framework Security -lsqlite3

dump_plist: dump_plist.m
	$(CC) $(CFLAGS) -o $@ $< -framework Foundation

cryptinfo: cryptinfo.c
	$(CC) $(CFLAGS) -o $@ $<

archinfo: archinfo.c
	$(CC) $(CFLAGS) -o $@ $<

push:
	scp $(PROGS) $(PLISTS) $(IPHONE_ADDR):$(IPHONE_DIR)

clean:
	rm -f *.o $(PROGS)

