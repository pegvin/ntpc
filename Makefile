# ntpc - a tiny NTP client
# See LICENSE file for copyright and license details.

PREFIX    = /usr/local
MANPREFIX = $(PREFIX)/share/man
# OpenBSD (uncomment)
#MANPREFIX = ${PREFIX}/man

VERSION   = 0.1
CFLAGS    = -std=c99 -pedantic -Wall -Wextra -Os $(INCS) -DVERSION=\"$(VERSION)\"
LDFLAGS   =
CC        = cc

SRC = ntp.c

all: ntpc

ntpc: $(SRC)
	@echo CC - $(SRC)
	@$(CC) $(CFLAGS) $(SRC) -o ./$@ $(LDFLAGS)

clean:
	@rm -f ./ntpc $(OBJ) ntpc-$(VERSION).tar.gz
	@echo Cleaned

dist: clean
	mkdir -p ntpc-$(VERSION)
	cp LICENSE Makefile README.md ntpc.1 $(SRC) ntpc-$(VERSION)
	tar -cf ntpc-$(VERSION).tar ntpc-$(VERSION)
	gzip ntpc-$(VERSION).tar
	rm -rf ntpc-$(VERSION)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f ./ntpc $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/ntpc
	mkdir -p $(DESTDIR)$(MANPREFIX)/man1
	sed "s/VERSION/$(VERSION)/g" < ntpc.1 > $(DESTDIR)$(MANPREFIX)/man1/ntpc.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/ntpc.1

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/ntpc $(DESTDIR)$(MANPREFIX)/man1/ntpc.1

.PHONY: all options clean dist install uninstall

