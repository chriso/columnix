from zcs.lib import *
from zcs.column import BIT, I32, I64, STR


class Writer(object):

    def __init__(self, path, columns, row_group_size=100000, sync=True):
        self.path = path
        self.columns = columns
        self.row_group_size = row_group_size
        self.sync = sync
        self.column_types = [column.type for column in columns]
        self.writer = None

        self.put_by_type = [self.put_bit, self.put_i32, self.put_i64,
                            self.put_str]

    def __enter__(self):
        assert self.writer is None
        self.writer = zcs_writer_new(self.path, self.row_group_size)
        if not self.writer:
            raise RuntimeError("failed to create writer for %s" % self.path)

        for column in self.columns:
            if not zcs_writer_add_column(self.writer, column.type,
                                         column.encoding, column.compression,
                                         column.level):
                raise RuntimeError("failed to add column")
        return self

    def __exit__(self, err, value, traceback):
        assert self.writer is not None
        if not err:
            zcs_writer_finish(self.writer, self.sync)
        zcs_writer_free(self.writer)
        self.writer = None

    def put(self, row):
        put_by_type = self.put_by_type
        column_types = self.column_types
        for i, value in enumerate(row):
            put_by_type[column_types[i]](i, value)

    def put_bit(self, column, value):
        assert self.writer is not None
        if not zcs_writer_put_bit(self.writer, column, value):
            raise RuntimeError("put_bit(%d, %r)" % (column, value))

    def put_i32(self, column, value):
        assert self.writer is not None
        if not zcs_writer_put_i32(self.writer, column, value):
            raise RuntimeError("put_i32(%d, %r)" % (column, value))

    def put_i64(self, column, value):
        assert self.writer is not None
        if not zcs_writer_put_i64(self.writer, column, value):
            raise RuntimeError("put_i64(%d, %r)" % (column, value))

    def put_str(self, column, value):
        assert self.writer is not None
        if not zcs_writer_put_str(self.writer, column, value):
            raise RuntimeError("put_str(%d, %r)" % (column, value))
