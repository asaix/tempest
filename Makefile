CC = gcc
CFLAGS = -std=c23 -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700 \
         -Wall -Wextra -Werror -Wno-unused-parameter -g
LDLIBS =

PREFIX = $(HOME)/.local
BINDIR = $(PREFIX)/bin

SRCS = $(wildcard *.c)

tempest: $(SRCS)
	$(CC) $(CFLAGS) -o tempest $(SRCS) $(LDLIBS)

install: tempest
	mkdir -p $(DESTDIR)$(BINDIR)
	install -m 755 tempest $(DESTDIR)$(BINDIR)/tempest

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/tempest

clean:
	rm -f tempest

.PHONY: install uninstall clean
