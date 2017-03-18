.PHONY: lib
lib:
	$(MAKE) -C lib $@

.PHONY: test
test: lib
	$(MAKE) -C test $@

.PHONY: clean
clean:
	$(MAKE) -C lib clean
	$(MAKE) -C bin clean
	$(MAKE) -C test clean
	rm -f coverage*.html */*.gcda */*.gcno */*.gcov

.PHONY: install
install:
	$(MAKE) -C lib $@
	$(MAKE) -C bin $@

.PHONY: uninstall
uninstall:
	$(MAKE) -C bin $@
	$(MAKE) -C lib $@

.PHONY: valgrind
valgrind: lib
	$(MAKE) -C test valgrind

.PHONY: analyze
analyze:
	$(MAKE) -C lib clean
	CFLAGS="--analyze $(CFLAGS)" $(MAKE) -C lib compile

.PHONY: sanitize
sanitize: FLAGS = -fsanitize=undefined -fsanitize=address
sanitize: test_with_flags

.PHONY: coverage
coverage: FLAGS = -coverage
coverage: test_with_flags
	gcovr -r . --exclude=test --branch --html --html-details -o coverage.html
	open coverage.html

.PHONY: test_with_flags
test_with_flags:
	$(MAKE) -C lib clean
	$(MAKE) -C test clean
	CFLAGS="$(CFLAGS) $(FLAGS)" LDFLAGS="$(LDFLAGS) $(FLAGS)" $(MAKE) test

.PHONY: format
format:
	clang-format -i */*.{c,h}

.PHONY: check
check: test
