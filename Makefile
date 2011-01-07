include iOS_env.mk

IPHONE_ADDR=iphone
IPHONE_DIR=/var/mobile

LDFLAGS=-framework Foundation -undefined warning -flat_namespace -DDEBUG 

PROGS=phoshizzle.dylib phoshizzle2.dylib archinfo
PLISTS=phoshizzle.plist phoshizzle2.plist

all: $(PROGS)

phoshizzle.dylib: phoshizzle.m
	$(CC) -o $@ $(CFLAGS) $< -dynamiclib -init _hook_setup $(LDFLAGS)

phoshizzle2.dylib: phoshizzle2.m
	$(CC) -o $@ $(CFLAGS) $< -dynamiclib -init _hook_setup $(LDFLAGS)

archinfo: archinfo.c
	$(CC) $(CFLAGS) -o $@ $<

push:
	scp $(PROGS) $(PLISTS) $(IPHONE_ADDR):$(IPHONE_DIR)

clean:
	rm -f *.o $(PROGS)

