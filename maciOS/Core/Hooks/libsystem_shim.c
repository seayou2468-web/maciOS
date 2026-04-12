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
#include <sys/ioctl.h>
#include <termios.h>
#include <dirent.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <signal.h>
#include "litehook/litehook.h"

// Original function pointers
typedef int (*orig_open_t)(const char*, int, mode_t);
static orig_open_t orig_open = NULL;

typedef int (*orig_stat_t)(const char*, struct stat*);
static orig_stat_t orig_stat = NULL;

typedef int (*orig_ioctl_t)(int, unsigned long, ...);
static orig_ioctl_t orig_ioctl = NULL;

typedef int (*orig_tcgetattr_t)(int, struct termios*);
static orig_tcgetattr_t orig_tcgetattr = NULL;

typedef int (*orig_tcsetattr_t)(int, int, const struct termios*);
static orig_tcsetattr_t orig_tcsetattr = NULL;

typedef DIR* (*orig_opendir_t)(const char*);
static orig_opendir_t orig_opendir = NULL;

typedef int (*orig_chmod_t)(const char*, mode_t);
static orig_chmod_t orig_chmod = NULL;

typedef void* (*orig_dlsym_t)(void*, const char*);
static orig_dlsym_t orig_dlsym = NULL;

const char* redirect_path(const char* path) {
    if (!path) return path;
    if (strstr(path, "/Documents/macroot/")) return path;

    if (strncmp(path, "/usr/lib/", 9) == 0 || strncmp(path, "/bin/", 5) == 0 || strncmp(path, "/System/", 8) == 0 || strncmp(path, "/private/", 9) == 0 || strncmp(path, "/etc/", 5) == 0 || strncmp(path, "/usr/bin/", 9) == 0) {
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

int hooked_ioctl(int fd, unsigned long request, ...) {
    if (!orig_ioctl) orig_ioctl = dlsym(RTLD_NEXT, "ioctl");
    va_list args;
    va_start(args, request);
    void* argp = va_arg(args, void*);
    va_end(args);
    if (request == TIOCGWINSZ) {
        struct winsize* ws = (struct winsize*)argp;
        ws->ws_row = 24;
        ws->ws_col = 80;
        ws->ws_xpixel = 0;
        ws->ws_ypixel = 0;
        return 0;
    }
    return orig_ioctl(fd, request, argp);
}

int hooked_tcgetattr(int fd, struct termios* termios_p) {
    if (!orig_tcgetattr) orig_tcgetattr = dlsym(RTLD_NEXT, "tcgetattr");
    return orig_tcgetattr(fd, termios_p);
}

int hooked_tcsetattr(int fd, int optional_actions, const struct termios* termios_p) {
    if (!orig_tcsetattr) orig_tcsetattr = dlsym(RTLD_NEXT, "tcsetattr");
    return orig_tcsetattr(fd, optional_actions, termios_p);
}

DIR* hooked_opendir(const char* name) {
    const char* target = redirect_path(name);
    if (!orig_opendir) orig_opendir = dlsym(RTLD_NEXT, "opendir");
    return orig_opendir(target);
}

int hooked_chmod(const char* path, mode_t mode) {
    const char* target = redirect_path(path);
    if (!orig_chmod) orig_chmod = dlsym(RTLD_NEXT, "chmod");
    return orig_chmod(target, mode);
}

void* hooked_dlsym(void* handle, const char* symbol) {
    if (!orig_dlsym) orig_dlsym = dlsym(RTLD_NEXT, "dlsym");
    // Intercept macOS specific symbols if needed
    return orig_dlsym(handle, symbol);
}

uid_t hooked_getuid(void) { return 501; }
uid_t hooked_getgid(void) { return 20; }

void setup_libsystem_hooks() {
    void* open_ptr = dlsym(RTLD_DEFAULT, "open");
    void* stat_ptr = dlsym(RTLD_DEFAULT, "stat");
    void* ioctl_ptr = dlsym(RTLD_DEFAULT, "ioctl");
    void* tcgetattr_ptr = dlsym(RTLD_DEFAULT, "tcgetattr");
    void* tcsetattr_ptr = dlsym(RTLD_DEFAULT, "tcsetattr");
    void* opendir_ptr = dlsym(RTLD_DEFAULT, "opendir");
    void* chmod_ptr = dlsym(RTLD_DEFAULT, "chmod");
    void* dlsym_ptr = dlsym(RTLD_DEFAULT, "dlsym");
    void* getuid_ptr = dlsym(RTLD_DEFAULT, "getuid");

    if (open_ptr) litehook_hook_function(open_ptr, hooked_open, (void**)&orig_open);
    if (stat_ptr) litehook_hook_function(stat_ptr, hooked_stat, (void**)&orig_stat);
    if (ioctl_ptr) litehook_hook_function(ioctl_ptr, hooked_ioctl, (void**)&orig_ioctl);
    if (tcgetattr_ptr) litehook_hook_function(tcgetattr_ptr, hooked_tcgetattr, (void**)&orig_tcgetattr);
    if (tcsetattr_ptr) litehook_hook_function(tcsetattr_ptr, hooked_tcsetattr, (void**)&orig_tcsetattr);
    if (opendir_ptr) litehook_hook_function(opendir_ptr, hooked_opendir, (void**)&orig_opendir);
    if (chmod_ptr) litehook_hook_function(chmod_ptr, hooked_chmod, (void**)&orig_chmod);
    if (dlsym_ptr) litehook_hook_function(dlsym_ptr, hooked_dlsym, (void**)&orig_dlsym);
    if (getuid_ptr) litehook_hook_function(getuid_ptr, hooked_getuid, NULL);
}
