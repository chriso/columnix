PROJECT = columnix

LDLIBS = -llz4 -lzstd

BASE_CFLAGS = -std=c99 -Wall -pedantic -Iinclude -g
CFLAGS += $(BASE_CFLAGS) -pthread

PREFIX ?= /usr/local
INCLUDEDIR ?= $(PREFIX)/include
BINDIR ?= $(PREFIX)/bin

ifeq ($(debug), 1)
  CFLAGS += -Og
else
  CFLAGS += -O3 -march=native
  CPU_FEATURES := $(shell ./contrib/cpu-features.sh)
  ifneq (,$(findstring AVX2,$(CPU_FEATURES)))
    CFLAGS += -DCX_AVX2 -DCX_PCMPISTRM
  else ifneq (,$(findstring AVX,$(CPU_FEATURES)))
    CFLAGS += -DCX_AVX -DCX_PCMPISTRM
  else ifneq (,$(findstring SSE4_2,$(CPU_FEATURES)))
    CFLAGS += -DCX_PCMPISTRM
  endif
endif

ifeq ($(asan), 1)
  EXTFLAGS += -fsanitize=address
endif
ifeq ($(usan), 1)
  EXTFLAGS += -fsanitize=undefined
endif
ifeq ($(coverage), 1)
  EXTFLAGS += -coverage
endif

ifeq ($(shell uname -s), Darwin)
  LIB = lib/lib$(PROJECT).dylib
else
  CFLAGS += -fPIC
  LDFLAGS += -pthread
  LIB = lib/lib$(PROJECT).so
endif

LIB_SRC = $(wildcard lib/*.c)
LIB_OBJ = $(LIB_SRC:.c=.o)

BIN_SRC = $(wildcard bin/*.c)
BIN_OBJ = $(BIN_SRC:.c=.o)
BIN = $(basename $(BIN_SRC))

TEST_SRC = $(wildcard test/*.c)
TEST_OBJ = $(TEST_SRC:.c=.o)

TESTS = test/runner

$(LIB): $(LIB_OBJ)
	$(CC) $(LDFLAGS) $(EXTFLAGS) -shared -o $@ $^ $(LDLIBS)

$(LIB_OBJ): CFLAGS += $(EXTFLAGS)
$(BIN_OBJ): CFLAGS += $(EXTFLAGS)
$(TEST_OBJ): CFLAGS = $(BASE_CFLAGS) -Itest

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN): $(BIN_OBJ) $(LIB)
	$(CC) $(LDFLAGS) -o $@ $@.o $(LIB)

$(TESTS): $(TEST_OBJ) $(LIB)
	$(CC) $(LDFLAGS) -o $@ $^

analyze: clean
	CC="$(CC) --analyze" $(MAKE) $(LIB_OBJ) $(BIN_OBJ) $(TEST_OBJ)

check: $(TESTS)
	@./$(TESTS) $(grep)

clean:
	rm -f $(TESTS) lib/lib$(PROJECT).* */*.o lib/*.gcno lib/*.gcda *.gcov coverage*.html

coverage: clean
	$(MAKE) check coverage=1
	gcovr -r . --branch --html --html-details -o coverage.html
	open coverage.html

format:
	clang-format -i */*.{c,h}

install: $(LIB) $(BIN)
	install -m 755 $(LIB) $(PREFIX)/$(LIB)
	install -d $(INCLUDEDIR)/$(PROJECT)
	install -m 644 $(wildcard include/*.h) $(INCLUDEDIR)/$(PROJECT)
	install -m 755 $(BIN) $(BINDIR)

uninstall:
	rm -f $(PREFIX)/$(LIB) $(INCLUDEDIR)/$(PROJECT)/*.h
	rmdir $(INCLUDEDIR)/$(PROJECT)

valgrind: $(TESTS)
	valgrind --leak-check=full $(TESTS) --no-fork

.PHONY: analyze check clean coverage format install uninstall valgrind
