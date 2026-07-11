#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Bionic 32-bit standard I/O structure array
char __sF[3 * 128] = {0};

static FILE *(*real_fopen)(const char *, const char *);
static size_t (*real_fwrite)(const void *, size_t, size_t, FILE *);
static int (*real_fputs)(const char *, FILE *);
static int (*real_fputc)(int, FILE *);
static int (*real_fflush)(FILE *);
static int (*real_fclose)(FILE *);
static size_t (*real_fread)(void *, size_t, size_t, FILE *);
static int (*real_fgetc)(FILE *);
static char *(*real_fgets)(char *, int, FILE *);
static int (*real_fileno)(FILE *);
static int (*real_ferror)(FILE *);
static int (*real_feof)(FILE *);
static void (*real_clearerr)(FILE *);
static int (*real_vfprintf)(FILE *, const char *, va_list);

__attribute__((constructor)) static void init_real_functions() {
  real_fopen =
      (FILE * (*)(const char *, const char *)) dlsym(RTLD_NEXT, "fopen");
  real_fwrite = (size_t (*)(const void *, size_t, size_t, FILE *))dlsym(
      RTLD_NEXT, "fwrite");
  real_fputs = (int (*)(const char *, FILE *))dlsym(RTLD_NEXT, "fputs");
  real_fputc = (int (*)(int, FILE *))dlsym(RTLD_NEXT, "fputc");
  real_fflush = (int (*)(FILE *))dlsym(RTLD_NEXT, "fflush");
  real_fclose = (int (*)(FILE *))dlsym(RTLD_NEXT, "fclose");
  real_fread =
      (size_t (*)(void *, size_t, size_t, FILE *))dlsym(RTLD_NEXT, "fread");
  real_fgetc = (int (*)(FILE *))dlsym(RTLD_NEXT, "fgetc");
  real_fgets = (char *(*)(char *, int, FILE *))dlsym(RTLD_NEXT, "fgets");
  real_fileno = (int (*)(FILE *))dlsym(RTLD_NEXT, "fileno");
  real_ferror = (int (*)(FILE *))dlsym(RTLD_NEXT, "ferror");
  real_feof = (int (*)(FILE *))dlsym(RTLD_NEXT, "feof");
  real_clearerr = (void (*)(FILE *))dlsym(RTLD_NEXT, "clearerr");
  real_vfprintf =
      (int (*)(FILE *, const char *, va_list))dlsym(RTLD_NEXT, "vfprintf");
}

static FILE *map_stream(FILE *stream) {
  char *p = (char *)stream;
  if (p >= __sF && p < __sF + sizeof(__sF)) {
    size_t offset = p - __sF;
    if (offset < 128)
      return stdin;
    if (offset < 256)
      return stdout;
    return stderr;
  }
  return stream;
}

FILE *fopen(const char *pathname, const char *mode) {
  /*
  // swfcache doesn't make initial loading faster
  // but might make battle faster??
  if (strncmp(pathname, "/swfcache", 9) == 0) {
    pathname = pathname + 1;
  }
  */
  if (mode && (strchr(mode, 'w') != NULL || strchr(mode, 'a') != NULL)) {
    pathname = "/dev/null";
  }
  // fprintf(stderr, "%s %s %s\n", "[fopen]", mode, pathname);
  return real_fopen(pathname, mode);
}

size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream) {
  return real_fwrite(ptr, size, nmemb, map_stream(stream));
}

int fputs(const char *s, FILE *stream) {
  return real_fputs(s, map_stream(stream));
}

int fputc(int c, FILE *stream) { return real_fputc(c, map_stream(stream)); }

int fflush(FILE *stream) { return real_fflush(map_stream(stream)); }

int fclose(FILE *stream) {
  FILE *mapped = map_stream(stream);
  if (mapped == stdin || mapped == stdout || mapped == stderr)
    return 0;
  return real_fclose(mapped);
}

size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
  return real_fread(ptr, size, nmemb, map_stream(stream));
}

int fgetc(FILE *stream) { return real_fgetc(map_stream(stream)); }

char *fgets(char *s, int size, FILE *stream) {
  return real_fgets(s, size, map_stream(stream));
}

int fileno(FILE *stream) { return real_fileno(map_stream(stream)); }

int ferror(FILE *stream) { return real_ferror(map_stream(stream)); }

int feof(FILE *stream) { return real_feof(map_stream(stream)); }

void clearerr(FILE *stream) { real_clearerr(map_stream(stream)); }

int vfprintf(FILE *stream, const char *format, va_list ap) {
  return real_vfprintf(map_stream(stream), format, ap);
}

int fprintf(FILE *stream, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int ret = vfprintf(map_stream(stream), format, ap);
  va_end(ap);
  return ret;
}
