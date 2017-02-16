LIBNAME = libzcs

CFLAGS += -std=c99 -Wall -pedantic -Iinclude -Itest
VENDOR_LIBS = -llz4 -lzstd

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

ifeq ($(shell uname -s), Darwin)
  LIB = lib/$(LIBNAME).dylib
else
  CFLAGS += -fPIC
  LIB = lib/$(LIBNAME).so
endif

SRC = $(wildcard lib/*.c)
OBJ = $(SRC:.c=.o)

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
	rm -f $(LIB) $(OBJ) $(TESTS) $(TEST_OBJ)

format:
	clang-format -i */*.{c,h}

valgrind: $(TESTS)
	valgrind --leak-check=full $(TESTS) --no-fork

.PHONY: analyze check clean format valgrind
