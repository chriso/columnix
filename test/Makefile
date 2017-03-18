CFLAGS += -std=c99 -I../lib -g -pedantic -Wall
LDFLAGS += -L../lib
LDLIBS = -lcolumnix

SRC := $(wildcard *.c)
OBJ := $(SRC:.c=.o)

RUNNER = runner

test: $(RUNNER)
	@./$(RUNNER) $(grep)

$(RUNNER): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(RUNNER) $(OBJ)

valgrind: $(RUNNER)
	LD_LIBRARY_PATH=../lib valgrind --leak-check=full ./$(RUNNER) --no-fork

.PHONY: test clean valgrind
