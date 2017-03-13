#ifndef CX_JAVA_
#define CX_JAVA_

#include <jni.h>

jlong Java_com_columnix_jni_Reader_cNew(JNIEnv *, jobject, jstring);
jlong Java_com_columnix_jni_Reader_cNewMatching(JNIEnv *, jobject, jstring,
                                                jlong);
void Java_com_columnix_jni_Reader_cFree(JNIEnv *, jobject, jlong);
jstring Java_com_columnix_jni_Reader_cMetadata(JNIEnv *, jobject, jlong);
jint Java_com_columnix_jni_Reader_cColumnCount(JNIEnv *, jobject, jlong);
jlong Java_com_columnix_jni_Reader_cRowCount(JNIEnv *, jobject, jlong);
void Java_com_columnix_jni_Reader_cRewind(JNIEnv *, jobject, jlong);
jboolean Java_com_columnix_jni_Reader_cNext(JNIEnv *, jobject, jlong);
jstring Java_com_columnix_jni_Reader_cColumnName(JNIEnv *, jobject, jlong,
                                                 jint);
jint Java_com_columnix_jni_Reader_cColumnType(JNIEnv *, jobject, jlong, jint);
jint Java_com_columnix_jni_Reader_cColumnEncoding(JNIEnv *, jobject, jlong,
                                                  jint);
jint Java_com_columnix_jni_Reader_cColumnCompression(JNIEnv *, jobject, jlong,
                                                     jint);
jboolean Java_com_columnix_jni_Reader_cIsNull(JNIEnv *, jobject, jlong, jint);
jboolean Java_com_columnix_jni_Reader_cGetBoolean(JNIEnv *, jobject, jlong,
                                                  jint);
jint Java_com_columnix_jni_Reader_cGetInt(JNIEnv *, jobject, jlong, jint);
jlong Java_com_columnix_jni_Reader_cGetLong(JNIEnv *, jobject, jlong, jint);
jstring Java_com_columnix_jni_Reader_cGetString(JNIEnv *, jobject, jlong, jint);
jbyteArray Java_com_columnix_jni_Reader_cGetStringBytes(JNIEnv *, jobject,
                                                        jlong, jint);

jlong Java_com_columnix_jni_Writer_cNew(JNIEnv *, jobject, jstring, jlong);
void Java_com_columnix_jni_Writer_cFree(JNIEnv *, jobject, jlong);
void Java_com_columnix_jni_Writer_cMetadata(JNIEnv *, jobject, jlong, jstring);
void Java_com_columnix_jni_Writer_cFinish(JNIEnv *, jobject, jlong, jboolean);
void Java_com_columnix_jni_Writer_cAddColumn(JNIEnv *, jobject, jlong, jstring,
                                             jint, jint, jint, jint);
void Java_com_columnix_jni_Writer_cPutNull(JNIEnv *, jobject, jlong, jint);
void Java_com_columnix_jni_Writer_cPutBoolean(JNIEnv *, jobject, jlong, jint,
                                              jboolean);
void Java_com_columnix_jni_Writer_cPutInt(JNIEnv *, jobject, jlong, jint, jint);
void Java_com_columnix_jni_Writer_cPutLong(JNIEnv *, jobject, jlong, jint,
                                           jlong);
void Java_com_columnix_jni_Writer_cPutString(JNIEnv *, jobject, jlong, jint,
                                             jstring);

jlong Java_com_columnix_jni_Predicate_00024_cNegate(JNIEnv *, jobject, jlong);
void Java_com_columnix_jni_Predicate_00024_cFree(JNIEnv *, jobject, jlong);
jlong Java_com_columnix_jni_Predicate_00024_cAnd(JNIEnv *, jobject, jlongArray);
jlong Java_com_columnix_jni_Predicate_00024_cOr(JNIEnv *, jobject, jlongArray);
jlong Java_com_columnix_jni_Predicate_00024_cNull(JNIEnv *, jobject, jint);

jlong Java_com_columnix_jni_Predicate_00024_cBooleanEquals(JNIEnv *, jobject,
                                                           jint, jboolean);

jlong Java_com_columnix_jni_Predicate_00024_cIntEquals(JNIEnv *, jobject, jint,
                                                       jint);
jlong Java_com_columnix_jni_Predicate_00024_cIntGreaterThan(JNIEnv *, jobject,
                                                            jint, jint);
jlong Java_com_columnix_jni_Predicate_00024_cIntLessThan(JNIEnv *, jobject,
                                                         jint, jint);

jlong Java_com_columnix_jni_Predicate_00024_cLongEquals(JNIEnv *, jobject, jint,
                                                        jlong);
jlong Java_com_columnix_jni_Predicate_00024_cLongGreaterThan(JNIEnv *, jobject,
                                                             jint, jlong);
jlong Java_com_columnix_jni_Predicate_00024_cLongLessThan(JNIEnv *, jobject,
                                                          jint, jlong);

jlong Java_com_columnix_jni_Predicate_00024_cStringEquals(JNIEnv *, jobject,
                                                          jint, jstring,
                                                          jboolean);
jlong Java_com_columnix_jni_Predicate_00024_cStringGreaterThan(JNIEnv *,
                                                               jobject, jint,
                                                               jstring,
                                                               jboolean);
jlong Java_com_columnix_jni_Predicate_00024_cStringLessThan(JNIEnv *, jobject,
                                                            jint, jstring,
                                                            jboolean);
jlong Java_com_columnix_jni_Predicate_00024_cStringContains(JNIEnv *, jobject,
                                                            jint, jstring, jint,
                                                            jboolean);

#endif
