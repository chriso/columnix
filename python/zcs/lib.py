import ctypes
import platform

shared_lib_ext = "dylib" if platform.uname()[0] == "Darwin" else "so"

libzcs = ctypes.cdll.LoadLibrary("libzcs.%s" % shared_lib_ext)

zcs_column_new = libzcs.zcs_column_new
zcs_column_new.argtypes = [ctypes.c_int, ctypes.c_int]
zcs_column_new.restype = ctypes.c_void_p

zcs_column_free = libzcs.zcs_column_free
zcs_column_free.argtypes = [ctypes.c_void_p]

zcs_column_put_bit = libzcs.zcs_column_put_bit
zcs_column_put_bit.argtypes = [ctypes.c_void_p, ctypes.c_bool]
zcs_column_put_bit.restype = ctypes.c_bool

zcs_column_put_i32 = libzcs.zcs_column_put_i32
zcs_column_put_i32.argtypes = [ctypes.c_void_p, ctypes.c_int32]
zcs_column_put_i32.restype = ctypes.c_bool

zcs_column_put_i64 = libzcs.zcs_column_put_i64
zcs_column_put_i64.argtypes = [ctypes.c_void_p, ctypes.c_int64]
zcs_column_put_i64.restype = ctypes.c_bool

zcs_column_put_str = libzcs.zcs_column_put_str
zcs_column_put_str.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
zcs_column_put_str.restype = ctypes.c_bool

zcs_row_group_new = libzcs.zcs_row_group_new
zcs_row_group_new.argtypes = []
zcs_row_group_new.restype = ctypes.c_void_p

zcs_row_group_free = libzcs.zcs_row_group_free
zcs_row_group_free.argtypes = [ctypes.c_void_p]

zcs_row_group_add_column = libzcs.zcs_row_group_add_column
zcs_row_group_add_column.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
zcs_row_group_add_column.restype = ctypes.c_bool

zcs_row_group_writer_new = libzcs.zcs_row_group_writer_new
zcs_row_group_writer_new.argtypes = [ctypes.c_char_p]
zcs_row_group_writer_new.restype = ctypes.c_void_p

zcs_row_group_writer_free = libzcs.zcs_row_group_writer_free
zcs_row_group_writer_free.argtypes = [ctypes.c_void_p]

zcs_row_group_writer_add_column = libzcs.zcs_row_group_writer_add_column
zcs_row_group_writer_add_column.argtypes = [ctypes.c_void_p, ctypes.c_int,
                                            ctypes.c_int, ctypes.c_int,
                                            ctypes.c_int]
zcs_row_group_writer_add_column.restype = ctypes.c_bool

zcs_row_group_writer_put = libzcs.zcs_row_group_writer_put
zcs_row_group_writer_put.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
zcs_row_group_writer_put.restype = ctypes.c_bool

zcs_row_group_writer_finish = libzcs.zcs_row_group_writer_finish
zcs_row_group_writer_finish.argtypes = [ctypes.c_void_p, ctypes.c_bool]
zcs_row_group_writer_finish.restype = ctypes.c_bool
