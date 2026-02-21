#include "gumus.h"

extern int main();

void _start() {
    int result = main();
    exit(result);
}
