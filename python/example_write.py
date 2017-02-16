from zcs.column import Column, I64, I32, STR, LZ4, ZSTD
from zcs.writer import Writer

columns = [Column(I64, compression=LZ4),
           Column(STR, compression=ZSTD),
           Column(I32)]

rows = [(1400000000000, "foo@bar.com", 23),
        (1400000001000, "foo@bar.com", 45),
        (1400000002000, "baz@bar.com", 67)]

with Writer("example.zcs", columns, row_group_size=2) as writer:
    for row in rows:
        writer.put(row)
