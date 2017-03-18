#ifndef CX_JAVA_H_
#define CX_JAVA_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <jni.h>

CX_EXPORT jlong Java_com_columnix_jni_Reader_create(JNIEnv *, jobject, jstring);
CX_EXPORT jlong Java_com_columnix_jni_Reader_createMatching(JNIEnv *, jobject,
                                                            jstring, jlong);
CX_EXPORT void Java_com_columnix_jni_Reader_free(JNIEnv *, jobject, jlong);
CX_EXPORT jstring Java_com_columnix_jni_Reader_getMetadata(JNIEnv *, jobject,
                                                           jlong);
CX_EXPORT jint Java_com_columnix_jni_Reader_columnCount(JNIEnv *, jobject,
                                                        jlong);
CX_EXPORT jlong Java_com_columnix_jni_Reader_rowCount(JNIEnv *, jobject, jlong);
CX_EXPORT void Java_com_columnix_jni_Reader_rewind(JNIEnv *, jobject, jlong);
CX_EXPORT jboolean Java_com_columnix_jni_Reader_next(JNIEnv *, jobject, jlong);
CX_EXPORT jstring Java_com_columnix_jni_Reader_columnName(JNIEnv *, jobject,
                                                          jlong, jint);
CX_EXPORT jint Java_com_columnix_jni_Reader_columnType(JNIEnv *, jobject, jlong,
                                                       jint);
CX_EXPORT jint Java_com_columnix_jni_Reader_columnEncoding(JNIEnv *, jobject,
                                                           jlong, jint);
CX_EXPORT jint Java_com_columnix_jni_Reader_columnCompression(JNIEnv *, jobject,
                                                              jlong, jint);
CX_EXPORT jboolean Java_com_columnix_jni_Reader_isNull(JNIEnv *, jobject, jlong,
                                                       jint);
CX_EXPORT jboolean Java_com_columnix_jni_Reader_getBoolean(JNIEnv *, jobject,
                                                           jlong, jint);
CX_EXPORT jint Java_com_columnix_jni_Reader_getInt(JNIEnv *, jobject, jlong,
                                                   jint);
CX_EXPORT jlong Java_com_columnix_jni_Reader_getLong(JNIEnv *, jobject, jlong,
                                                     jint);
CX_EXPORT jfloat Java_com_columnix_jni_Reader_getFloat(JNIEnv *, jobject, jlong,
                                                       jint);
CX_EXPORT jdouble Java_com_columnix_jni_Reader_getDouble(JNIEnv *, jobject,
                                                         jlong, jint);
CX_EXPORT jstring Java_com_columnix_jni_Reader_getString(JNIEnv *, jobject,
                                                         jlong, jint);
CX_EXPORT jbyteArray Java_com_columnix_jni_Reader_getStringBytes(JNIEnv *,
                                                                 jobject, jlong,
                                                                 jint);

CX_EXPORT jlong Java_com_columnix_jni_Writer_create(JNIEnv *, jobject, jstring,
                                                    jlong);
CX_EXPORT void Java_com_columnix_jni_Writer_free(JNIEnv *, jobject, jlong);
CX_EXPORT void Java_com_columnix_jni_Writer_setMetadata(JNIEnv *, jobject,
                                                        jlong, jstring);
CX_EXPORT void Java_com_columnix_jni_Writer_finish(JNIEnv *, jobject, jlong,
                                                   jboolean);
CX_EXPORT void Java_com_columnix_jni_Writer_addColumn(JNIEnv *, jobject, jlong,
                                                      jstring, jint, jint, jint,
                                                      jint);
CX_EXPORT void Java_com_columnix_jni_Writer_putNull(JNIEnv *, jobject, jlong,
                                                    jint);
CX_EXPORT void Java_com_columnix_jni_Writer_putBoolean(JNIEnv *, jobject, jlong,
                                                       jint, jboolean);
CX_EXPORT void Java_com_columnix_jni_Writer_putInt(JNIEnv *, jobject, jlong,
                                                   jint, jint);
