import ctypes
import platform

shared_lib_ext = "dylib" if platform.uname()[0] == "Darwin" else "so"

libzcs = ctypes.cdll.LoadLibrary("libzcs.%s" % shared_lib_ext)

zcs_writer_new = libzcs.zcs_writer_new
zcs_writer_new.argtypes = [ctypes.c_char_p, ctypes.c_size_t]
zcs_writer_new.restype = ctypes.c_void_p

zcs_writer_free = libzcs.zcs_writer_free
zcs_writer_free.argtypes = [ctypes.c_void_p]

zcs_writer_add_column = libzcs.zcs_writer_add_column
zcs_writer_add_column.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_int,
                                  ctypes.c_int, ctypes.c_int]
zcs_writer_add_column.restype = ctypes.c_bool

zcs_writer_put_bit = libzcs.zcs_writer_put_bit
zcs_writer_put_bit.argtypes = [ctypes.c_void_p, ctypes.c_bool]
zcs_writer_put_bit.restype = ctypes.c_bool

zcs_writer_put_i32 = libzcs.zcs_writer_put_i32
zcs_writer_put_i32.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int32]
zcs_writer_put_i32.restype = ctypes.c_bool

zcs_writer_put_i64 = libzcs.zcs_writer_put_i64
zcs_writer_put_i64.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_int64]
zcs_writer_put_i64.restype = ctypes.c_bool

zcs_writer_put_str = libzcs.zcs_writer_put_str
zcs_writer_put_str.argtypes = [ctypes.c_void_p, ctypes.c_size_t,
                               ctypes.c_char_p]
zcs_writer_put_str.restype = ctypes.c_bool

zcs_writer_finish = libzcs.zcs_writer_finish
zcs_writer_finish.argtypes = [ctypes.c_void_p, ctypes.c_bool]
zcs_writer_finish.restype = ctypes.c_bool
