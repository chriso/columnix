from zcs.lib import *
from zcs.column import BIT, I32, I64, STR


class Writer(object):

    def __init__(self, path, columns, row_group_size=100000, sync=True):
        self.path = path
        self.columns = columns
        self.column_types = [column.type for column in columns]
        self.row_group_size = row_group_size
        self.sync = sync

        self.writer = None
        self.row_group_columns = None
        self.row_count = 0

    def __enter__(self):
        assert self.writer is None
        self.writer = zcs_row_group_writer_new(self.path)
        if not self.writer:
            raise RuntimeError("failed to create writer for %s" % self.path)

        for column in self.columns:
            if not zcs_row_group_writer_add_column(self.writer, column.type,
                                                   column.encoding,
                                                   column.compression,
                                                   column.level):
                raise RuntimeError("failed to add column")
        return self

    def __exit__(self, err, value, traceback):
        assert self.writer is not None
        if not err:
            self._flush_row_group_columns()
            zcs_row_group_writer_finish(self.writer, self.sync)
        zcs_row_group_writer_free(self.writer)
        self.writer = None

    def put(self, row):
        if self.row_group_columns is None:
            assert self.writer is not None
            self.row_group_columns = \
                [zcs_column_new(column.type, column.encoding)
                 for column in self.columns]
            if any(column is None for column in self.row_group_columns):
                raise RuntimeError("failed to create column")

        for i, value in enumerate(row):
            column_type = self.column_types[i]
            row_group_column = self.row_group_columns[i]
            if column_type == BIT:
                ok = zcs_column_put_bit(row_group_column, value)
            elif column_type == I32:
                ok = zcs_column_put_i32(row_group_column, value)
            elif column_type == I64:
                ok = zcs_column_put_i64(row_group_column, value)
            elif column_type == STR:
                ok = zcs_column_put_str(row_group_column, value)
            if not ok:
                raise RuntimeError("failed to put value %r for column %i" %
                                   (value, i))

        self.row_count += 1
        if self.row_count == self.row_group_size:
            self._flush_row_group_columns()

    def _flush_row_group_columns(self):
        assert self.writer is not None
        if self.row_group_columns is None:
            return
        row_group = zcs_row_group_new()
        if not row_group:
            raise MemoryError
        for column in self.row_group_columns:
            if not zcs_row_group_add_column(row_group, column):
                raise RuntimeError("failed to add row group column")
        if not zcs_row_group_writer_put(self.writer, row_group):
            raise RuntimeError("failed to put row group")
        self.row_count = 0
        for column in self.row_group_columns:
            zcs_column_free(column)
        self.row_group_columns = None
        zcs_row_group_free(row_group)