CX_EXPORT void Java_com_columnix_jni_Writer_putLong(JNIEnv *, jobject, jlong,
                                                    jint, jlong);
CX_EXPORT void Java_com_columnix_jni_Writer_putFloat(JNIEnv *, jobject, jlong,
                                                     jint, jfloat);
CX_EXPORT void Java_com_columnix_jni_Writer_putDouble(JNIEnv *, jobject, jlong,
                                                      jint, jdouble);
CX_EXPORT void Java_com_columnix_jni_Writer_putString(JNIEnv *, jobject, jlong,
                                                      jint, jstring);

CX_EXPORT jlong Java_com_columnix_jni_Predicate_negate(JNIEnv *, jobject,
                                                       jlong);
CX_EXPORT void Java_com_columnix_jni_Predicate_free(JNIEnv *, jobject, jlong);
CX_EXPORT jlong Java_com_columnix_jni_Predicate_and(JNIEnv *, jobject,
                                                    jlongArray);
CX_EXPORT jlong Java_com_columnix_jni_Predicate_or(JNIEnv *, jobject,
                                                   jlongArray);
CX_EXPORT jlong Java_com_columnix_jni_Predicate_isNull(JNIEnv *, jobject, jint);

CX_EXPORT jlong Java_com_columnix_jni_Predicate_booleanEquals(JNIEnv *, jobject,
                                                              jint, jboolean);

CX_EXPORT jlong Java_com_columnix_jni_Predicate_intEquals(JNIEnv *, jobject,
                                                          jint, jint);
CX_EXPORT jlong Java_com_columnix_jni_Predicate_intGreaterThan(JNIEnv *,
                                                               jobject, jint,
                                                               jint);
CX_EXPORT jlong Java_com_columnix_jni_Predicate_intLessThan(JNIEnv *, jobject,
                                                            jint, jint);

CX_EXPORT jlong Java_com_columnix_jni_Predicate_longEquals(JNIEnv *, jobject,
                                                           jint, jlong);
CX_EXPORT jlong Java_com_columnix_jni_Predicate_longGreaterThan(JNIEnv *,
                                                                jobject, jint,
                                                                jlong);
CX_EXPORT jlong Java_com_columnix_jni_Predicate_longLessThan(JNIEnv *, jobject,
                                                             jint, jlong);

CX_EXPORT jfloat Java_com_columnix_jni_Predicate_floatEquals(JNIEnv *, jobject,
                                                             jint, jfloat);
CX_EXPORT jfloat Java_com_columnix_jni_Predicate_floatGreaterThan(JNIEnv *,
                                                                  jobject, jint,
                                                                  jfloat);
CX_EXPORT jfloat Java_com_columnix_jni_Predicate_floatLessThan(JNIEnv *,
                                                               jobject, jint,
                                                               jfloat);

CX_EXPORT jdouble Java_com_columnix_jni_Predicate_doubleEquals(JNIEnv *,
                                                               jobject, jint,
                                                               jdouble);
CX_EXPORT jdouble Java_com_columnix_jni_Predicate_doubleGreaterThan(JNIEnv *,
                                                                    jobject,
                                                                    jint,
                                                                    jdouble);
CX_EXPORT jdouble Java_com_columnix_jni_Predicate_doubleLessThan(JNIEnv *,
                                                                 jobject, jint,
                                                                 jdouble);

CX_EXPORT jlong Java_com_columnix_jni_Predicate_stringEquals(JNIEnv *, jobject,
                                                             jint, jstring,
                                                             jboolean);
CX_EXPORT jlong Java_com_columnix_jni_Predicate_stringGreaterThan(JNIEnv *,
                                                                  jobject, jint,
                                                                  jstring,
                                                                  jboolean);
CX_EXPORT jlong Java_com_columnix_jni_Predicate_stringLessThan(JNIEnv *,
                                                               jobject, jint,
                                                               jstring,
                                                               jboolean);
CX_EXPORT jlong Java_com_columnix_jni_Predicate_stringContains(JNIEnv *,
                                                               jobject, jint,
                                                               jstring, jint,
                                                               jboolean);

#ifdef __cplusplus
}
#endif

#endif
