#pragma once

// =========================================================================
// Custom Lightweight JNI Mocking Environment (No jni.h required)
// =========================================================================

typedef unsigned char jboolean;
typedef int jint;

struct JNINativeInterface {
  void *functions[300];
};

struct JNIInvokeInterface {
  void *functions[12];
};

typedef const JNINativeInterface **JNIEnv_ptr;
typedef const JNIInvokeInterface **JavaVM_ptr;
typedef void *jclass_ptr;
typedef void *jstring_ptr;

// Expose the mock environment pointers to other translation units
extern JNIEnv_ptr mock_env;
extern JavaVM_ptr mock_vm;

/**
 * Initializes the vtables of the mock structures with safety generic stubs
 * and overrides critical JNI entrypoints.
 */
void init_jni_mock();
