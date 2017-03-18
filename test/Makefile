CFLAGS += -std=c99 -I../lib -g -pedantic -Wall
LDFLAGS += -L../lib
LDLIBS = -lcolumnix

SRC := $(wildcard *.c)
OBJ := $(SRC:.c=.o)

RUNNER = runner

ifeq ($(shell uname -s), Darwin)
  WITH_LIB = DYLD_LIBRARY_PATH=../lib
else
  WITH_LIB = LD_LIBRARY_PATH=../lib
endif

test: $(RUNNER)
	@$(WITH_LIB) ./$(RUNNER) $(grep)

$(RUNNER): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(RUNNER) $(OBJ)

valgrind: $(RUNNER)
	$(WITH_LIB) valgrind --leak-check=full ./$(RUNNER) --no-fork

.PHONY: test clean valgrind
