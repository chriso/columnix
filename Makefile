LIBNAME = libzcs

CFLAGS += -std=c99 -Wall -pedantic -Iinclude -Itest
VENDOR_LIBS = -llz4 -lzstd

PREFIX ?= /usr/local
INCLUDEDIR = $(PREFIX)/include/zcs

ifeq ($(release), 1)
  CFLAGS += -O3 -march=native -DNDEBUG
else
  CFLAGS += -g
endif

ifeq ($(avx2), 1)
  CFLAGS += -DZCS_AVX2 -mavx2
endif

ifeq ($(pcmpistrm), 1)
  CFLAGS += -DZCS_PCMPISTRM -msse4.2
endif

ifeq ($(asan), 1)
  CFLAGS += -fsanitize=address
  LDFLAGS += -fsanitize=address
endif

ifeq ($(usan), 1)
  CFLAGS += -fsanitize=undefined
  LDFLAGS += -fsanitize=undefined
endif

ifeq ($(coverage), 1)
  CFLAGS += -coverage
  LDFLAGS += -coverage
endif

ifeq ($(shell uname -s), Darwin)
  LIB = lib/$(LIBNAME).dylib
else
  CFLAGS += -fPIC
  LIB = lib/$(LIBNAME).so
endif

SRC = $(wildcard lib/*.c)
OBJ = $(SRC:.c=.o)
HEADERS = $(wildcard include/*.h)

TEST_SRC = $(wildcard test/*.c)
TEST_OBJ = $(TEST_SRC:.c=.o)

TESTS = test/runner

$(LIB): $(OBJ)
	$(CC) $(LDFLAGS) -shared -o $@ $^ $(VENDOR_LIBS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

$(TESTS): $(TEST_OBJ) $(LIB)
	$(CC) $(LDFLAGS) -o $@ $^

analyze: clean
	CFLAGS=--analyze $(MAKE) $(OBJ) $(TEST_OBJ)

check: $(TESTS)
	@./$(TESTS) $(grep)

clean:
	rm -f lib/$(LIBNAME).* $(OBJ) $(TESTS) $(TEST_OBJ) \
	    {lib,test}/*.{gcno,gcda} *.gcov *.html

coverage: clean
	$(MAKE) check coverage=1
	gcovr -r . --gcov-exclude=test --branch --html --html-details -o coverage.html
	open coverage.html

format:
	clang-format -i */*.{c,h}

install: $(LIB)
	install $(LIB) $(PREFIX)/$(LIB)
	install -d $(INCLUDEDIR)
	install -m 644 $(HEADERS) $(INCLUDEDIR)

uninstall:
	rm -f $(PREFIX)/$(LIB) $(INCLUDEDIR)/*.h
	rmdir $(INCLUDEDIR)

valgrind: $(TESTS)
	valgrind --leak-check=full $(TESTS) --no-fork

.PHONY: analyze check clean format install uninstall valgrind
