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

jlong Java_com_columnix_jni_Reader_create(JNIEnv *env, jobject this,
                                          jstring java_path)
{
    const char *path = (*env)->GetStringUTFChars(env, java_path, 0);
    if (!path)
        return 0;
    struct cx_reader *reader = cx_reader_new(path);
    (*env)->ReleaseStringUTFChars(env, java_path, path);
    if (!reader)
        cx_java_throw(env, "java/lang/Exception", "cx_reader_new()");
    return (jlong)reader;
}

jlong Java_com_columnix_jni_Reader_createMatching(JNIEnv *env, jobject this,
                                                  jstring java_path, jlong ptr)
{
    struct cx_predicate *predicate = (struct cx_predicate *)ptr;
    const char *path = (*env)->GetStringUTFChars(env, java_path, 0);
    if (!path)
        return 0;
    struct cx_reader *reader = cx_reader_new_matching(path, predicate);
    (*env)->ReleaseStringUTFChars(env, java_path, path);
    if (!reader)
        cx_java_throw(env, "java/lang/Exception", "cx_reader_new_matching()");
    return (jlong)reader;
}

void Java_com_columnix_jni_Reader_free(JNIEnv *env, jobject this, jlong ptr)
{
    cx_reader_free((struct cx_reader *)ptr);
}

jint Java_com_columnix_jni_Reader_columnCount(JNIEnv *env, jobject this,
                                              jlong ptr)
{
    return cx_reader_column_count((struct cx_reader *)ptr);
}

jlong Java_com_columnix_jni_Reader_rowCount(JNIEnv *env, jobject this,
                                            jlong ptr)
{
    return cx_reader_row_count((struct cx_reader *)ptr);
}

void Java_com_columnix_jni_Reader_rewind(JNIEnv *env, jobject this, jlong ptr)
{
    cx_reader_rewind((struct cx_reader *)ptr);
}

jboolean Java_com_columnix_jni_Reader_next(JNIEnv *env, jobject this, jlong ptr)
{
    struct cx_reader *reader = (struct cx_reader *)ptr;
    if (!cx_reader_next(reader)) {
        if (cx_reader_error(reader))
            cx_java_throw(env, "java/lang/Exception", "cx_reader_error()");
        return JNI_FALSE;
    }
    return JNI_TRUE;
}

jstring Java_com_columnix_jni_Reader_getMetadata(JNIEnv *env, jobject this,
                                                 jlong ptr)
{
    const char *metadata = NULL;
    if (!cx_reader_metadata((struct cx_reader *)ptr, &metadata)) {
        cx_java_throw(env, "java/lang/Exception", "cx_reader_metadata()");
        return NULL;
    }
    return metadata ? (*env)->NewStringUTF(env, metadata) : NULL;
}

jstring Java_com_columnix_jni_Reader_columnName(JNIEnv *env, jobject this,
                                                jlong ptr, jint index)
{
    const char *name = cx_reader_column_name((struct cx_reader *)ptr, index);
    if (!name) {
        cx_java_throw(env, "java/lang/Exception", "cx_reader_column_name()");
        return NULL;
    }
    return (*env)->NewStringUTF(env, name);
}

jint Java_com_columnix_jni_Reader_columnType(JNIEnv *env, jobject this,
                                             jlong ptr, jint index)
{
    return cx_reader_column_type((struct cx_reader *)ptr, index);
}

jint Java_com_columnix_jni_Reader_columnEncoding(JNIEnv *env, jobject this,
                                                 jlong ptr, jint index)
{
    return cx_reader_column_encoding((struct cx_reader *)ptr, index);
}

jint Java_com_columnix_jni_Reader_columnCompression(JNIEnv *env, jobject this,
                                                    jlong ptr, jint index)
{
    return cx_reader_column_compression((struct cx_reader *)ptr, index);
}

jboolean Java_com_columnix_jni_Reader_isNull(JNIEnv *env, jobject this,
                                             jlong ptr, jint index)
{
    bool value;
    if (!cx_reader_get_null((struct cx_reader *)ptr, index, &value)) {
        cx_java_throw(env, "java/lang/Exception", "cx_reader_get_null()");
        return JNI_FALSE;
    }
    return (jboolean)value;
}

