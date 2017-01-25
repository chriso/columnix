CFLAGS = -std=c11 -Wall -pedantic -Iinclude

SRC = $(wildcard lib/*.c)
OBJ = $(SRC:.c=.o)

TEST_SRC = $(wildcard test/*.c)
TEST_OBJ = $(TEST_SRC:.c=.o)

LIB = libzcs.dylib

TESTS = tests

$(LIB): $(OBJ)
	$(CC) $(LDFLAGS) -shared -o $@ $^

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

$(TESTS): $(LIB) $(TEST_OBJ)
	$(CC) $(LDFLAGS) -o $@ $^

check: $(TESTS)
	@./$(TESTS)

clean:
	rm -f $(LIB) $(OBJ) $(TESTS) $(TEST_OBJ)

.PHONY: check clean
