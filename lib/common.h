#define ZCS_NO_INLINE __attribute__((noinline))

#define ZCS_UNLIKELY(expr) __builtin_expect((expr), 0)
