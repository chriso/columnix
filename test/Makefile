CFLAGS += -std=c99 -I../lib -g -pedantic -Wall

ifneq ($(shell uname -s), Darwin)
  LDFLAGS += -pthread
endif

# link with the static lib. This allows us to test parts of
# the library that are hidden via -fvisibility=hidden
LDLIBS = ../lib/libcolumnix.a -llz4 -lzstd

SRC := $(wildcard *.c)
OBJ := $(SRC:.c=.o)

RUNNER = runner

test: $(RUNNER)
	./$(RUNNER) $(grep)

$(RUNNER): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(RUNNER) $(OBJ)

valgrind: $(RUNNER)
	valgrind --leak-check=full ./$(RUNNER) --no-fork

.PHONY: test clean valgrind
