#ifndef ZCS_JAVA_
#define ZCS_JAVA_

#include <jni.h>

jlong Java_zcs_jni_Reader_nativeNew(JNIEnv *, jobject, jstring);
void Java_zcs_jni_Reader_nativeFree(JNIEnv *, jobject, jlong);
jint Java_zcs_jni_Reader_nativeColumnCount(JNIEnv *, jobject, jlong);
jlong Java_zcs_jni_Reader_nativeRowCount(JNIEnv *, jobject, jlong);
void Java_zcs_jni_Reader_nativeRewind(JNIEnv *, jobject, jlong);
jboolean Java_zcs_jni_Reader_nativeNext(JNIEnv *, jobject, jlong);
jint Java_zcs_jni_Reader_nativeColumnType(JNIEnv *, jobject, jlong, jint);
jint Java_zcs_jni_Reader_nativeColumnEncoding(JNIEnv *, jobject, jlong, jint);
jint Java_zcs_jni_Reader_nativeColumnCompression(JNIEnv *, jobject, jlong,
                                                 jint);
jboolean Java_zcs_jni_Reader_nativeIsNull(JNIEnv *, jobject, jlong, jint);
jboolean Java_zcs_jni_Reader_nativeGetBoolean(JNIEnv *, jobject, jlong, jint);
jint Java_zcs_jni_Reader_nativeGetInt(JNIEnv *, jobject, jlong, jint);
jlong Java_zcs_jni_Reader_nativeGetLong(JNIEnv *, jobject, jlong, jint);
jstring Java_zcs_jni_Reader_nativeGetString(JNIEnv *, jobject, jlong, jint);

jlong Java_zcs_jni_Writer_nativeNew(JNIEnv *, jobject, jstring, jlong);
void Java_zcs_jni_Writer_nativeFree(JNIEnv *, jobject, jlong);
void Java_zcs_jni_Writer_nativeFinish(JNIEnv *, jobject, jlong, jboolean);
void Java_zcs_jni_Writer_nativeAddColumn(JNIEnv *, jobject, jlong, jint, jint,
                                         jint, jint);
void Java_zcs_jni_Writer_nativePutNull(JNIEnv *, jobject, jlong, jint);
void Java_zcs_jni_Writer_nativePutBoolean(JNIEnv *, jobject, jlong, jint,
                                          jboolean);
void Java_zcs_jni_Writer_nativePutInt(JNIEnv *, jobject, jlong, jint, jint);
void Java_zcs_jni_Writer_nativePutLong(JNIEnv *, jobject, jlong, jint, jlong);
void Java_zcs_jni_Writer_nativePutString(JNIEnv *, jobject, jlong, jint,
                                         jstring);

#endif
