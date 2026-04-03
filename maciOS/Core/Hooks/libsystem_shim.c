#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/syscall.h>
#include <sys/sysctl.h>
#include "litehook/litehook.h"

// Original function pointers
typedef int (*orig_open_t)(const char*, int, mode_t);
static orig_open_t orig_open = NULL;

typedef int (*orig_stat_t)(const char*, struct stat*);
static orig_stat_t orig_stat = NULL;

typedef int (*orig_syscall_t)(int, ...);
static orig_syscall_t orig_syscall = NULL;

typedef int (*orig_sysctl_t)(int*, u_int, void*, size_t*, void*, size_t);
static orig_sysctl_t orig_sysctl = NULL;

typedef void* (*orig_dlopen_t)(const char*, int);
static orig_dlopen_t orig_dlopen = NULL;

const char* redirect_path(const char* path) {
    if (!path) return path;
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

int hooked_syscall(int number, ...) {
    if (!orig_syscall) orig_syscall = dlsym(RTLD_NEXT, "syscall");

    va_list args;
    va_start(args, number);

    // MacOS and iOS syscall numbers are mostly compatible,
    // but some specific ones may need mapping here.
    // Example: redirecting path-based syscalls
    if (number == SYS_open) {
        const char* path = va_arg(args, const char*);
        int oflag = va_arg(args, int);
        mode_t mode = va_arg(args, mode_t);
        va_end(args);
        return hooked_open(path, oflag, mode);
    }

    // For others, we'll need to pass through carefully.
    // This is complex for variadic syscalls, so we'll handle common ones.

    va_end(args);
    // Generic passthrough if possible (simplified)
    return -1; // ENOSYS or handle specifically
}

int hooked_sysctl(int *name, u_int namelen, void *oldp, size_t *oldlenp, void *newp, size_t newlen) {
    if (!orig_sysctl) orig_sysctl = dlsym(RTLD_NEXT, "sysctl");

    // Virtualize macOS specific sysctls (e.g., machine name, OS version)
    if (namelen >= 2 && name[0] == CTL_KERN) {
        if (name[1] == KERN_OSTYPE) {
            if (oldp) strcpy((char*)oldp, "Darwin");
            if (oldlenp) *oldlenp = 7;
            return 0;
        }
        if (name[1] == KERN_OSRELEASE) {
            if (oldp) strcpy((char*)oldp, "24.0.0"); // macOS 15 release
            if (oldlenp) *oldlenp = 7;
            return 0;
        }
    }

    return orig_sysctl(name, namelen, oldp, oldlenp, newp, newlen);
}

void* hooked_dlopen(const char* path, int mode) {
    if (!orig_dlopen) orig_dlopen = dlsym(RTLD_NEXT, "dlopen");
    const char* target = redirect_path(path);
    return orig_dlopen(target, mode);
}

void setup_libsystem_hooks() {
    void* open_ptr = dlsym(RTLD_DEFAULT, "open");
    void* stat_ptr = dlsym(RTLD_DEFAULT, "stat");
    void* syscall_ptr = dlsym(RTLD_DEFAULT, "syscall");
    void* sysctl_ptr = dlsym(RTLD_DEFAULT, "sysctl");
    void* dlopen_ptr = dlsym(RTLD_DEFAULT, "dlopen");

    if (open_ptr) litehook_hook_function(open_ptr, hooked_open, (void**)&orig_open);
    if (stat_ptr) litehook_hook_function(stat_ptr, hooked_stat, (void**)&orig_stat);
    if (syscall_ptr) litehook_hook_function(syscall_ptr, hooked_syscall, (void**)&orig_syscall);
    if (sysctl_ptr) litehook_hook_function(sysctl_ptr, hooked_sysctl, (void**)&orig_sysctl);
    if (dlopen_ptr) litehook_hook_function(dlopen_ptr, hooked_dlopen, (void**)&orig_dlopen);
}
