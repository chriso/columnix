#ifndef ZCS_JAVA_
#define ZCS_JAVA_

#include <jni.h>

jlong Java_zcs_jni_Reader_nativeNew(JNIEnv *, jobject, jstring);

void Java_zcs_jni_Reader_nativeFree(JNIEnv *, jobject, jlong);

jlong Java_zcs_jni_Reader_nativeColumnCount(JNIEnv *, jobject, jlong);

jint Java_zcs_jni_Reader_nativeColumnType(JNIEnv *, jobject, jlong, jint);

jint Java_zcs_jni_Reader_nativeColumnEncoding(JNIEnv *, jobject, jlong, jint);

jint Java_zcs_jni_Reader_nativeColumnCompression(JNIEnv *, jobject, jlong,
                                                 jint);

#endif
