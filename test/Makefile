CFLAGS += -std=c99 -I../lib -g -pedantic -Wall
LDFLAGS += -L../lib
LDLIBS = -lcolumnix

SRC := $(wildcard *.c)
OBJ := $(SRC:.c=.o)

RUNNER = runner

test: $(RUNNER)
	@./$(RUNNER) $(grep)

$(RUNNER): $(OBJ)
	$(CC) $(LDFLAGS) $(EXTFLAGS) -o $@ $^ $(LDLIBS)

.c.o:
	$(CC) $(CFLAGS) $(EXTFLAGS) -c -o $@ $<

clean:
	rm -f $(RUNNER) *.o *.gcno *.gcda *.gcov

valgrind: $(RUNNER)
	valgrind --leak-check=full ./$(RUNNER) --no-fork

.PHONY: test clean valgrind
