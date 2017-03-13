#include <assert.h>
#include <string.h>

#include "java.h"
#include "reader.h"
#include "writer.h"

static void cx_java_throw(JNIEnv *env, const char *class, const char *msg)
{
    jclass exception = (*env)->FindClass(env, class);
    (*env)->ThrowNew(env, exception, msg);
}

static void cx_java_npe(JNIEnv *env, const char *msg)
{
    cx_java_throw(env, "java/lang/NullPointerException", msg);
}

static const char *cx_java_string_new(JNIEnv *env, jstring string)
{
    return (*env)->GetStringUTFChars(env, string, 0);
}

static void cx_java_string_free(JNIEnv *env, jstring string, const char *ptr)
{
    (*env)->ReleaseStringUTFChars(env, string, ptr);
}

static struct cx_predicate *cx_java_predicate_cast(JNIEnv *env, jlong ptr)
{
    struct cx_predicate *predicate = (struct cx_predicate *)ptr;
    if (!predicate)
        cx_java_npe(env, "predicate ptr");
    return predicate;
}

jlong Java_com_columnix_jni_c_Reader_create(JNIEnv *env, jobject this,
                                            jstring java_path)
{
    const char *path = cx_java_string_new(env, java_path);
    if (!path)
        return 0;
    struct cx_reader *reader = cx_reader_new(path);
    cx_java_string_free(env, java_path, path);
    if (!reader)
        goto error;
    return (jlong)reader;
error:
    cx_java_throw(env, "java/lang/Exception", "cx_reader_new()");
    return 0;
}

jlong Java_com_columnix_jni_c_Reader_createMatching(JNIEnv *env, jobject this,
                                                    jstring java_path,
                                                    jlong ptr)
{
    struct cx_predicate *predicate = cx_java_predicate_cast(env, ptr);
    if (!predicate)
        return 0;
    const char *path = cx_java_string_new(env, java_path);
    if (!path)
        return 0;
    struct cx_reader *reader = cx_reader_new_matching(path, predicate);
    cx_java_string_free(env, java_path, path);
    if (!reader)
        goto error;
    return (jlong)reader;
error:
    cx_java_throw(env, "java/lang/Exception", "cx_reader_new_matching()");
    return 0;
}

static struct cx_reader *cx_java_reader_cast(JNIEnv *env, jlong ptr)
{
    struct cx_reader *reader = (struct cx_reader *)ptr;
    if (!reader)
        cx_java_npe(env, "reader ptr");
    return reader;
}

void Java_com_columnix_jni_c_Reader_free(JNIEnv *env, jobject this, jlong ptr)
{
    struct cx_reader *reader = cx_java_reader_cast(env, ptr);
    if (!reader)
        return;
    cx_reader_free(reader);
}

jint Java_com_columnix_jni_c_Reader_columnCount(JNIEnv *env, jobject this,
                                                jlong ptr)
{
    struct cx_reader *reader = cx_java_reader_cast(env, ptr);
    if (!reader)
        return 0;
    return cx_reader_column_count(reader);
}

jlong Java_com_columnix_jni_c_Reader_rowCount(JNIEnv *env, jobject this,
                                              jlong ptr)
{
    struct cx_reader *reader = cx_java_reader_cast(env, ptr);
    if (!reader)
        return 0;
    return cx_reader_row_count(reader);
}

void Java_com_columnix_jni_c_Reader_rewind(JNIEnv *env, jobject this, jlong ptr)
{
    struct cx_reader *reader = cx_java_reader_cast(env, ptr);
    if (!reader)
        return;
    cx_reader_rewind(reader);
}

