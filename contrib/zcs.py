"""
Python bindings for libcx.

Example write (Python):

    from cx import Writer, Column, I64, I32, STR, LZ4, ZSTD

    columns = [Column(I64, "timestamp", compression=LZ4),
               Column(STR, "email", compression=ZSTD),
               Column(I32, "id")]

    rows = [(1400000000000, "foo@bar.com", 23),
            (1400000001000, "foo@bar.com", 45),
            (1400000002000, "baz@bar.com", 67)]

    with Writer("example.cx", columns, row_group_size=2) as writer:
        for row in rows:
            writer.put(row)

Example read (C):

    #define __STDC_FORMAT_MACROS
    #include <assert.h>
    #include <inttypes.h>
    #include <stdio.h>
    #include <cx/reader.h>

    int main()
    {
        struct cx_reader *reader = cx_reader_new("example.cx");
        assert(reader);
        int64_t timestamp;
        const struct cx_string *email;
        int32_t event;
        while (cx_reader_next(reader)) {
            assert(cx_reader_get_i64(reader, 0, &timestamp) &&
                   cx_reader_get_str(reader, 1, &email) &&
                   cx_reader_get_i32(reader, 2, &event));
            printf("{%" PRIi64 ", %s, %d}\n", timestamp, email->ptr, event);
        }
        assert(!cx_reader_error(reader));
        cx_reader_free(reader);
    }
"""

from ctypes import cdll, util
from ctypes import c_char_p, c_size_t, c_void_p, c_int, c_int32, c_int64, c_bool

libcx = cdll.LoadLibrary(util.find_library("cx"))

cx_writer_new = libcx.cx_writer_new
cx_writer_new.argtypes = [c_char_p, c_size_t]
cx_writer_new.restype = c_void_p

cx_writer_free = libcx.cx_writer_free
cx_writer_free.argtypes = [c_void_p]

cx_writer_add_column = libcx.cx_writer_add_column
cx_writer_add_column.argtypes = [c_void_p, c_char_p, c_int, c_int, c_int,
                                  c_int]

cx_writer_put_null = libcx.cx_writer_put_null
cx_writer_put_null.argtypes = [c_void_p, c_size_t]

cx_writer_put_bit = libcx.cx_writer_put_bit
cx_writer_put_bit.argtypes = [c_void_p, c_size_t, c_bool]

cx_writer_put_i32 = libcx.cx_writer_put_i32
cx_writer_put_i32.argtypes = [c_void_p, c_size_t, c_int32]

cx_writer_put_i64 = libcx.cx_writer_put_i64
cx_writer_put_i64.argtypes = [c_void_p, c_size_t, c_int64]

cx_writer_put_str = libcx.cx_writer_put_str
cx_writer_put_str.argtypes = [c_void_p, c_size_t, c_char_p]

cx_writer_finish = libcx.cx_writer_finish
cx_writer_finish.argtypes = [c_void_p, c_bool]

BIT = 0
I32 = 1
I64 = 2
STR = 3

LZ4 = 1
LZ4HC = 2
ZSTD = 3


class Column(object):
    def __init__(self, type, name, encoding=None, compression=None, level=1):
        self.type = type
        self.name = name
        self.encoding = encoding or 0
        self.compression = compression or 0
        self.level = level


class Writer(object):
    def __init__(self, path, columns, row_group_size=100000, sync=True):
        self.path = path
        self.columns = columns
        self.row_group_size = row_group_size
        self.sync = sync
        self.column_types = [column.type for column in columns]
        self.put_lookup = [self.put_bit, self.put_i32, self.put_i64,
                           self.put_str]
        self.writer = None

    def __enter__(self):
        assert self.writer is None
        self.writer = cx_writer_new(self.path, self.row_group_size)
        if not self.writer:
            raise RuntimeError("failed to create writer for %s" % self.path)
        for column in self.columns:
            if not cx_writer_add_column(self.writer, column.name, column.type,
                                         column.encoding, column.compression,
                                         column.level):
                raise RuntimeError("failed to add column")
        return self

    def __exit__(self, err, value, traceback):
        assert self.writer is not None
        if not err:
            cx_writer_finish(self.writer, self.sync)
        cx_writer_free(self.writer)
        self.writer = None

    def put(self, row):
        put_lookup = self.put_lookup
        column_types = self.column_types
        for column, value in enumerate(row):
            if value is None:
                self.put_null(column)
            else:
                put_lookup[column_types[column]](column, value)

    def put_null(self, column):
        assert self.writer is not None
        if not cx_writer_put_null(self.writer, column):
            raise RuntimeError("put_null(%d)" % column)

    def put_bit(self, column, value):
        assert self.writer is not None
        if not cx_writer_put_bit(self.writer, column, value):
            raise RuntimeError("put_bit(%d, %r)" % (column, value))

    def put_i32(self, column, value):
        assert self.writer is not None
        if not cx_writer_put_i32(self.writer, column, value):
            raise RuntimeError("put_i32(%d, %r)" % (column, value))

    def put_i64(self, column, value):
        assert self.writer is not None
        if not cx_writer_put_i64(self.writer, column, value):
            raise RuntimeError("put_i64(%d, %r)" % (column, value))

    def put_str(self, column, value):
        assert self.writer is not None
        if not cx_writer_put_str(self.writer, column, value):
            raise RuntimeError("put_str(%d, %r)" % (column, value))
