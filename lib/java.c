#include "java.h"
#include "reader.h"
#include "writer.h"

static void zcs_java_throw(JNIEnv *env, const char *class, const char *msg)
{
    jclass exception = (*env)->FindClass(env, class);
    (*env)->ThrowNew(env, exception, msg);
}

static void zcs_java_npe(JNIEnv *env, const char *msg)
{
    zcs_java_throw(env, "java/lang/NullPointerException", msg);
}

static const char *zcs_java_string_new(JNIEnv *env, jstring string)
{
    return (*env)->GetStringUTFChars(env, string, 0);
}

static void zcs_java_string_free(JNIEnv *env, jstring string, const char *ptr)
{
    (*env)->ReleaseStringUTFChars(env, string, ptr);
}

static struct zcs_predicate *zcs_java_predicate_cast(JNIEnv *env, jlong ptr)
{
    struct zcs_predicate *predicate = (struct zcs_predicate *)ptr;
    if (!predicate)
        zcs_java_npe(env, "predicate ptr");
    return predicate;
}

jlong Java_zcs_jni_Reader_nativeNew(JNIEnv *env, jobject this,
                                    jstring java_path)
{
    const char *path = zcs_java_string_new(env, java_path);
    struct zcs_reader *reader = zcs_reader_new(path);
    zcs_java_string_free(env, java_path, path);
    if (!reader)
        goto error;
    return (jlong)reader;
error:
    zcs_java_throw(env, "java/lang/Exception", "zcs_reader_new()");
    return 0;
}

jlong Java_zcs_jni_Reader_nativeNewMatching(JNIEnv *env, jobject this,
                                            jstring java_path, jlong ptr)
{
    struct zcs_predicate *predicate = zcs_java_predicate_cast(env, ptr);
    if (!predicate)
        return 0;
    const char *path = zcs_java_string_new(env, java_path);
    struct zcs_reader *reader = zcs_reader_new_matching(path, predicate);
    zcs_java_string_free(env, java_path, path);
    if (!reader)
        goto error;
    return (jlong)reader;
error:
    zcs_java_throw(env, "java/lang/Exception", "zcs_reader_new_matching()");
    return 0;
}

static struct zcs_reader *zcs_java_reader_cast(JNIEnv *env, jlong ptr)
{
    struct zcs_reader *reader = (struct zcs_reader *)ptr;
    if (!reader)
        zcs_java_npe(env, "reader ptr");
    return reader;
}

void Java_zcs_jni_Reader_nativeFree(JNIEnv *env, jobject this, jlong ptr)
{
    struct zcs_reader *reader = zcs_java_reader_cast(env, ptr);
    if (!reader)
        return;
    zcs_reader_free(reader);
}

jint Java_zcs_jni_Reader_nativeColumnCount(JNIEnv *env, jobject this, jlong ptr)
{
    struct zcs_reader *reader = zcs_java_reader_cast(env, ptr);
    if (!reader)
        return 0;
    return zcs_reader_column_count(reader);
}

jlong Java_zcs_jni_Reader_nativeRowCount(JNIEnv *env, jobject this, jlong ptr)
{
    struct zcs_reader *reader = zcs_java_reader_cast(env, ptr);
    if (!reader)
        return 0;
    return zcs_reader_row_count(reader);
}

void Java_zcs_jni_Reader_nativeRewind(JNIEnv *env, jobject this, jlong ptr)
{
    struct zcs_reader *reader = zcs_java_reader_cast(env, ptr);
    if (!reader)
        return;
    zcs_reader_rewind(reader);
}

