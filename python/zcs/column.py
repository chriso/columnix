BIT = 0
I32 = 1
I64 = 2
STR = 3

NONE = 0

LZ4 = 1
LZ4HC = 2
ZSTD = 3


class Column(object):

    def __init__(self, type, encoding=NONE, compression=NONE, level=1):
        self.type = type
        self.encoding = encoding
        self.compression = compression
        self.level = level
