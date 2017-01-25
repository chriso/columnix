CFLAGS = -std=c11 -Wall -pedantic -Iinclude -Itest

SRC = $(wildcard lib/*.c)
OBJ = $(SRC:.c=.o)

TEST_SRC = $(wildcard test/*.c)
TEST_OBJ = $(TEST_SRC:.c=.o)

LIB = lib/libzcs.dylib

TESTS = test/runner

$(LIB): $(OBJ)
	$(CC) $(LDFLAGS) -shared -o $@ $^

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

$(TESTS): $(LIB) $(TEST_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

check: $(TESTS)
	@./$(TESTS) $(grep)

clean:
	rm -f $(LIB) $(OBJ) $(TESTS) $(TEST_OBJ)

format:
	clang-format -i */*.{c,h}

.PHONY: check clean format
