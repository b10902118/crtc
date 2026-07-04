#include "jni.hpp"
#include "log.hpp"

static JNINativeInterface env_table;
static const JNINativeInterface *env_table_ptr = &env_table;
JNIEnv_ptr mock_env = &env_table_ptr;

static JNIInvokeInterface vm_table;
static const JNIInvokeInterface *vm_table_ptr = &vm_table;
JavaVM_ptr mock_vm = &vm_table_ptr;

// Generic stub to safely intercept and log unhandled calls
extern "C" long generic_jni_stub(void *a, void *b, void *c, void *d, void *e,
                                 void *f) {
  // Quietly return 0 to prevent excessive spam, yet prevent crashes
  return 0;
}

// JavaVM Mock Functions
extern "C" jint mock_DestroyJavaVM(void *vm) {
  LOGD("[JNI MOCK] DestroyJavaVM called");
  return 0;
}

extern "C" jint mock_AttachCurrentThread(void *vm, void **env, void *args) {
  LOGD("[JNI MOCK] AttachCurrentThread called, registering Mock JNIEnv");
  *env = (void *)&env_table_ptr;
  return 0;
}

extern "C" jint mock_DetachCurrentThread(void *vm) {
  LOGD("[JNI MOCK] DetachCurrentThread called");
  return 0;
}

extern "C" jint mock_GetEnv(void *vm, void **env, jint version) {
  LOGD("[JNI MOCK] GetEnv called, returning Mock JNIEnv");
  *env = (void *)&env_table_ptr;
  return 0;
}

extern "C" jint mock_AttachCurrentThreadAsDaemon(void *vm, void **env,
                                                 void *args) {
  LOGD("[JNI MOCK] AttachCurrentThreadAsDaemon called");
  *env = (void *)&env_table_ptr;
  return 0;
}

// JNIEnv Mock Functions
extern "C" jint mock_GetVersion(void *env) {
  return 0x00010006; // JNI_VERSION_1_6
}

extern "C" void *mock_FindClass(void *env, const char *name) {
  LOGD("[JNI MOCK] JNIEnv::FindClass called: ", name);
  return (void *)0xDEADBEEF; // Return dummy class handle
}

extern "C" void *mock_GetStaticMethodID(void *env, void *clazz,
                                        const char *name, const char *sig) {
  LOGD("[JNI MOCK] JNIEnv::GetStaticMethodID called: ", name, " (sig: ", sig,
       ")");
  return (void *)0xCAFEBABE; // Return dummy method handle
}

extern "C" void *mock_GetMethodID(void *env, void *clazz, const char *name,
                                  const char *sig) {
  LOGD("[JNI MOCK] JNIEnv::GetMethodID called: ", name, " (sig: ", sig, ")");
  return (void *)0xBAADF00D; // Return dummy method handle
}

extern "C" void *mock_NewStringUTF(void *env, const char *bytes) {
  LOGD("[JNI MOCK] JNIEnv::NewStringUTF called: ", bytes ? bytes : "NULL");
  // Cast the C-string directly to object handle to bypass memory allocation
  return (void *)bytes;
}

extern "C" const char *mock_GetStringUTFChars(void *env, void *jstr,
                                              unsigned char *isCopy) {
  if (isCopy)
    *isCopy = 1;
  return (const char *)jstr; // Cast back our faked string handle
}

extern "C" void mock_ReleaseStringUTFChars(void *env, void *jstr,
                                           const char *chars) {
  // No-op for our static faked strings
}

extern "C" jboolean mock_ExceptionCheck(void *env) {
  return 0; // Return false (no active exceptions)
}

/**
 * Initializes the vtables of the mock structures with safety generic stubs
 * and overrides critical JNI entrypoints.
 */
void init_jni_mock() {
  // Populate with safety stubs
  for (int i = 0; i < 300; ++i) {
    env_table.functions[i] = (void *)generic_jni_stub;
  }
  for (int i = 0; i < 12; ++i) {
    vm_table.functions[i] = (void *)generic_jni_stub;
  }

  // Bind JavaVM interface
  vm_table.functions[3] = (void *)mock_DestroyJavaVM;
  vm_table.functions[4] = (void *)mock_AttachCurrentThread;
  vm_table.functions[5] = (void *)mock_DetachCurrentThread;
  vm_table.functions[6] = (void *)mock_GetEnv;
  vm_table.functions[7] = (void *)mock_AttachCurrentThreadAsDaemon;

  // Bind JNIEnv interface
  env_table.functions[4] = (void *)mock_GetVersion;
  env_table.functions[6] = (void *)mock_FindClass;
  env_table.functions[33] = (void *)mock_GetMethodID;
  env_table.functions[113] = (void *)mock_GetStaticMethodID;
  env_table.functions[167] = (void *)mock_NewStringUTF;
  env_table.functions[169] = (void *)mock_GetStringUTFChars;
  env_table.functions[170] = (void *)mock_ReleaseStringUTFChars;
  env_table.functions[228] = (void *)mock_ExceptionCheck;
}