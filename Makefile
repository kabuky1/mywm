CC = gcc
CFLAGS = -Wall -Wextra -O2
LDFLAGS = -lX11

PREFIX = $(HOME)/.local
BINDIR = $(PREFIX)/bin

all: wm

wm: wm.o
	$(CC) -o $@ $^ $(LDFLAGS)

wm.o: wm.c config.h
	$(CC) $(CFLAGS) -c $<

install: wm
	mkdir -p $(BINDIR)
	install -m 755 wm $(BINDIR)/wm

clean:
	rm -f wm *.o

.PHONY: all clean install
