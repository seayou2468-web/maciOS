#include "litehook.h"
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <mach/mach.h>

void litehook_hook_function(void *target, void *replacement, void **original) {
    if (!target || !replacement) return;

    if (original) *original = target;

    vm_address_t addr = (vm_address_t)target;
    vm_size_t size = 16;

    kern_return_t kr = vm_protect(mach_task_self(), addr, size, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY);
    if (kr != KERN_SUCCESS) return;

    uint32_t patch[] = {
        0x58000050,
        0xd61f0200,
        (uint32_t)((uintptr_t)replacement & 0xFFFFFFFF),
        (uint32_t)((uintptr_t)replacement >> 32)
    };

    memcpy(target, patch, sizeof(patch));

    vm_protect(mach_task_self(), addr, size, FALSE, VM_PROT_READ | VM_PROT_EXEC);
}
