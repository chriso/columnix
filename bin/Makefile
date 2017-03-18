DESTDIR ?=
PREFIX  ?= /usr/local
BINDIR ?= $(PREFIX)/bin

CFLAGS += -std=c99 -Os -g -pedantic -Wall
LDLIBS = -lcolumnix

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
BIN = $(basename $(SRC))

$(BIN): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LDLIBS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(BIN) $(OBJ)

install: $(BIN)
	install -m 755 $(BIN) $(DESTDIR)$(BINDIR)

uninstall:
	$(RM) $(addprefix $(DESTDIR)$(BINDIR)/,$(BIN))

.PHONY: clean install uninstall
