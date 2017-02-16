BIT = 0
I32 = 1
I64 = 2
STR = 3

NONE = 0

LZ4 = 1
ZSTD = 2


class Column(object):

    def __init__(self, type, encoding=NONE, compression=NONE, level=1):
        self.type = type
        self.encoding = encoding
        self.compression = compression
        self.level = level