jboolean Java_com_columnix_jni_Reader_getBoolean(JNIEnv *env, jobject this,
                                                 jlong ptr, jint index)
{
    bool value;
    if (!cx_reader_get_bit((struct cx_reader *)ptr, index, &value)) {
        cx_java_throw(env, "java/lang/Exception", "cx_reader_get_bit()");
        return JNI_FALSE;
    }
    return (jboolean)value;
}

jint Java_com_columnix_jni_Reader_getInt(JNIEnv *env, jobject this, jlong ptr,
                                         jint index)
{
    int32_t value;
    if (!cx_reader_get_i32((struct cx_reader *)ptr, index, &value)) {
        cx_java_throw(env, "java/lang/Exception", "cx_reader_get_i32()");
        return 0;
    }
    return (jint)value;
}

jlong Java_com_columnix_jni_Reader_getLong(JNIEnv *env, jobject this, jlong ptr,
                                           jint index)
{
    int64_t value;
    if (!cx_reader_get_i64((struct cx_reader *)ptr, index, &value)) {
        cx_java_throw(env, "java/lang/Exception", "cx_reader_get_i64()");
        return 0;
    }
    return (jlong)value;
}

jfloat Java_com_columnix_jni_Reader_getFloat(JNIEnv *env, jobject this,
                                             jlong ptr, jint index)
{
    float value;
    if (!cx_reader_get_flt((struct cx_reader *)ptr, index, &value)) {
        cx_java_throw(env, "java/lang/Exception", "cx_reader_get_flt()");
        return 0;
    }
    return (jfloat)value;
}

jdouble Java_com_columnix_jni_Reader_getDouble(JNIEnv *env, jobject this,
                                               jlong ptr, jint index)
{
    double value;
    if (!cx_reader_get_dbl((struct cx_reader *)ptr, index, &value)) {
        cx_java_throw(env, "java/lang/Exception", "cx_reader_get_dbl()");
        return 0;
    }
    return (jdouble)value;
}

jstring Java_com_columnix_jni_Reader_getString(JNIEnv *env, jobject this,
                                               jlong ptr, jint index)
{
    const struct cx_string *value;
    if (!cx_reader_get_str((struct cx_reader *)ptr, index, &value)) {
        cx_java_throw(env, "java/lang/Exception", "cx_reader_get_str()");
        return NULL;
    }
    return (*env)->NewStringUTF(env, value->ptr);
}

