#include <stdio.h>

int main(int argc, char** argv) {
    printf("Hello from macOS CLI running on iOS!\n");
    for (int i = 0; i < argc; i++) {
        printf("Arg[%d]: %s\n", i, argv[i]);
    }
    return 0;
}
