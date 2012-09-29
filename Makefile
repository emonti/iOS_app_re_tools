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

dump_keychain: dump_keychain.m
	$(CC) -o $@ $(CFLAGS) $< -framework Foundation -framework Security -lsqlite3

# target dump_keychain with armv6 macho section only - ldid fat support sux
dump_keychain_armv6: dump_keychain.m
	$(CC) -o $@ -arch armv6 $< -framework Foundation -framework Security -lsqlite3

lsos: lsos.c
	$(CC) $(CFLAGS) -o $@ $<

testcert: testcert.m
	$(CC) $(CFLAGS) -o $@ $< -framework Foundation -framework Security

dump_plist: dump_plist.m
	$(CC) $(CFLAGS) -o $@ $< -framework Foundation

cryptinfo: cryptinfo.c
	$(CC) $(CFLAGS) -o $@ $<

archinfo: archinfo.c
	$(CC) $(CFLAGS) -o $@ $<

push:
	scp $(PROGS) $(PLISTS) $(IPHONE_ADDR):$(IPHONE_DIR)

clean:
	rm -f *.o $(PROGS) dump_keychain_armv6

codesign: dump_keychain
	codesign -f -s "iPhone Developer" --entitlements keychain.xcent dump_keychain