jbyteArray Java_com_columnix_jni_Reader_getStringBytes(JNIEnv *env,
                                                       jobject this, jlong ptr,
                                                       jint index)
{
    const struct cx_string *value;
    if (!cx_reader_get_str((struct cx_reader *)ptr, index, &value)) {
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

jlong Java_com_columnix_jni_Writer_create(JNIEnv *env, jobject this,
                                          jstring java_path,
                                          jlong row_group_size)
{
    const char *path = (*env)->GetStringUTFChars(env, java_path, 0);
    if (!path)
        return 0;
    struct cx_writer *writer = cx_writer_new(path, row_group_size);
    (*env)->ReleaseStringUTFChars(env, java_path, path);
    if (!writer)
        cx_java_throw(env, "java/lang/Exception", "cx_writer_new()");
    return (jlong)writer;
}

void Java_com_columnix_jni_Writer_free(JNIEnv *env, jobject this, jlong ptr)
{
    cx_writer_free((struct cx_writer *)ptr);
}

void Java_com_columnix_jni_Writer_setMetadata(JNIEnv *env, jobject this,
                                              jlong ptr, jstring java_metadata)
{
    const char *metadata = (*env)->GetStringUTFChars(env, java_metadata, 0);
    if (!metadata)
        return;
    if (!cx_writer_metadata((struct cx_writer *)ptr, metadata))
        cx_java_throw(env, "java/lang/Exception", "cx_writer_metadata()");
    (*env)->ReleaseStringUTFChars(env, java_metadata, metadata);
}

void Java_com_columnix_jni_Writer_finish(JNIEnv *env, jobject this, jlong ptr,
                                         jboolean sync)
{
    if (!cx_writer_finish((struct cx_writer *)ptr, sync))
        cx_java_throw(env, "java/lang/Exception", "cx_writer_finish()");
}

void Java_com_columnix_jni_Writer_addColumn(JNIEnv *env, jobject this,
                                            jlong ptr, jstring java_name,
                                            jint type, jint encoding,
                                            jint compression, jint level)
{
    struct cx_writer *writer = (struct cx_writer *)ptr;
    const char *name = (*env)->GetStringUTFChars(env, java_name, 0);
    if (!name)
        return;
    if (!cx_writer_add_column(writer, name, type, encoding, compression, level))
        cx_java_throw(env, "java/lang/Exception", "cx_writer_add_column()");
    (*env)->ReleaseStringUTFChars(env, java_name, name);
}

void Java_com_columnix_jni_Writer_putNull(JNIEnv *env, jobject this, jlong ptr,
                                          jint index)
{
    if (!cx_writer_put_null((struct cx_writer *)ptr, index))
        cx_java_throw(env, "java/lang/Exception", "cx_writer_put_null()");
}

void Java_com_columnix_jni_Writer_putBoolean(JNIEnv *env, jobject this,
                                             jlong ptr, jint index,
                                             jboolean value)
{
    if (!cx_writer_put_bit((struct cx_writer *)ptr, index, value))
        cx_java_throw(env, "java/lang/Exception", "cx_writer_put_bit()");
}

void Java_com_columnix_jni_Writer_putInt(JNIEnv *env, jobject this, jlong ptr,
                                         jint index, jint value)
{
    if (!cx_writer_put_i32((struct cx_writer *)ptr, index, value))
        cx_java_throw(env, "java/lang/Exception", "cx_writer_put_i32()");
}

void Java_com_columnix_jni_Writer_putLong(JNIEnv *env, jobject this, jlong ptr,
                                          jint index, jlong value)
{
    if (!cx_writer_put_i64((struct cx_writer *)ptr, index, value))
        cx_java_throw(env, "java/lang/Exception", "cx_writer_put_i64()");
}

void Java_com_columnix_jni_Writer_putFloat(JNIEnv *env, jobject this, jlong ptr,
                                           jint index, jfloat value)
{
    if (!cx_writer_put_flt((struct cx_writer *)ptr, index, value))
        cx_java_throw(env, "java/lang/Exception", "cx_writer_put_flt()");
}

void Java_com_columnix_jni_Writer_putDouble(JNIEnv *env, jobject this,
                                            jlong ptr, jint index,
                                            jdouble value)
{
    if (!cx_writer_put_dbl((struct cx_writer *)ptr, index, value))
        cx_java_throw(env, "java/lang/Exception", "cx_writer_put_dbl()");
}

void Java_com_columnix_jni_Writer_putString(JNIEnv *env, jobject this,
                                            jlong ptr, jint index,
                                            jstring java_value)
{
    struct cx_writer *writer = (struct cx_writer *)ptr;
    bool ok;
    if (!java_value) {
        ok = cx_writer_put_null(writer, index);
    } else {
        const char *value = (*env)->GetStringUTFChars(env, java_value, 0);
        if (!value)
            return;
        ok = cx_writer_put_str(writer, index, value);
        (*env)->ReleaseStringUTFChars(env, java_value, value);
    }
    if (!ok)
        cx_java_throw(env, "java/lang/Exception", "cx_writer_put_str()");
}

jlong Java_com_columnix_jni_Predicate_negate(JNIEnv *env, jobject this,
                                             jlong ptr)
{
    return (jlong)cx_predicate_negate((struct cx_predicate *)ptr);
}

void Java_com_columnix_jni_Predicate_free(JNIEnv *env, jobject this, jlong ptr)
{
    cx_predicate_free((struct cx_predicate *)ptr);
}

jlong Java_com_columnix_jni_Predicate_and(JNIEnv *env, jobject this,
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
    return (jlong)predicate;
}

jlong Java_com_columnix_jni_Predicate_or(JNIEnv *env, jobject this,
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
    return (jlong)predicate;
}

jlong Java_com_columnix_jni_Predicate_isNull(JNIEnv *env, jobject this,
                                             jint column)
{
    return (jlong)cx_predicate_new_null(column);
}

jlong Java_com_columnix_jni_Predicate_booleanEquals(JNIEnv *env, jobject this,
                                                    jint column, jboolean value)
{
    return (jlong)cx_predicate_new_bit_eq(column, value);
}

jlong Java_com_columnix_jni_Predicate_intEquals(JNIEnv *env, jobject this,
                                                jint column, jint value)
{
    return (jlong)cx_predicate_new_i32_eq(column, value);
}

jlong Java_com_columnix_jni_Predicate_intGreaterThan(JNIEnv *env, jobject this,
                                                     jint column, jint value)
{
    return (jlong)cx_predicate_new_i32_gt(column, value);
}

jlong Java_com_columnix_jni_Predicate_intLessThan(JNIEnv *env, jobject this,
                                                  jint column, jint value)
{
    return (jlong)cx_predicate_new_i32_lt(column, value);
}

jlong Java_com_columnix_jni_Predicate_longEquals(JNIEnv *env, jobject this,
                                                 jint column, jlong value)
{
    return (jlong)cx_predicate_new_i64_eq(column, value);
}

jlong Java_com_columnix_jni_Predicate_longGreaterThan(JNIEnv *env, jobject this,
                                                      jint column, jlong value)
{
    return (jlong)cx_predicate_new_i64_gt(column, value);
}

jlong Java_com_columnix_jni_Predicate_longLessThan(JNIEnv *env, jobject this,
                                                   jint column, jlong value)
{
    return (jlong)cx_predicate_new_i64_lt(column, value);
}

jfloat Java_com_columnix_jni_Predicate_floatEquals(JNIEnv *env, jobject this,
                                                   jint column, jfloat value)
{
    return (jlong)cx_predicate_new_flt_eq(column, value);
}

jfloat Java_com_columnix_jni_Predicate_floatGreaterThan(JNIEnv *env,
                                                        jobject this,
                                                        jint column,
                                                        jfloat value)
{
    return (jlong)cx_predicate_new_flt_gt(column, value);
}

jfloat Java_com_columnix_jni_Predicate_floatLessThan(JNIEnv *env, jobject this,
                                                     jint column, jfloat value)
{
    return (jlong)cx_predicate_new_flt_lt(column, value);
}

jdouble Java_com_columnix_jni_Predicate_doubleEquals(JNIEnv *env, jobject this,
                                                     jint column, jdouble value)
{
    return (jlong)cx_predicate_new_dbl_eq(column, value);
}

jdouble Java_com_columnix_jni_Predicate_doubleGreaterThan(JNIEnv *env,
                                                          jobject this,
                                                          jint column,
                                                          jdouble value)
{
    return (jlong)cx_predicate_new_dbl_gt(column, value);
}

jdouble Java_com_columnix_jni_Predicate_doubleLessThan(JNIEnv *env,
                                                       jobject this,
                                                       jint column,
                                                       jdouble value)
{
    return (jlong)cx_predicate_new_dbl_lt(column, value);
}

jlong Java_com_columnix_jni_Predicate_stringEquals(JNIEnv *env, jobject this,
                                                   jint column,
                                                   jstring java_string,
                                                   jboolean caseSensitive)
{
    const char *string = (*env)->GetStringUTFChars(env, java_string, 0);
    if (!string)
        return 0;
    struct cx_predicate *predicate =
        cx_predicate_new_str_eq(column, string, caseSensitive);
    (*env)->ReleaseStringUTFChars(env, java_string, string);
    return (jlong)predicate;
}

jlong Java_com_columnix_jni_Predicate_stringGreaterThan(JNIEnv *env,
                                                        jobject this,
                                                        jint column,
                                                        jstring java_string,
                                                        jboolean caseSensitive)
{
    const char *string = (*env)->GetStringUTFChars(env, java_string, 0);
    if (!string)
        return 0;
    struct cx_predicate *predicate =
        cx_predicate_new_str_gt(column, string, caseSensitive);
    (*env)->ReleaseStringUTFChars(env, java_string, string);
    return (jlong)predicate;
}

jlong Java_com_columnix_jni_Predicate_stringLessThan(JNIEnv *env, jobject this,
                                                     jint column,
                                                     jstring java_string,
                                                     jboolean caseSensitive)
{
    const char *string = (*env)->GetStringUTFChars(env, java_string, 0);
    if (!string)
        return 0;
    struct cx_predicate *predicate =
        cx_predicate_new_str_lt(column, string, caseSensitive);
    (*env)->ReleaseStringUTFChars(env, java_string, string);
    return (jlong)predicate;
}

jlong Java_com_columnix_jni_Predicate_stringContains(JNIEnv *env, jobject this,
                                                     jint column,
                                                     jstring java_string,
                                                     jint location,
                                                     jboolean caseSensitive)
{
    const char *string = (*env)->GetStringUTFChars(env, java_string, 0);
    if (!string)
        return 0;
    struct cx_predicate *predicate =
        cx_predicate_new_str_contains(column, string, caseSensitive, location);
    (*env)->ReleaseStringUTFChars(env, java_string, string);
    return (jlong)predicate;
}
