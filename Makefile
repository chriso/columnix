lib:
	$(MAKE) -C lib $@

test: lib
	$(MAKE) -C test $@

clean:
	$(MAKE) -C lib clean
	$(MAKE) -C bin clean
	$(MAKE) -C test clean

install:
	$(MAKE) -C lib $@
	$(MAKE) -C bin $@

uninstall:
	$(MAKE) -C bin $@
	$(MAKE) -C lib $@

valgrind: lib
	$(MAKE) -C test valgrind

analyze:
	$(MAKE) -C lib clean
	CFLAGS="--analyze $(CFLAGS)" $(MAKE) -C lib compile

sanitize:
	$(MAKE) -C lib clean
	$(MAKE) -C test clean
	EXTFLAGS="-fsanitize=undefined -fsanitize=address" $(MAKE) test

format:
	clang-format -i */*.{c,h}

check: test

.PHONY: lib test clean install uninstall valgrind analyze sanitize format check
