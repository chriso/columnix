#ifndef CX_JAVA_
#define CX_JAVA_

#include <jni.h>

jlong Java_com_columnix_jni_Reader_nativeNew(JNIEnv *, jobject, jstring);
jlong Java_com_columnix_jni_Reader_nativeNewMatching(JNIEnv *, jobject, jstring,
                                                     jlong);
void Java_com_columnix_jni_Reader_nativeFree(JNIEnv *, jobject, jlong);
jint Java_com_columnix_jni_Reader_nativeColumnCount(JNIEnv *, jobject, jlong);
jlong Java_com_columnix_jni_Reader_nativeRowCount(JNIEnv *, jobject, jlong);
void Java_com_columnix_jni_Reader_nativeRewind(JNIEnv *, jobject, jlong);
jboolean Java_com_columnix_jni_Reader_nativeNext(JNIEnv *, jobject, jlong);
jstring Java_com_columnix_jni_Reader_nativeColumnName(JNIEnv *, jobject, jlong,
                                                      jint);
jint Java_com_columnix_jni_Reader_nativeColumnType(JNIEnv *, jobject, jlong,
                                                   jint);
jint Java_com_columnix_jni_Reader_nativeColumnEncoding(JNIEnv *, jobject, jlong,
                                                       jint);
jint Java_com_columnix_jni_Reader_nativeColumnCompression(JNIEnv *, jobject,
                                                          jlong, jint);
jboolean Java_com_columnix_jni_Reader_nativeIsNull(JNIEnv *, jobject, jlong,
                                                   jint);
jboolean Java_com_columnix_jni_Reader_nativeGetBoolean(JNIEnv *, jobject, jlong,
                                                       jint);
jint Java_com_columnix_jni_Reader_nativeGetInt(JNIEnv *, jobject, jlong, jint);
jlong Java_com_columnix_jni_Reader_nativeGetLong(JNIEnv *, jobject, jlong,
                                                 jint);
jstring Java_com_columnix_jni_Reader_nativeGetString(JNIEnv *, jobject, jlong,
                                                     jint);
jbyteArray Java_com_columnix_jni_Reader_nativeGetStringBytes(JNIEnv *, jobject,
                                                             jlong, jint);

jlong Java_com_columnix_jni_Writer_nativeNew(JNIEnv *, jobject, jstring, jlong);
void Java_com_columnix_jni_Writer_nativeFree(JNIEnv *, jobject, jlong);
void Java_com_columnix_jni_Writer_nativeFinish(JNIEnv *, jobject, jlong,
                                               jboolean);
void Java_com_columnix_jni_Writer_nativeAddColumn(JNIEnv *, jobject, jlong,
                                                  jstring, jint, jint, jint,
                                                  jint);
void Java_com_columnix_jni_Writer_nativePutNull(JNIEnv *, jobject, jlong, jint);
void Java_com_columnix_jni_Writer_nativePutBoolean(JNIEnv *, jobject, jlong,
                                                   jint, jboolean);
void Java_com_columnix_jni_Writer_nativePutInt(JNIEnv *, jobject, jlong, jint,
                                               jint);
void Java_com_columnix_jni_Writer_nativePutLong(JNIEnv *, jobject, jlong, jint,
                                                jlong);
void Java_com_columnix_jni_Writer_nativePutString(JNIEnv *, jobject, jlong,
                                                  jint, jstring);

jlong Java_com_columnix_jni_Predicate_00024_nativeNegate(JNIEnv *, jobject,
                                                         jlong);
void Java_com_columnix_jni_Predicate_00024_nativeFree(JNIEnv *, jobject, jlong);
jlong Java_com_columnix_jni_Predicate_00024_nativeAnd(JNIEnv *, jobject,
                                                      jlongArray);
jlong Java_com_columnix_jni_Predicate_00024_nativeOr(JNIEnv *, jobject,
                                                     jlongArray);
jlong Java_com_columnix_jni_Predicate_00024_nativeNull(JNIEnv *, jobject, jint);

jlong Java_com_columnix_jni_Predicate_00024_nativeBooleanEquals(JNIEnv *,
                                                                jobject, jint,
                                                                jboolean);

jlong Java_com_columnix_jni_Predicate_00024_nativeIntEquals(JNIEnv *, jobject,
                                                            jint, jint);
jlong Java_com_columnix_jni_Predicate_00024_nativeIntGreaterThan(JNIEnv *,
                                                                 jobject, jint,
                                                                 jint);
jlong Java_com_columnix_jni_Predicate_00024_nativeIntLessThan(JNIEnv *, jobject,
                                                              jint, jint);

jlong Java_com_columnix_jni_Predicate_00024_nativeLongEquals(JNIEnv *, jobject,
                                                             jint, jlong);
jlong Java_com_columnix_jni_Predicate_00024_nativeLongGreaterThan(JNIEnv *,
                                                                  jobject, jint,
                                                                  jlong);
jlong Java_com_columnix_jni_Predicate_00024_nativeLongLessThan(JNIEnv *,
                                                               jobject, jint,
                                                               jlong);

jlong Java_com_columnix_jni_Predicate_00024_nativeStringEquals(JNIEnv *,
                                                               jobject, jint,
                                                               jstring,
                                                               jboolean);
jlong Java_com_columnix_jni_Predicate_00024_nativeStringGreaterThan(
    JNIEnv *, jobject, jint, jstring, jboolean);
jlong Java_com_columnix_jni_Predicate_00024_nativeStringLessThan(JNIEnv *,
                                                                 jobject, jint,
                                                                 jstring,
                                                                 jboolean);
jlong Java_com_columnix_jni_Predicate_00024_nativeStringContains(JNIEnv *,
                                                                 jobject, jint,
                                                                 jstring, jint,
                                                                 jboolean);

#endif
