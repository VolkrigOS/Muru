CC = cc
STRIP = strip

CFLAGS = -std=c99 -Wall -Wextra -pedantic -D_XOPEN_SOURCE=700 -static

PREFIX = /usr
SBINDIR = $(PREFIX)/sbin
DESTDIR =

PROGRAMS = init su1 shutdown reboot

all: build

init: init.c
	$(CC) $(CFLAGS) init.c -o init
	$(STRIP) init

su1: su1.c
	$(CC) $(CFLAGS) su1.c -o su1
	$(STRIP) su1

shutdown: shutdown.c
	$(CC) $(CFLAGS) shutdown.c -o shutdown
	$(STRIP) shutdown

reboot: reboot.c
	$(CC) $(CFLAGS) reboot.c -o reboot
	$(STRIP) reboot

build: $(PROGRAMS)

install: build
	mkdir -p $(DESTDIR)$(SBINDIR)
	cp -f $(PROGRAMS) $(DESTDIR)$(SBINDIR)/

uninstall:
	rm -f $(addprefix $(DESTDIR)$(SBINDIR)/,$(PROGRAMS))

clean:
	rm -f $(PROGRAMS)

.PHONY: all build install uninstall clean