jboolean Java_zcs_jni_Reader_nativeNext(JNIEnv *env, jobject this, jlong ptr)
{
    struct zcs_reader *reader = zcs_java_reader_cast(env, ptr);
    if (!reader)
        return JNI_FALSE;
    if (!zcs_reader_next(reader)) {
        if (zcs_reader_error(reader))
            zcs_java_throw(env, "java/lang/Exception", "zcs_reader_error()");
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

static bool zcs_java_reader_check_column_bounds(JNIEnv *env,
                                                const struct zcs_reader *reader,
                                                jint index)
{
    if (index < 0 || index >= zcs_reader_column_count(reader)) {
        zcs_java_throw(env, "java/lang/IndexOutOfBoundsException",
                       "column index out of bounds");
        return false;
    }
    return true;
}

jint Java_zcs_jni_Reader_nativeColumnType(JNIEnv *env, jobject this, jlong ptr,
                                          jint index)
{
    struct zcs_reader *reader = zcs_java_reader_cast(env, ptr);
    if (!reader)
        return 0;
    if (!zcs_java_reader_check_column_bounds(env, reader, index))
        return 0;
    return zcs_reader_column_type(reader, index);
}

jint Java_zcs_jni_Reader_nativeColumnEncoding(JNIEnv *env, jobject this,
                                              jlong ptr, jint index)
{
    struct zcs_reader *reader = zcs_java_reader_cast(env, ptr);
    if (!reader)
        return 0;
    if (!zcs_java_reader_check_column_bounds(env, reader, index))
        return 0;
    return zcs_reader_column_encoding(reader, index);
}

jint Java_zcs_jni_Reader_nativeColumnCompression(JNIEnv *env, jobject this,
                                                 jlong ptr, jint index)
{
    struct zcs_reader *reader = zcs_java_reader_cast(env, ptr);
    if (!reader)
        return 0;
    if (!zcs_java_reader_check_column_bounds(env, reader, index))
        return 0;
    return zcs_reader_column_compression(reader, index);
}

jboolean Java_zcs_jni_Reader_nativeIsNull(JNIEnv *env, jobject this, jlong ptr,
                                          jint index)
{
    struct zcs_reader *reader = zcs_java_reader_cast(env, ptr);
    if (!reader)
        return 0;
    if (!zcs_java_reader_check_column_bounds(env, reader, index))
        return 0;
    bool value;
    if (!zcs_reader_get_null(reader, index, &value)) {
        zcs_java_throw(env, "java/lang/Exception", "zcs_reader_get_null()");
        return JNI_FALSE;
    }
    return (jboolean)value;
}

jboolean Java_zcs_jni_Reader_nativeGetBoolean(JNIEnv *env, jobject this,
                                              jlong ptr, jint index)
{
    struct zcs_reader *reader = zcs_java_reader_cast(env, ptr);
    if (!reader)
        return JNI_FALSE;
    if (!zcs_java_reader_check_column_bounds(env, reader, index))
        return JNI_FALSE;
    bool value;
    if (!zcs_reader_get_bit(reader, index, &value)) {
        zcs_java_throw(env, "java/lang/Exception", "zcs_reader_get_bit()");
        return JNI_FALSE;
    }
    return (jboolean)value;
}

jint Java_zcs_jni_Reader_nativeGetInt(JNIEnv *env, jobject this, jlong ptr,
                                      jint index)
{
    struct zcs_reader *reader = zcs_java_reader_cast(env, ptr);
    if (!reader)
        return 0;
    if (!zcs_java_reader_check_column_bounds(env, reader, index))
        return 0;
    int32_t value;
    if (!zcs_reader_get_i32(reader, index, &value)) {
        zcs_java_throw(env, "java/lang/Exception", "zcs_reader_get_i32()");
        return 0;
    }
    return (jint)value;
}

jlong Java_zcs_jni_Reader_nativeGetLong(JNIEnv *env, jobject this, jlong ptr,
                                        jint index)
{
    struct zcs_reader *reader = zcs_java_reader_cast(env, ptr);
    if (!reader)
        return 0;
    if (!zcs_java_reader_check_column_bounds(env, reader, index))
        return 0;
    int64_t value;
    if (!zcs_reader_get_i64(reader, index, &value)) {
        zcs_java_throw(env, "java/lang/Exception", "zcs_reader_get_i64()");
        return 0;
    }
    return (jlong)value;
}

jstring Java_zcs_jni_Reader_nativeGetString(JNIEnv *env, jobject this,
                                            jlong ptr, jint index)
{
    struct zcs_reader *reader = zcs_java_reader_cast(env, ptr);
    if (!reader)
        return NULL;
    if (!zcs_java_reader_check_column_bounds(env, reader, index))
        return NULL;
    const struct zcs_string *value;
    if (!zcs_reader_get_str(reader, index, &value)) {
        zcs_java_throw(env, "java/lang/Exception", "zcs_reader_get_str()");
        return NULL;
    }
    return (*env)->NewStringUTF(env, value->ptr);
}

jlong Java_zcs_jni_Writer_nativeNew(JNIEnv *, jobject, jstring, jlong);
void Java_zcs_jni_Writer_nativeFree(JNIEnv *, jobject, jlong);
void Java_zcs_jni_Writer_nativeFinish(JNIEnv *, jobject, jlong, jboolean);

jlong Java_zcs_jni_Writer_nativeNew(JNIEnv *env, jobject this,
                                    jstring java_path, jlong row_group_size)
{
    if (row_group_size <= 0) {
        zcs_java_throw(env, "java/lang/IllegalArgumentException",
                       "row group size <= 0");
        return 0;
    }
    const char *path = zcs_java_string_new(env, java_path);
    struct zcs_writer *writer = zcs_writer_new(path, row_group_size);
    zcs_java_string_free(env, java_path, path);
    if (!writer)
        goto error;
    return (jlong)writer;
error:
    zcs_java_throw(env, "java/lang/Exception", "zcs_writer_new()");
    return 0;
}

static struct zcs_writer *zcs_java_writer_cast(JNIEnv *env, jlong ptr)
{
    struct zcs_writer *writer = (struct zcs_writer *)ptr;
    if (!writer)
        zcs_java_npe(env, "writer ptr");
    return writer;
}

void Java_zcs_jni_Writer_nativeFree(JNIEnv *env, jobject this, jlong ptr)
{
    struct zcs_writer *writer = zcs_java_writer_cast(env, ptr);
    if (!writer)
        return;
    zcs_writer_free(writer);
}

void Java_zcs_jni_Writer_nativeFinish(JNIEnv *env, jobject this, jlong ptr,
                                      jboolean sync)
{
    struct zcs_writer *writer = zcs_java_writer_cast(env, ptr);
    if (!writer)
        return;
    if (!zcs_writer_finish(writer, sync))
        zcs_java_throw(env, "java/lang/Exception", "zcs_writer_finish()");
}

void Java_zcs_jni_Writer_nativeAddColumn(JNIEnv *env, jobject this, jlong ptr,
                                         jint type, jint encoding,
                                         jint compression, jint level)
{
    struct zcs_writer *writer = zcs_java_writer_cast(env, ptr);
    if (!writer)
        return;
    if (!zcs_writer_add_column(writer, type, encoding, compression, level))
        zcs_java_throw(env, "java/lang/Exception", "zcs_writer_add_column()");
}

void Java_zcs_jni_Writer_nativePutNull(JNIEnv *env, jobject this, jlong ptr,
                                       jint index)
{
    struct zcs_writer *writer = zcs_java_writer_cast(env, ptr);
    if (!writer)
        return;
    if (!zcs_writer_put_null(writer, index))
        zcs_java_throw(env, "java/lang/Exception", "zcs_writer_put_null()");
}

void Java_zcs_jni_Writer_nativePutBoolean(JNIEnv *env, jobject this, jlong ptr,
                                          jint index, jboolean value)
{
    struct zcs_writer *writer = zcs_java_writer_cast(env, ptr);
    if (!writer)
        return;
    if (!zcs_writer_put_bit(writer, index, value))
        zcs_java_throw(env, "java/lang/Exception", "zcs_writer_put_bit()");
}

void Java_zcs_jni_Writer_nativePutInt(JNIEnv *env, jobject this, jlong ptr,
                                      jint index, jint value)
{
    struct zcs_writer *writer = zcs_java_writer_cast(env, ptr);
    if (!writer)
        return;
    if (!zcs_writer_put_i32(writer, index, value))
        zcs_java_throw(env, "java/lang/Exception", "zcs_writer_put_i32()");
}

void Java_zcs_jni_Writer_nativePutLong(JNIEnv *env, jobject this, jlong ptr,
                                       jint index, jlong value)
{
    struct zcs_writer *writer = zcs_java_writer_cast(env, ptr);
    if (!writer)
        return;
    if (!zcs_writer_put_i64(writer, index, value))
        zcs_java_throw(env, "java/lang/Exception", "zcs_writer_put_i64()");
}

void Java_zcs_jni_Writer_nativePutString(JNIEnv *env, jobject this, jlong ptr,
                                         jint index, jstring java_value)
{
    struct zcs_writer *writer = zcs_java_writer_cast(env, ptr);
    if (!writer)
        return;
    bool ok;
    if (!java_value) {
        ok = zcs_writer_put_null(writer, index);
    } else {
        const char *value = zcs_java_string_new(env, java_value);
        ok = zcs_writer_put_str(writer, index, value);
        zcs_java_string_free(env, java_value, value);
    }
    if (!ok)
        zcs_java_throw(env, "java/lang/Exception", "zcs_writer_put_str()");
}

static jlong zcs_java_predicate_ptr(JNIEnv *env,
                                    const struct zcs_predicate *ptr)
{
    if (!ptr)
        zcs_java_throw(env, "java/lang/OutOfMemoryError", "");
    return (jlong)ptr;
}

jlong Java_zcs_jni_predicates_Predicate_nativeNegate(JNIEnv *env, jobject this,
                                                     jlong ptr)
{
    struct zcs_predicate *predicate = zcs_java_predicate_cast(env, ptr);
    if (!predicate)
        return 0;
    return zcs_java_predicate_ptr(env, zcs_predicate_negate(predicate));
}

void Java_zcs_jni_predicates_Predicate_nativeFree(JNIEnv *env, jobject this,
                                                  jlong ptr)
{
    struct zcs_predicate *predicate = zcs_java_predicate_cast(env, ptr);
    if (!predicate)
        return;
    zcs_predicate_free(predicate);
}

jlong Java_zcs_jni_predicates_Equals_nativeLongEquals(JNIEnv *env, jobject this,
                                                      jint column, jlong value)
{
    return zcs_java_predicate_ptr(env, zcs_predicate_new_i64_eq(column, value));
}
