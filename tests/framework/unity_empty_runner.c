#include <stdio.h>

#include "unity.h"

void setUp(void) {
}

void tearDown(void) {
}

int main(void) {
    puts("框架冒烟，零个生产测试。");
    UNITY_BEGIN();
    return UNITY_END();
}
