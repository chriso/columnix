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
