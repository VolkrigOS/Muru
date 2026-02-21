CC = cc
STRIP = strip
CFLAGS = -std=c99 -Wall -Wextra -pedantic -D_XOPEN_SOURCE=700

all: build

init:
	$(CC) $(CFLAGS) init.c -o init
	$(STRIP) init

su1:
	$(CC) $(CFLAGS) su1.c -o su1
	$(STRIP) su1

shutdown:
	$(CC) $(CFLAGS) shutdown.c -o shutdown
	$(STRIP) shutdown

reboot:
	$(CC) $(CFLAGS) reboot.c -o reboot
	$(STRIP) reboot

build: init su1 shutdown reboot

install: build
	mkdir -p $(DESTDIR)/sbin
	cp -fa init su1 shutdown reboot $(DESTDIR)/sbin

clean:
	rm -f init shutdown reboot su1
