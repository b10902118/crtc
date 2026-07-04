#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cmath>

extern "C" {

int __android_log_write(int prio, const char* tag, const char* text) {
    return printf("[%s] %s\n", tag ? tag : "LOG", text);
}

int __android_log_print(int prio, const char* tag, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    printf("[%s] ", tag ? tag : "LOG");
    int ret = vprintf(fmt, ap);
    printf("\n");
    va_end(ap);
    return ret;
}

int __android_log_vprint(int prio, const char* tag, const char* fmt, va_list ap) {
    printf("[%s] ", tag ? tag : "LOG");
    int ret = vprintf(fmt, ap);
    printf("\n");
    return ret;
}

void __assert2(const char* file, int line, const char* function, const char* msg) {
    fprintf(stderr, "[ASSERT ERROR] %s:%d in %s: %s\n", file, line, function, msg);
    abort();
}

}
