LDLIBS = -llz4 -lzstd

BASE_CFLAGS = -std=c99 -Wall -pedantic -Iinclude -g
CFLAGS += $(BASE_CFLAGS) -pthread

PREFIX ?= /usr/local
INCLUDEDIR ?= $(PREFIX)/include

SRC_FILES = column.c compress.c match.c predicate.c reader.c row.c row_group.c writer.c

#ifeq ($(java), 1)
  JAVA_HOME := $(shell /usr/libexec/java_home)
  JAVA_OS := $(shell uname -s | tr A-Z a-z)
  CFLAGS += -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/$(JAVA_OS)
  SRC_FILES += java.c
#endif

ifeq ($(release), 1)
  CFLAGS += -O3 -march=native -DZCS_AVX2
endif
ifeq ($(pcmpistrm), 1)
  CFLAGS += -DZCS_PCMPISTRM
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
  LIB = lib/libzcs.dylib
else
  CFLAGS += -fPIC
  LDFLAGS += -pthread
  LIB = lib/libzcs.so
endif

SRC := $(addprefix lib/,$(SRC_FILES))
OBJ := $(SRC:.c=.o)

TEST_SRC := $(wildcard test/*.c)
TEST_OBJ := $(TEST_SRC:.c=.o)

TESTS = test/runner

$(LIB): $(OBJ)
	$(CC) $(LDFLAGS) $(EXTFLAGS) -shared -o $@ $^ $(LDLIBS)

$(OBJ): CFLAGS += $(EXTFLAGS)
$(TEST_OBJ): CFLAGS = $(BASE_CFLAGS) -Itest

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

$(TESTS): $(TEST_OBJ) $(LIB)
	$(CC) $(LDFLAGS) -o $@ $^

analyze: clean
	CC="$(CC) --analyze" $(MAKE) $(OBJ) $(TEST_OBJ)

check: $(TESTS)
	@./$(TESTS) $(grep)

clean:
	rm -f $(TESTS) lib/libzcs.* */*.o lib/*.gcno lib/*.gcda *.gcov coverage*.html

coverage: clean
	$(MAKE) check coverage=1
	gcovr -r . --branch --html --html-details -o coverage.html
	open coverage.html

format:
	clang-format -i */*.{c,h}

install: $(LIB)
	install -m 755 $(LIB) $(PREFIX)/$(LIB)
	install -d $(INCLUDEDIR)/zcs
	install -m 644 $(wildcard include/*.h) $(INCLUDEDIR)/zcs

uninstall:
	rm -f $(PREFIX)/$(LIB) $(INCLUDEDIR)/zcs/*.h
	rmdir $(INCLUDEDIR)/zcs

valgrind: $(TESTS)
	valgrind --leak-check=full $(TESTS) --no-fork

.PHONY: analyze check clean coverage format install uninstall valgrind
