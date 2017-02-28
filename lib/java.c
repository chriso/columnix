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
    zcs_java_throw(env, "java/lang/IllegalArgumentException",
                   "zcs_reader_new()");
    return 0;
}

void Java_zcs_jni_Reader_nativeFree(JNIEnv *env, jobject this, jlong ptr)
{
    struct zcs_reader *reader = (struct zcs_reader *)ptr;
    if (!reader)
        goto error;
    zcs_reader_free(reader);
    return;
error:
    zcs_java_npe(env, "zcs_reader_free()");
}

jlong Java_zcs_jni_Reader_nativeColumnCount(JNIEnv *env, jobject this,
                                            jlong ptr)
{
    struct zcs_reader *reader = (struct zcs_reader *)ptr;
    if (!reader)
        goto error;
    return zcs_reader_column_count(reader);
error:
    zcs_java_npe(env, "zcs_reader_column_count()");
    return 0;
}
