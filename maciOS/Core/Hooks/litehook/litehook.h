#ifndef LITEHOOK_H
#define LITEHOOK_H

#include <stddef.h>

void litehook_hook_function(void *target, void *replacement, void **original);

#endif
