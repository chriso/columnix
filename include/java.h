#ifndef CX_JAVA_
#define CX_JAVA_

#include <jni.h>

jlong Java_com_columnix_jni_Reader_create(JNIEnv *, jobject, jstring);
jlong Java_com_columnix_jni_Reader_createMatching(JNIEnv *, jobject, jstring,
                                                  jlong);
void Java_com_columnix_jni_Reader_free(JNIEnv *, jobject, jlong);
jstring Java_com_columnix_jni_Reader_getMetadata(JNIEnv *, jobject, jlong);
jint Java_com_columnix_jni_Reader_columnCount(JNIEnv *, jobject, jlong);
jlong Java_com_columnix_jni_Reader_rowCount(JNIEnv *, jobject, jlong);
void Java_com_columnix_jni_Reader_rewind(JNIEnv *, jobject, jlong);
jboolean Java_com_columnix_jni_Reader_next(JNIEnv *, jobject, jlong);
jstring Java_com_columnix_jni_Reader_columnName(JNIEnv *, jobject, jlong, jint);
jint Java_com_columnix_jni_Reader_columnType(JNIEnv *, jobject, jlong, jint);
jint Java_com_columnix_jni_Reader_columnEncoding(JNIEnv *, jobject, jlong,
                                                 jint);
jint Java_com_columnix_jni_Reader_columnCompression(JNIEnv *, jobject, jlong,
                                                    jint);
jboolean Java_com_columnix_jni_Reader_isNull(JNIEnv *, jobject, jlong, jint);
jboolean Java_com_columnix_jni_Reader_getBoolean(JNIEnv *, jobject, jlong,
                                                 jint);
jint Java_com_columnix_jni_Reader_getInt(JNIEnv *, jobject, jlong, jint);
jlong Java_com_columnix_jni_Reader_getLong(JNIEnv *, jobject, jlong, jint);
jstring Java_com_columnix_jni_Reader_getString(JNIEnv *, jobject, jlong, jint);
jbyteArray Java_com_columnix_jni_Reader_getStringBytes(JNIEnv *, jobject, jlong,
                                                       jint);

jlong Java_com_columnix_jni_Writer_create(JNIEnv *, jobject, jstring, jlong);
void Java_com_columnix_jni_Writer_free(JNIEnv *, jobject, jlong);
void Java_com_columnix_jni_Writer_setMetadata(JNIEnv *, jobject, jlong,
                                              jstring);
void Java_com_columnix_jni_Writer_finish(JNIEnv *, jobject, jlong, jboolean);
void Java_com_columnix_jni_Writer_addColumn(JNIEnv *, jobject, jlong, jstring,
                                            jint, jint, jint, jint);
void Java_com_columnix_jni_Writer_putNull(JNIEnv *, jobject, jlong, jint);
void Java_com_columnix_jni_Writer_putBoolean(JNIEnv *, jobject, jlong, jint,
                                             jboolean);
void Java_com_columnix_jni_Writer_putInt(JNIEnv *, jobject, jlong, jint, jint);
void Java_com_columnix_jni_Writer_putLong(JNIEnv *, jobject, jlong, jint,
                                          jlong);
void Java_com_columnix_jni_Writer_putString(JNIEnv *, jobject, jlong, jint,
                                            jstring);

jlong Java_com_columnix_jni_Predicate_negate(JNIEnv *, jobject, jlong);
void Java_com_columnix_jni_Predicate_free(JNIEnv *, jobject, jlong);
jlong Java_com_columnix_jni_Predicate_and(JNIEnv *, jobject, jlongArray);
jlong Java_com_columnix_jni_Predicate_or(JNIEnv *, jobject, jlongArray);
jlong Java_com_columnix_jni_Predicate_isNull(JNIEnv *, jobject, jint);

jlong Java_com_columnix_jni_Predicate_booleanEquals(JNIEnv *, jobject, jint,
                                                    jboolean);

jlong Java_com_columnix_jni_Predicate_intEquals(JNIEnv *, jobject, jint, jint);
jlong Java_com_columnix_jni_Predicate_intGreaterThan(JNIEnv *, jobject, jint,
                                                     jint);
jlong Java_com_columnix_jni_Predicate_intLessThan(JNIEnv *, jobject, jint,
                                                  jint);

jlong Java_com_columnix_jni_Predicate_longEquals(JNIEnv *, jobject, jint,
                                                 jlong);
jlong Java_com_columnix_jni_Predicate_longGreaterThan(JNIEnv *, jobject, jint,
                                                      jlong);
jlong Java_com_columnix_jni_Predicate_longLessThan(JNIEnv *, jobject, jint,
                                                   jlong);

jlong Java_com_columnix_jni_Predicate_stringEquals(JNIEnv *, jobject, jint,
                                                   jstring, jboolean);
jlong Java_com_columnix_jni_Predicate_stringGreaterThan(JNIEnv *, jobject, jint,
                                                        jstring, jboolean);
jlong Java_com_columnix_jni_Predicate_stringLessThan(JNIEnv *, jobject, jint,
                                                     jstring, jboolean);
jlong Java_com_columnix_jni_Predicate_stringContains(JNIEnv *, jobject, jint,
                                                     jstring, jint, jboolean);

#endif
