CC = cc
STRIP = strip

CFLAGS = -std=c99 -Os -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Wformat=2 -Wundef -D_XOPEN_SOURCE=700 -ffunction-sections -fdata-sections -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-ident -static
LDFLAGS = -Wl,--gc-sections -Wl,--as-needed -static

PREFIX = /usr
SBINDIR = $(PREFIX)/sbin
DESTDIR =

PROGRAMS = init su1 shutdown reboot murud muructl

all: build

init: init.c
	$(CC) $(CFLAGS) $(LDFLAGS) init.c -o init
	$(STRIP) --strip-all init

su1: su1.c
	$(CC) $(CFLAGS) $(LDFLAGS) su1.c -o su1
	$(STRIP) --strip-all su1

shutdown: shutdown.c
	$(CC) $(CFLAGS) $(LDFLAGS) shutdown.c -o shutdown
	$(STRIP) --strip-all shutdown

reboot: reboot.c
	$(CC) $(CFLAGS) $(LDFLAGS) reboot.c -o reboot
	$(STRIP) --strip-all reboot

build: $(PROGRAMS)

install: build
	mkdir -p $(DESTDIR)$(SBINDIR)
	cp -f $(PROGRAMS) $(DESTDIR)$(SBINDIR)/

uninstall:
	rm -f $(addprefix $(DESTDIR)$(SBINDIR)/,$(PROGRAMS))

clean:
	rm -f $(PROGRAMS)

.PHONY: all build install uninstall clean
