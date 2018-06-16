# Columnix

Columnix is a columnar storage format, similar to [Parquet][parquet] and [ORC][orc].

The experiment was to beat Parquet's read performance in [Spark][spark] for flat schemas,
while simultaneously reducing the disk footprint by utilizing newer compression
algorithms such as [lz4][lz4] and [zstd][zstd].

Columnix supports:
1. row groups
2. indexes (at the row group level, and file level)
3. vectorized reads
4. predicate pushdown
5. lazy reads
6. AVX2 and AVX512 predicate matching
7. memory-mapped IO

Spark's Parquet reader supports 1-4, but has no support for lazy reads, only
limited SIMD support (whatever the JVM provides) and IO is through HDFS.

Support for complex schemas was not a goal of the project. The format has no support for
Parquet's [Dremel-style][dremel-style] definition & repetition levels or ORC's
[compound types][orc-types] (struct, list, map, union).

The library does not currently support encoding of data prior to (or instead of) compression,
for example run-length or dict encoding, despite placeholders in the code alluding to it. It was
next on the TODO list, but I'd like to explore alternative approaches such as
[github.com/chriso/treecomp](https://github.com/chriso/treecomp).

The following bindings are provided:
- Python (ctypes): [./contrib/columnix.py][py-bindings]
- Spark (JNI): [chriso/columnix-spark][spark-bindings]

One major caveat: the library uses `mmap` for reads. There is no HDFS compatibility and
so there is limited real world use for the time being.


[parquet]: https://parquet.apache.org
[orc]: https://orc.apache.org
[lz4]: https://lz4.github.io/lz4/
[zstd]: http://facebook.github.io/zstd/
[spark]: https://spark.apache.org
[dremel-style]: https://blog.twitter.com/engineering/en_us/a/2013/dremel-made-simple-with-parquet.html
[orc-types]: https://orc.apache.org/docs/types.html
[py-bindings]: https://github.com/chriso/columnix/blob/master/contrib/columnix.py
[spark-bindings]: https://github.com/chriso/columnix-spark
