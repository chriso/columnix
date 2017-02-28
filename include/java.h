#ifndef ZCS_JAVA_
#define ZCS_JAVA_

#include <jni.h>

jlong Java_zcs_jni_Reader_nativeNew(JNIEnv *, jobject, jstring);

void Java_zcs_jni_Reader_nativeFree(JNIEnv *, jobject, jlong);

jlong Java_zcs_jni_Reader_nativeColumnCount(JNIEnv *, jobject, jlong);

#endif
