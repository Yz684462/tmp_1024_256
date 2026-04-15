#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// 关键：显式做 ones[i] * data[i] 累加
float dot_product_like(float *data, float *ones, size_t n) {
    float sum = 0.0f;
    for (size_t i = 0; i < n; ++i) {
        sum += ones[i] * data[i];
    }
    return sum;
}

int main() {
    const size_t n = 1024;
    float *data = (float *)malloc(n * sizeof(float));
    float *ones = (float *)malloc(n * sizeof(float));

    for (size_t i = 0; i < n; ++i) {
        data[i] = (float)(i + 1);  // 1.0f, 2.0f, ...
        ones[i] = 1.0f;            // 全 1
    }

    float total = dot_product_like(data, ones, n);
    printf("Sum = %.2f\n", total);  // 应为 524800.00

    free(data);
    free(ones);
    return 0;
}