jboolean Java_com_columnix_jni_c_Reader_next(JNIEnv *env, jobject this,
                                             jlong ptr)
{
    struct cx_reader *reader = cx_java_reader_cast(env, ptr);
    if (!reader)
        return JNI_FALSE;
    if (!cx_reader_next(reader)) {
        if (cx_reader_error(reader))
            cx_java_throw(env, "java/lang/Exception", "cx_reader_error()");
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

static bool cx_java_reader_check_column_bounds(JNIEnv *env,
                                               const struct cx_reader *reader,
                                               jint index)
{
    if (index < 0 || index >= cx_reader_column_count(reader)) {
        cx_java_throw(env, "java/lang/IndexOutOfBoundsException",
                      "column index out of bounds");
        return false;
    }
    return true;
}

jstring Java_com_columnix_jni_c_Reader_getMetadata(JNIEnv *env, jobject this,
                                                   jlong ptr)
{
    struct cx_reader *reader = cx_java_reader_cast(env, ptr);
    if (!reader)
        return NULL;
    const char *metadata = NULL;
    if (!cx_reader_metadata(reader, &metadata)) {
        cx_java_throw(env, "java/lang/Exception", "cx_reader_metadata()");
        return NULL;
    }
    return metadata ? (*env)->NewStringUTF(env, metadata) : NULL;
}

jstring Java_com_columnix_jni_c_Reader_columnName(JNIEnv *env, jobject this,
                                                  jlong ptr, jint index)
{
    struct cx_reader *reader = cx_java_reader_cast(env, ptr);
    if (!reader)
        return 0;
    if (!cx_java_reader_check_column_bounds(env, reader, index))
        return 0;

    const char *name = cx_reader_column_name(reader, index);
    if (!name) {
        cx_java_npe(env, "column name");
        return 0;
    }
    return (*env)->NewStringUTF(env, name);
}

jint Java_com_columnix_jni_c_Reader_columnType(JNIEnv *env, jobject this,
                                               jlong ptr, jint index)
{
    struct cx_reader *reader = cx_java_reader_cast(env, ptr);
    if (!reader)
        return 0;
    if (!cx_java_reader_check_column_bounds(env, reader, index))
        return 0;
    return cx_reader_column_type(reader, index);
}

jint Java_com_columnix_jni_c_Reader_columnEncoding(JNIEnv *env, jobject this,
                                                   jlong ptr, jint index)
{
    struct cx_reader *reader = cx_java_reader_cast(env, ptr);
    if (!reader)
        return 0;
    if (!cx_java_reader_check_column_bounds(env, reader, index))
        return 0;
    return cx_reader_column_encoding(reader, index);
}

jint Java_com_columnix_jni_c_Reader_columnCompression(JNIEnv *env, jobject this,
                                                      jlong ptr, jint index)
{
    struct cx_reader *reader = cx_java_reader_cast(env, ptr);
    if (!reader)
        return 0;
    if (!cx_java_reader_check_column_bounds(env, reader, index))
        return 0;
    return cx_reader_column_compression(reader, index);
}

jboolean Java_com_columnix_jni_c_Reader_isNull(JNIEnv *env, jobject this,
                                               jlong ptr, jint index)
{
    struct cx_reader *reader = cx_java_reader_cast(env, ptr);
    if (!reader)
        return 0;
    if (!cx_java_reader_check_column_bounds(env, reader, index))
        return 0;
    bool value;
    if (!cx_reader_get_null(reader, index, &value)) {
        cx_java_throw(env, "java/lang/Exception", "cx_reader_get_null()");
        return JNI_FALSE;
    }
    return (jboolean)value;
}

jboolean Java_com_columnix_jni_c_Reader_getBoolean(JNIEnv *env, jobject this,
                                                   jlong ptr, jint index)
{
    struct cx_reader *reader = cx_java_reader_cast(env, ptr);
    if (!reader)
        return JNI_FALSE;
    if (!cx_java_reader_check_column_bounds(env, reader, index))
        return JNI_FALSE;
    bool value;
    if (!cx_reader_get_bit(reader, index, &value)) {
        cx_java_throw(env, "java/lang/Exception", "cx_reader_get_bit()");
        return JNI_FALSE;
    }
    return (jboolean)value;
}

jint Java_com_columnix_jni_c_Reader_getInt(JNIEnv *env, jobject this, jlong ptr,
                                           jint index)
{
    struct cx_reader *reader = cx_java_reader_cast(env, ptr);
    if (!reader)
        return 0;
    if (!cx_java_reader_check_column_bounds(env, reader, index))
        return 0;
    int32_t value;
    if (!cx_reader_get_i32(reader, index, &value)) {
        cx_java_throw(env, "java/lang/Exception", "cx_reader_get_i32()");
        return 0;
    }
    return (jint)value;
}

jlong Java_com_columnix_jni_c_Reader_getLong(JNIEnv *env, jobject this,
                                             jlong ptr, jint index)
{
    struct cx_reader *reader = cx_java_reader_cast(env, ptr);
    if (!reader)
        return 0;
    if (!cx_java_reader_check_column_bounds(env, reader, index))
        return 0;
    int64_t value;
    if (!cx_reader_get_i64(reader, index, &value)) {
        cx_java_throw(env, "java/lang/Exception", "cx_reader_get_i64()");
        return 0;
    }
    return (jlong)value;
}

jstring Java_com_columnix_jni_c_Reader_getString(JNIEnv *env, jobject this,
                                                 jlong ptr, jint index)
{
    struct cx_reader *reader = cx_java_reader_cast(env, ptr);
    if (!reader)
        return NULL;
    if (!cx_java_reader_check_column_bounds(env, reader, index))
        return NULL;
    const struct cx_string *value;
    if (!cx_reader_get_str(reader, index, &value)) {
        cx_java_throw(env, "java/lang/Exception", "cx_reader_get_str()");
        return NULL;
    }
    return (*env)->NewStringUTF(env, value->ptr);
}

jbyteArray Java_com_columnix_jni_c_Reader_getStringBytes(JNIEnv *env,
                                                         jobject this,
                                                         jlong ptr, jint index)
{
    struct cx_reader *reader = cx_java_reader_cast(env, ptr);
    if (!reader)
        return NULL;
    if (!cx_java_reader_check_column_bounds(env, reader, index))
        return NULL;
    const struct cx_string *value;
    if (!cx_reader_get_str(reader, index, &value)) {
        cx_java_throw(env, "java/lang/Exception", "cx_reader_get_str()");
        return NULL;
    }
    jbyteArray bytes = (*env)->NewByteArray(env, value->len);
    if (!bytes)
        return NULL;
    (*env)->SetByteArrayRegion(env, bytes, 0, value->len,
                               (const jbyte *)value->ptr);
    return bytes;
}

jlong Java_com_columnix_jni_c_Writer_create(JNIEnv *env, jobject this,
                                            jstring java_path,
                                            jlong row_group_size)
{
    if (row_group_size <= 0) {
        cx_java_throw(env, "java/lang/IllegalArgumentException",
                      "row group size <= 0");
        return 0;
    }
    const char *path = cx_java_string_new(env, java_path);
    if (!path)
        return 0;
    struct cx_writer *writer = cx_writer_new(path, row_group_size);
    cx_java_string_free(env, java_path, path);
    if (!writer)
        goto error;
    return (jlong)writer;
error:
    cx_java_throw(env, "java/lang/Exception", "cx_writer_new()");
    return 0;
}

static struct cx_writer *cx_java_writer_cast(JNIEnv *env, jlong ptr)
{
    struct cx_writer *writer = (struct cx_writer *)ptr;
    if (!writer)
        cx_java_npe(env, "writer ptr");
    return writer;
}

void Java_com_columnix_jni_c_Writer_free(JNIEnv *env, jobject this, jlong ptr)
{
    struct cx_writer *writer = cx_java_writer_cast(env, ptr);
    if (!writer)
        return;
    cx_writer_free(writer);
}

void Java_com_columnix_jni_c_Writer_setMetadata(JNIEnv *env, jobject this,
                                                jlong ptr,
                                                jstring java_metadata)
{
    struct cx_writer *writer = cx_java_writer_cast(env, ptr);
    if (!writer)
        return;
    const char *metadata = cx_java_string_new(env, java_metadata);
    if (!metadata)
        return;
    if (!cx_writer_metadata(writer, metadata))
        cx_java_throw(env, "java/lang/Exception", "cx_writer_metadata()");
    cx_java_string_free(env, java_metadata, metadata);
}

void Java_com_columnix_jni_c_Writer_finish(JNIEnv *env, jobject this, jlong ptr,
                                           jboolean sync)
{
    struct cx_writer *writer = cx_java_writer_cast(env, ptr);
    if (!writer)
        return;
    if (!cx_writer_finish(writer, sync))
        cx_java_throw(env, "java/lang/Exception", "cx_writer_finish()");
}

void Java_com_columnix_jni_c_Writer_addColumn(JNIEnv *env, jobject this,
                                              jlong ptr, jstring java_name,
                                              jint type, jint encoding,
                                              jint compression, jint level)
{
    struct cx_writer *writer = cx_java_writer_cast(env, ptr);
    if (!writer)
        return;
    const char *name = cx_java_string_new(env, java_name);
    if (!name)
        return;
    if (!cx_writer_add_column(writer, name, type, encoding, compression, level))
        cx_java_throw(env, "java/lang/Exception", "cx_writer_add_column()");
    cx_java_string_free(env, java_name, name);
}

void Java_com_columnix_jni_c_Writer_putNull(JNIEnv *env, jobject this,
                                            jlong ptr, jint index)
{
    struct cx_writer *writer = cx_java_writer_cast(env, ptr);
    if (!writer)
        return;
    if (!cx_writer_put_null(writer, index))
        cx_java_throw(env, "java/lang/Exception", "cx_writer_put_null()");
}

void Java_com_columnix_jni_c_Writer_putBoolean(JNIEnv *env, jobject this,
                                               jlong ptr, jint index,
                                               jboolean value)
{
    struct cx_writer *writer = cx_java_writer_cast(env, ptr);
    if (!writer)
        return;
    if (!cx_writer_put_bit(writer, index, value))
        cx_java_throw(env, "java/lang/Exception", "cx_writer_put_bit()");
}

void Java_com_columnix_jni_c_Writer_putInt(JNIEnv *env, jobject this, jlong ptr,
                                           jint index, jint value)
{
    struct cx_writer *writer = cx_java_writer_cast(env, ptr);
    if (!writer)
        return;
    if (!cx_writer_put_i32(writer, index, value))
        cx_java_throw(env, "java/lang/Exception", "cx_writer_put_i32()");
}

void Java_com_columnix_jni_c_Writer_putLong(JNIEnv *env, jobject this,
                                            jlong ptr, jint index, jlong value)
{
    struct cx_writer *writer = cx_java_writer_cast(env, ptr);
    if (!writer)
        return;
    if (!cx_writer_put_i64(writer, index, value))
        cx_java_throw(env, "java/lang/Exception", "cx_writer_put_i64()");
}

void Java_com_columnix_jni_c_Writer_putString(JNIEnv *env, jobject this,
                                              jlong ptr, jint index,
                                              jstring java_value)
{
    struct cx_writer *writer = cx_java_writer_cast(env, ptr);
    if (!writer)
        return;
    bool ok;
    if (!java_value) {
        ok = cx_writer_put_null(writer, index);
    } else {
        const char *value = cx_java_string_new(env, java_value);
        if (!value)
            return;
        ok = cx_writer_put_str(writer, index, value);
        cx_java_string_free(env, java_value, value);
    }
    if (!ok)
        cx_java_throw(env, "java/lang/Exception", "cx_writer_put_str()");
}

static jlong cx_java_predicate_ptr(JNIEnv *env, const struct cx_predicate *ptr)
{
    if (!ptr)
        cx_java_throw(env, "java/lang/Exception", "bad predicate or OOM");
    return (jlong)ptr;
}

jlong Java_com_columnix_jni_c_Predicate_negate(JNIEnv *env, jobject this,
                                               jlong ptr)
{
    struct cx_predicate *predicate = cx_java_predicate_cast(env, ptr);
    if (!predicate)
        return 0;
    return cx_java_predicate_ptr(env, cx_predicate_negate(predicate));
}

void Java_com_columnix_jni_c_Predicate_free(JNIEnv *env, jobject this,
                                            jlong ptr)
{
    struct cx_predicate *predicate = cx_java_predicate_cast(env, ptr);
    if (!predicate)
        return;
    cx_predicate_free(predicate);
}

jlong Java_com_columnix_jni_c_Predicate_and(JNIEnv *env, jobject this,
                                            jlongArray array)
{
    size_t count = (*env)->GetArrayLength(env, array);
    assert(sizeof(jlong) == sizeof(struct cx_predicate *));
    jlong *operands = (*env)->GetLongArrayElements(env, array, NULL);
    if (!operands)
        return 0;
    struct cx_predicate *predicate =
        cx_predicate_new_aand(count, (struct cx_predicate **)operands);
    (*env)->ReleaseLongArrayElements(env, array, operands, JNI_ABORT);
    return cx_java_predicate_ptr(env, predicate);
}

jlong Java_com_columnix_jni_c_Predicate_or(JNIEnv *env, jobject this,
                                           jlongArray array)
{
    size_t count = (*env)->GetArrayLength(env, array);
    assert(sizeof(jlong) == sizeof(struct cx_predicate *));
    jlong *operands = (*env)->GetLongArrayElements(env, array, NULL);
    if (!operands)
        return 0;
    struct cx_predicate *predicate =
        cx_predicate_new_aor(count, (struct cx_predicate **)operands);
    (*env)->ReleaseLongArrayElements(env, array, operands, JNI_ABORT);
    return cx_java_predicate_ptr(env, predicate);
}

jlong Java_com_columnix_jni_c_Predicate_isNull(JNIEnv *env, jobject this,
                                               jint column)
{
    return cx_java_predicate_ptr(env, cx_predicate_new_null(column));
}

jlong Java_com_columnix_jni_c_Predicate_booleanEquals(JNIEnv *env, jobject this,
                                                      jint column,
                                                      jboolean value)
{
    return cx_java_predicate_ptr(env, cx_predicate_new_bit_eq(column, value));
}

jlong Java_com_columnix_jni_c_Predicate_intEquals(JNIEnv *env, jobject this,
                                                  jint column, jint value)
{
    return cx_java_predicate_ptr(env, cx_predicate_new_i32_eq(column, value));
}

jlong Java_com_columnix_jni_c_Predicate_intGreaterThan(JNIEnv *env,
                                                       jobject this,
                                                       jint column, jint value)
{
    return cx_java_predicate_ptr(env, cx_predicate_new_i32_gt(column, value));
}

jlong Java_com_columnix_jni_c_Predicate_intLessThan(JNIEnv *env, jobject this,
                                                    jint column, jint value)
{
    return cx_java_predicate_ptr(env, cx_predicate_new_i32_lt(column, value));
}

jlong Java_com_columnix_jni_c_Predicate_longEquals(JNIEnv *env, jobject this,
                                                   jint column, jlong value)
{
    return cx_java_predicate_ptr(env, cx_predicate_new_i64_eq(column, value));
}

jlong Java_com_columnix_jni_c_Predicate_longGreaterThan(JNIEnv *env,
                                                        jobject this,
                                                        jint column,
                                                        jlong value)
{
    return cx_java_predicate_ptr(env, cx_predicate_new_i64_gt(column, value));
}

jlong Java_com_columnix_jni_c_Predicate_longLessThan(JNIEnv *env, jobject this,
                                                     jint column, jlong value)
{
    return cx_java_predicate_ptr(env, cx_predicate_new_i64_lt(column, value));
}

jlong Java_com_columnix_jni_c_Predicate_stringEquals(JNIEnv *env, jobject this,
                                                     jint column,
                                                     jstring java_string,
                                                     jboolean caseSensitive)
{
    const char *string = cx_java_string_new(env, java_string);
    if (!string)
        return 0;
    struct cx_predicate *predicate =
        cx_predicate_new_str_eq(column, string, caseSensitive);
    cx_java_string_free(env, java_string, string);
    return cx_java_predicate_ptr(env, predicate);
}

jlong Java_com_columnix_jni_c_Predicate_stringGreaterThan(
    JNIEnv *env, jobject this, jint column, jstring java_string,
    jboolean caseSensitive)
{
    const char *string = cx_java_string_new(env, java_string);
    if (!string)
        return 0;
    struct cx_predicate *predicate =
        cx_predicate_new_str_gt(column, string, caseSensitive);
    cx_java_string_free(env, java_string, string);
    return cx_java_predicate_ptr(env, predicate);
}

jlong Java_com_columnix_jni_c_Predicate_stringLessThan(JNIEnv *env,
                                                       jobject this,
                                                       jint column,
                                                       jstring java_string,
                                                       jboolean caseSensitive)
{
    const char *string = cx_java_string_new(env, java_string);
    if (!string)
        return 0;
    struct cx_predicate *predicate =
        cx_predicate_new_str_lt(column, string, caseSensitive);
    cx_java_string_free(env, java_string, string);
    return cx_java_predicate_ptr(env, predicate);
}

jlong Java_com_columnix_jni_c_Predicate_stringContains(
    JNIEnv *env, jobject this, jint column, jstring java_string, jint location,
    jboolean caseSensitive)
{
    const char *string = cx_java_string_new(env, java_string);
    if (!string)
        return 0;
    struct cx_predicate *predicate =
        cx_predicate_new_str_contains(column, string, caseSensitive, location);
    cx_java_string_free(env, java_string, string);
    return cx_java_predicate_ptr(env, predicate);
}
