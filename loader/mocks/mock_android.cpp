#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <pthread.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

static pthread_mutex_t *get_real_mutex(void *bionic_mutex) {
  return (pthread_mutex_t *)bionic_mutex;
}

static pthread_cond_t *get_real_cond(void *bionic_cond) {
  return (pthread_cond_t *)bionic_cond;
}

// Mirror the minimal standard Android NDK Asset Manager structures
struct AAssetManager {
  std::string base_path;
};

struct AAsset {
  FILE *file;
  size_t length;
};

struct AAssetDir {
  // Dummy directory wrapper if needed
  void *dir;
};

extern "C" char __sF[];
extern "C" int *__errno_location(void);
extern "C" int *__errno() { return __errno_location(); }
static short toupper_tab_data[257];
static short tolower_tab_data[257];
static char ctype_data[257];

extern "C" {

#if defined(__i386__)
uintptr_t __stack_chk_guard = 0x000a0d00;

int __fpclassifyd(double x) { return fpclassify(x); }

int __fpclassifyf(float x) { return fpclassify(x); }
#endif


int __isfinite(double d) {
    return std::isfinite(d);
}

int __isfinitef(float f) {
    return std::isfinite(f);
}

#if defined(__arm__)
void* dl_unwind_find_exidx(void* pc, int* pcount) {
    if (pcount) *pcount = 0;
    return nullptr;
}

// implemented in libgcc
long long __divdi3(long long a, long long b);
long long __moddi3(long long a, long long b);

long long __gnu_ldivmod_helper(long long a, long long b, long long* remainder) {
    long long quotient = __divdi3(a, b);
    *remainder = __moddi3(a, b);
    return quotient;
}
#endif

const short *_toupper_tab_ = &toupper_tab_data[1];
const short *_tolower_tab_ = &tolower_tab_data[1];
const char *_ctype_ = &ctype_data[1];

int __system_property_get(const char *name, char *value) {
  if (name && value) {
    std::string prop(name);
    if (prop == "ro.build.version.sdk") {
      value[0] = '2';
      value[1] = '4';
      value[2] = '\0';
      return 2;
    }
    value[0] = '\0';
  }
  return 0;
}

__attribute__((constructor)) void init_ctype_tables() {
  toupper_tab_data[0] = -1; // EOF
  tolower_tab_data[0] = -1; // EOF
  for (int i = 0; i < 256; ++i) {
    toupper_tab_data[i + 1] = (i >= 'a' && i <= 'z') ? (i - 32) : i;
    tolower_tab_data[i + 1] = (i >= 'A' && i <= 'Z') ? (i + 32) : i;
  }

  ctype_data[0] = 0; // EOF
  for (int i = 0; i < 256; ++i) {
    char mask = 0;
    if (i >= 'A' && i <= 'Z')
      mask |= 0x01; // _U
    if (i >= 'a' && i <= 'z')
      mask |= 0x02; // _L
    if (i >= '0' && i <= '9')
      mask |= 0x04; // _N
    if (i == ' ' || i == '\t' || i == '\r' || i == '\n' || i == '\v' ||
        i == '\f')
      mask |= 0x08; // _S
    if ((i >= 33 && i <= 47) || (i >= 58 && i <= 64) || (i >= 91 && i <= 96) ||
        (i >= 123 && i <= 126))
      mask |= 0x10; // _P
    if (i < 32 || i == 127)
      mask |= 0x20; // _C
    if ((i >= '0' && i <= '9') || (i >= 'A' && i <= 'F') ||
        (i >= 'a' && i <= 'f'))
      mask |= 0x40; // _X
    if (i == ' ' || i == '\t')
      mask |= 0x80; // _B
    ctype_data[i + 1] = mask;
  }
}

AAssetManager *AAssetManager_fromJava(void *env, void *assetManager) {
  AAssetManager *mgr = new AAssetManager();
  mgr->base_path = "assets"; // Redirects assets to local directory
  return mgr;
}

AAsset *AAssetManager_open(AAssetManager *mgr, const char *filename, int mode) {
  printf("AAssetManager_open called\n");
  std::string full_path = (mgr ? mgr->base_path : "assets") + "/" + filename;
  FILE *f = fopen(full_path.c_str(), "rb");
  if (!f) {
    // Try fallback to current directory
    f = fopen(filename, "rb");
    if (!f)
      return nullptr;
  }

  fseek(f, 0, SEEK_END);
  size_t len = ftell(f);
  fseek(f, 0, SEEK_SET);

  AAsset *asset = new AAsset();
  asset->file = f;
  asset->length = len;
  return asset;
}

int AAsset_read(AAsset *asset, void *buf, size_t count) {
  if (!asset || !asset->file)
    return -1;
  return fread(buf, 1, count, asset->file);
}

void AAsset_close(AAsset *asset) {
  if (asset) {
    if (asset->file)
      fclose(asset->file);
    delete asset;
  }
}

off_t AAsset_getLength(AAsset *asset) { return asset ? asset->length : 0; }

off_t AAsset_getRemainingLength(AAsset *asset) {
  if (!asset || !asset->file)
    return 0;
  off_t curr = ftell(asset->file);
  return asset->length - curr;
}

off_t AAsset_seek(AAsset *asset, off_t offset, int whence) {
  if (!asset || !asset->file)
    return -1;
  fseek(asset->file, offset, whence);
  return ftell(asset->file);
}

const void *AAsset_getBuffer(AAsset *asset) {
  // Not heavily used, but return NULL or allocate in memory if needed
  return nullptr;
}

AAssetDir *AAssetManager_openDir(AAssetManager *mgr, const char *dirName) {
  return nullptr;
}

const char *AAssetDir_getNextFileName(AAssetDir *assetDir) { return nullptr; }

void AAssetDir_close(AAssetDir *assetDir) {
  // No-op
}

int pthread_mutex_init(pthread_mutex_t *mutex,
                       const pthread_mutexattr_t *attr) {
  return 0;
}

int pthread_mutex_lock(pthread_mutex_t *mutex) { return 0; }

int pthread_mutex_trylock(pthread_mutex_t *mutex) { return 0; }

int pthread_mutex_unlock(pthread_mutex_t *mutex) { return 0; }

int pthread_mutex_destroy(pthread_mutex_t *mutex) { return 0; }

int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr) {
  return 0;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
  return 0;
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                           const struct timespec *abstime) {
  return 0;
}

int pthread_cond_signal(pthread_cond_t *cond) { return 0; }

int pthread_cond_broadcast(pthread_cond_t *cond) { return 0; }

int pthread_cond_destroy(pthread_cond_t *cond) { return 0; }
}