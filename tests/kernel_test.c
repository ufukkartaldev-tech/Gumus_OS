#include "test_framework.h"
#include "../src/kernel/core/string.h"
#include "../src/kernel/core/math.h"

int test_string_len() {
    ASSERT(strlen("") == 0, "Empty string length should be 0");
    ASSERT(strlen("abc") == 3, "Length of 'abc' should be 3");
    return 1;
}

int test_string_cmp() {
    ASSERT(strcmp("abc", "abc") == 0, "Strings should be equal");
    ASSERT(strcmp("abc", "abd") < 0, "abc < abd");
    ASSERT(strcmp("abc", "aba") > 0, "abc > aba");
    ASSERT(strncmp("abcd", "abce", 3) == 0, "First 3 chars should be equal");
    return 1;
}

int test_string_mem() {
    char buf[10];
    memset(buf, 'A', 5);
    ASSERT(buf[0] == 'A' && buf[4] == 'A', "Memset failed");
    
    char src[] = "test";
    char dst[10];
    memcpy(dst, src, 5);
    ASSERT(strcmp(dst, "test") == 0, "Memcpy failed");
    return 1;
}

int test_math_basic() {
    // We use a small epsilon for floating point comparison if needed, 
    // but here we check basic properties
    ASSERT(sqrt(4.0) == 2.0, "sqrt(4) should be 2");
    ASSERT(sqrt(0.0) == 0.0, "sqrt(0) should be 0");
    
    // sin(0) = 0
    double s0 = sin(0.0);
    ASSERT(s0 > -0.001 && s0 < 0.001, "sin(0) should be approx 0");
    
    return 1;
}

void kernel_main() {
    TEST_HEADER("Kernel Core Tests");
    
    RUN_TEST(test_string_len, "String Length");
    RUN_TEST(test_string_cmp, "String Comparison");
    RUN_TEST(test_string_mem, "Memory Operations");
    RUN_TEST(test_math_basic, "Math Functions");
    
    TEST_FOOTER();
    
    while(1) {
        __asm__ volatile("hlt");
    }
}
