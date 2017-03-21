PROJECT = columnix

# these are also set in version.h
VERSION_MAJOR = 1
VERSION_MINOR = 0
VERSION_RELEASE = 0
VERSION = $(VERSION_MAJOR).$(VERSION_MINOR).$(VERSION_RELEASE)

PREFIX ?= /usr/local
LIBDIR ?= $(PREFIX)/lib
INCLUDEDIR ?= $(PREFIX)/include
PKGCONFIGDIR ?= $(LIBDIR)/pkgconfig

LDLIBS = -llz4 -lzstd

CFLAGS += -std=c99 -g -pedantic -Wall -pthread -fvisibility=hidden
LDFLAGS += -fvisibility=hidden

OPTFLAGS ?= -O3 -march=native

SRC = column.c compress.c index.c match.c predicate.c \
      reader.c row.c row_group.c writer.c

HEADERS = column.h common.h compress.h file.h index.h \
	  predicate.h reader.h row.h row_group.h version.h writer.h

ifeq ($(java), 1)
  JAVA_HOME := $(shell /usr/libexec/java_home)
  JAVA_OS := $(shell uname | tr A-Z a-z)
  CFLAGS += -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/$(JAVA_OS)
  SRC += java.c
endif

LIBNAME = lib$(PROJECT)

ifeq ($(shell uname), Darwin)
  LIBEXT = dylib
  SHARED_LIB_VERSION = $(LIBNAME).$(VERSION).$(LIBEXT)
  SHARED_LIB_MAJOR = $(LIBNAME).$(VERSION_MAJOR).$(LIBEXT)
  SHARED_LIB = $(LIBNAME).$(LIBEXT)
  SONAME_FLAGS = -install_name $(LIBDIR)/$(SHARED_LIB_MAJOR) \
		 -compatibility_version $(VERSION_MAJOR) \
		 -current_version $(VERSION)
else
  CFLAGS += -fPIC
  LDFLAGS += -pthread
  LIBEXT = so
  SHARED_LIB_VERSION = $(LIBNAME).$(LIBEXT).$(VERSION)
  SHARED_LIB_MAJOR = $(LIBNAME).$(LIBEXT).$(VERSION_MAJOR)
  SONAME_FLAGS = -Wl,-soname=$(SHARED_LIB_VERSION)
endif

SHARED_LIB = $(LIBNAME).$(LIBEXT)
STATIC_LIB = $(LIBNAME).a

PKGCONFIG_FILE = $(LIBNAME).pc

OBJ := $(SRC:.c=.o)

ifneq ($(debug), 1)
  CFLAGS += $(OPTFLAGS)
  CPU_FEATURES := $(shell ../contrib/cpu-features.sh)
  ifneq (,$(findstring AVX512F,$(CPU_FEATURES)))
    CFLAGS += -DCX_AVX512 -mavx512f
  else ifneq (,$(findstring AVX2,$(CPU_FEATURES)))
    CFLAGS += -DCX_AVX2 -mavx2
  else ifneq (,$(findstring AVX,$(CPU_FEATURES)))
    CFLAGS += -DCX_AVX -mavx
  endif
  ifneq (,$(findstring SSE4_2,$(CPU_FEATURES)))
    CFLAGS += -DCX_SSE42 -msse4.2
  endif
endif

lib: $(SHARED_LIB_VERSION) $(STATIC_LIB)

$(STATIC_LIB): $(OBJ)
	$(AR) rcs $@ $^

$(SHARED_LIB_VERSION): $(OBJ)
	$(CC) $^ $(LDFLAGS) $(LDLIBS) $(SONAME_FLAGS) -shared -o $@

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $<

$(PKGCONFIG_FILE): $(PKGCONFIG_FILE).in
	sed -e 's|@PREFIX@|$(PREFIX)|' \
            -e 's|@LIBDIR@|$(LIBDIR)|' \
            -e 's|@INCLUDEDIR@|$(INCLUDEDIR)|' \
            -e 's|@VERSION@|$(VERSION)|' \
            $< >$@

compile: $(OBJ)

clean:
	$(RM) $(OBJ) $(STATIC_LIB) $(LIBNAME).dylib $(LIBNAME).so.* $(PKGCONFIG_FILE)

install: lib $(PKGCONFIG_FILE)
	install -m 755 $(SHARED_LIB_VERSION) $(STATIC_LIB) $(LIBDIR)/
	ln -sf $(SHARED_LIB_VERSION) $(LIBDIR)/$(SHARED_LIB)
	ln -sf $(SHARED_LIB_VERSION) $(LIBDIR)/$(SHARED_LIB_MAJOR)
	install -d $(INCLUDEDIR)/$(PROJECT)/
	install -m 644 $(HEADERS) $(INCLUDEDIR)/$(PROJECT)/
	install -m 644 $(PKGCONFIG_FILE) $(PKGCONFIGDIR)/

uninstall:
	$(RM) $(addprefix $(LIBDIR)/,$(SHARED_LIB) $(SHARED_LIB_VERSION) $(SHARED_LIB_MAJOR) $(STATIC_LIB))
	$(RM) $(PKGCONFIGDIR)/$(PKGCONFIG_FILE)
	$(RM) $(INCLUDEDIR)/$(PROJECT)/*.h
	rmdir $(INCLUDEDIR)/$(PROJECT)

.PHONY: compile clean install uninstall
