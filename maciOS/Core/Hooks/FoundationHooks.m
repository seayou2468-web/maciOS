#import <Foundation/Foundation.h>
#import <objc/runtime.h>
#import "litehook/litehook.h"

// Original function pointers
static NSString* (*orig_NSHomeDirectory)(void);

NSString* hooked_NSHomeDirectory(void) {
    const char* home = getenv("LC_HOME_PATH");
    if (home) {
        return [NSString stringWithUTF8String:home];
    }
    return orig_NSHomeDirectory();
}

@interface NSBundle (Hook)
+ (NSBundle *)hooked_mainBundle;
@end

@implementation NSBundle (Hook)
+ (NSBundle *)hooked_mainBundle {
    // In a real implementation, we would return a bundle representing the macOS app being executed.
    // For now, we return the current app's bundle.
    return [self hooked_mainBundle]; // This will be swapped back with Method Swizzling
}
@end

void setup_foundation_hooks() {
    // Hook C-level Foundation functions
    void* home_ptr = dlsym(RTLD_DEFAULT, "NSHomeDirectory");
    if (home_ptr) litehook_hook_function(home_ptr, hooked_NSHomeDirectory, (void**)&orig_NSHomeDirectory);

    // Method Swizzling for NSBundle.mainBundle
    Method original = class_getClassMethod([NSBundle class], @selector(mainBundle));
    Method replacement = class_getClassMethod([NSBundle class], @selector(hooked_mainBundle));
    method_exchangeImplementations(original, replacement);
}
