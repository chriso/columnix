#include "java.h"
#include "reader.h"

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

jlong Java_zcs_jni_Reader_nativeColumnCount(JNIEnv *env, jobject this,
                                            jlong ptr)
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
