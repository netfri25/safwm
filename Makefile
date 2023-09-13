CFLAGS += -std=c2x -Wall -Wextra -pedantic
CFLAGS += -I/usr/include/freetype2
PREFIX ?= /usr
LINKIN += -lX11 -lpthread -lm
BINDIR ?= $(PREFIX)/bin
CC     ?= gcc

all: safwm

config.h:
	cp config.def.h config.h

safwm: safwm.c safwm.h keymaps.h config.h Makefile
	$(CC) -O3 $(CFLAGS) -o $@ $< $(LINKIN) $(LDFLAGS)

install: all
	install -Dm755 safwm $(DESTDIR)$(BINDIR)/safwm

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/safwm

clean:
	rm -f safwm *.o

.PHONY: all install uninstall clean
