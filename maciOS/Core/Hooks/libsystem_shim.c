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

typedef int (*orig_access_t)(const char*, int);
static orig_access_t orig_access = NULL;

typedef int (*orig_chdir_t)(const char*);
static orig_chdir_t orig_chdir = NULL;

typedef char* (*orig_getcwd_t)(char*, size_t);
static orig_getcwd_t orig_getcwd = NULL;

typedef int (*orig_mkdir_t)(const char*, mode_t);
static orig_mkdir_t orig_mkdir = NULL;

typedef int (*orig_unlink_t)(const char*);
static orig_unlink_t orig_unlink = NULL;

typedef void* (*orig_dlopen_t)(const char*, int);
static orig_dlopen_t orig_dlopen = NULL;

const char* redirect_path(const char* path) {
    if (!path) return path;
    if (strstr(path, "/Documents/macroot/")) return path;

    if (strncmp(path, "/usr/lib/", 9) == 0 || strncmp(path, "/bin/", 5) == 0 || strncmp(path, "/System/", 8) == 0 || strncmp(path, "/private/", 9) == 0) {
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

int hooked_access(const char* path, int mode) {
    const char* target = redirect_path(path);
    if (!orig_access) orig_access = dlsym(RTLD_NEXT, "access");
    return orig_access(target, mode);
}

int hooked_chdir(const char* path) {
    const char* target = redirect_path(path);
    if (!orig_chdir) orig_chdir = dlsym(RTLD_NEXT, "chdir");
    return orig_chdir(target);
}

int hooked_mkdir(const char* path, mode_t mode) {
    const char* target = redirect_path(path);
    if (!orig_mkdir) orig_mkdir = dlsym(RTLD_NEXT, "mkdir");
    return orig_mkdir(target, mode);
}

int hooked_unlink(const char* path) {
    const char* target = redirect_path(path);
    if (!orig_unlink) orig_unlink = dlsym(RTLD_NEXT, "unlink");
    return orig_unlink(target);
}

void* hooked_dlopen(const char* path, int mode) {
    if (!orig_dlopen) orig_dlopen = dlsym(RTLD_NEXT, "dlopen");
    const char* target = redirect_path(path);
    return orig_dlopen(target, mode);
}

void setup_libsystem_hooks() {
    void* open_ptr = dlsym(RTLD_DEFAULT, "open");
    void* stat_ptr = dlsym(RTLD_DEFAULT, "stat");
    void* access_ptr = dlsym(RTLD_DEFAULT, "access");
    void* chdir_ptr = dlsym(RTLD_DEFAULT, "chdir");
    void* mkdir_ptr = dlsym(RTLD_DEFAULT, "mkdir");
    void* unlink_ptr = dlsym(RTLD_DEFAULT, "unlink");
    void* dlopen_ptr = dlsym(RTLD_DEFAULT, "dlopen");

    if (open_ptr) litehook_hook_function(open_ptr, hooked_open, (void**)&orig_open);
    if (stat_ptr) litehook_hook_function(stat_ptr, hooked_stat, (void**)&orig_stat);
    if (access_ptr) litehook_hook_function(access_ptr, hooked_access, (void**)&orig_access);
    if (chdir_ptr) litehook_hook_function(chdir_ptr, hooked_chdir, (void**)&orig_chdir);
    if (mkdir_ptr) litehook_hook_function(mkdir_ptr, hooked_mkdir, (void**)&orig_mkdir);
    if (unlink_ptr) litehook_hook_function(unlink_ptr, hooked_unlink, (void**)&orig_unlink);
    if (dlopen_ptr) litehook_hook_function(dlopen_ptr, hooked_dlopen, (void**)&orig_dlopen);
}
