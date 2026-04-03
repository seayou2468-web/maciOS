#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include "litehook/litehook.h"

// Original function pointers
typedef int (*orig_open_t)(const char*, int, mode_t);
static orig_open_t orig_open = NULL;

typedef int (*orig_stat_t)(const char*, struct stat*);
static orig_stat_t orig_stat = NULL;

// MacOS specific symbols or versions often missing in iOS or behaving differently
// This shim will expand as more apps are tested.

const char* redirect_path(const char* path) {
    if (!path) return path;

    // Check if path is already redirected
    if (strstr(path, "/Documents/macroot/")) return path;

    if (strncmp(path, "/usr/lib/", 9) == 0 || strncmp(path, "/bin/", 5) == 0 || strncmp(path, "/System/", 8) == 0) {
        static char redirected[1024];
        const char* home = getenv("LC_HOME_PATH");
        if (home) {
            snprintf(redirected, sizeof(redirected), "%s/Documents/macroot%s", home, path);
            return redirected;
        }
    }
    return path;
}

int hooked_open(const char* path, int oflag, ...) {
    const char* target = redirect_path(path);
    if (!orig_open) orig_open = dlsym(RTLD_NEXT, "open");

    mode_t mode = 0;
    if (oflag & O_CREAT) {
        va_list args;
        va_start(args, oflag);
        mode = va_arg(args, mode_t);
        va_end(args);
    }

    return orig_open(target, oflag, mode);
}

int hooked_stat(const char* path, struct stat* buf) {
    const char* target = redirect_path(path);
    if (!orig_stat) orig_stat = dlsym(RTLD_NEXT, "stat");
    return orig_stat(target, buf);
}

// macOS specific __error function might need shimming
int* hooked_error(void) {
    static int dummy_errno = 0;
    return &dummy_errno;
}

void setup_libsystem_hooks() {
    void* open_ptr = dlsym(RTLD_DEFAULT, "open");
    void* stat_ptr = dlsym(RTLD_DEFAULT, "stat");

    if (open_ptr) litehook_hook_function(open_ptr, hooked_open, (void**)&orig_open);
    if (stat_ptr) litehook_hook_function(stat_ptr, hooked_stat, (void**)&orig_stat);

    // Add more hooks for libSystem symbols here
}
