PROJECT = columnix

DESTDIR ?=
PREFIX  ?= /usr/local
LIBDIR ?= $(PREFIX)/lib
INCLUDEDIR = $(PREFIX)/include

LDLIBS = -llz4 -lzstd

CFLAGS += -std=c99 -g -pedantic -Wall -pthread

OPTFLAGS ?= -O3 -march=native

SRC = column.c compress.c index.c match.c predicate.c reader.c row.c row_group.c writer.c

ifeq ($(java), 1)
  JAVA_HOME := $(shell /usr/libexec/java_home)
  JAVA_OS := $(shell uname -s | tr A-Z a-z)
  CFLAGS += -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/$(JAVA_OS)
  SRC += java.c
endif

ifeq ($(shell uname -s), Darwin)
  LIBEXT = dylib
else
  CFLAGS += -fPIC
  LDFLAGS += -pthread
  LIBEXT = so
endif

LIB = lib$(PROJECT).$(LIBEXT)

OBJ := $(SRC:.c=.o)

ifneq ($(debug), 1)
  CFLAGS += $(OPTFLAGS)
  CPU_FEATURES := $(shell ../contrib/cpu-features.sh)
  ifneq (,$(findstring AVX512F,$(CPU_FEATURES)))
    CFLAGS += -DCX_AVX512 -mavx512f
  else ifneq (,$(findstring AVX2,$(CPU_FEATURES)))
    CFLAGS += -DCX_AVX2 -mavx2
  else ifneq (,$(findstring AVX,$(CPU_FEATURES)))
    CFLAGS += -DCX_AVX -mavx
  endif
  ifneq (,$(findstring SSE4_2,$(CPU_FEATURES)))
    CFLAGS += -DCX_SSE42 -msse4.2
  endif
endif

lib: $(LIB)

$(LIB): $(OBJ)
	$(CC) $(LDFLAGS) $(EXTFLAGS) -shared -o $@ $^ $(LDLIBS)

.c.o:
	$(CC) $(CFLAGS) $(EXTFLAGS) -c -o $@ $<

compile: $(OBJ)

clean:
	rm -f *.o *.so *.dylib *.gcno *.gcda *.gcov

install: $(LIB)
	install -m 755 $(LIB) $(DESTDIR)$(LIBDIR)
	install -d $(DESTDIR)$(INCLUDEDIR)/$(PROJECT)
	install -m 644 $(wildcard *.h) $(DESTDIR)$(INCLUDEDIR)/$(PROJECT)

uninstall:
	rm -f $(DESTDIR)$(LIBDIR)/$(LIB) $(DESTDIR)$(INCLUDEDIR)/$(PROJECT)/*.h
	rmdir $(DESTDIR)$(INCLUDEDIR)/$(PROJECT)

.PHONY: compile clean install uninstall
