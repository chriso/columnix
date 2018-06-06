# Columnix

Columnix is a columnar storage format, similar to [Parquet][parquet] and [ORC][orc].

The goal was to beat Parquet on read performance in [Spark][spark] for flat schemas,
while simultaneously reducing the disk footprint by utilizing newer compression
algorithms such as [lz4][lz4] and [zstd][zstd].

Columnix supports:
1. vectorized reads
2. predicate pushdown
3. lazy reads
4. AVX2 and AVX512 predicate matching implementations
5. memory-mapped IO

Spark's Parquet reader supports 1 and 2, but has no support for lazy reads, only
limited SIMD support, and IO is through a HDFS layer.

Support for complex schemas was not a goal of the project. The format has no support for
Parquet's [Dremel-style][dremel-style] definition & repetition levels, or ORC's
[compound types][orc-types] (struct, list, map, union).

The library does not currently support encoding of data prior to compression (for example
run-length or dict encoding), despite placeholders in the code alluding to it. I do not
intend to complete this part of the library.

Number 5 is the primary reason I won't be putting in any further development work. The library
uses memory-mapped IO only; local reads are fast, but there is no HDFS compatibility and so the
format has limited real-world use outside of benchmarks and trivial workloads.

The following bindings are provided:
- Python (ctypes): [./contrib/columnix.py][py-bindings]
- Spark (JNI): [chriso/columnix-spark][spark-bindings]


[parquet]: https://parquet.apache.org
[orc]: https://orc.apache.org
[lz4]: https://lz4.github.io/lz4/
[zstd]: http://facebook.github.io/zstd/
[spark]: https://spark.apache.org
[dremel-style]: https://blog.twitter.com/engineering/en_us/a/2013/dremel-made-simple-with-parquet.html
[orc-types]: https://orc.apache.org/docs/types.html
[py-bindings]: https://github.com/chriso/columnix/blob/master/contrib/columnix.py
[spark-bindings]: https://github.com/chriso/columnix-spark
