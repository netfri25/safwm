CFLAGS += -Wall -Wextra -pedantic
CFLAGS += -I/usr/include/freetype2
PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin
CC     ?= gcc

all: safwm

config.h:
	cp config.def.h config.h

safwm: safwm.c safwm.h config.h Makefile
	$(CC) -O3 $(CFLAGS) -o $@ $< -lX11 $(LDFLAGS)

install: all
	install -Dm755 safwm $(DESTDIR)$(BINDIR)/safwm

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/safwm

clean:
	rm -f safwm *.o

.PHONY: all install uninstall clean
