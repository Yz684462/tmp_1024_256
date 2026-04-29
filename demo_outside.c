#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>    // getpid()

void call_migration(){
    pid_t pid = getpid();
    FILE* file = NULL;
    char proc_path_ai[] = "/proc/set_ai_thread";
    file = fopen(proc_path_ai, "w");
    if (file == NULL) {
        // 打开文件失败，通常意味着没有权限或文件不存在
        perror("错误: 无法打开 /proc/set_ai_thread");
        return;
    }
    if (fprintf(file, "%d", -pid) < 0) {
        // 写入操作失败
        perror("错误: 写入 PID 到 /proc/set_ai_thread 失败");
        fclose(file);
        return;
    }

    // 关闭文件句柄
    if (fclose(file) != 0) {
        // 关闭文件失败
        perror("错误: 关闭 /proc/set_ai_thread 文件失败");
        return; // 虽然写入已成功，但关闭失败也应视为一种错误
    }
}

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
    
    call_migration();
    while (1) {
        float total = dot_product_like(data, ones, n);
        printf("Sum = %.2f\n", total);  // 应为 524800.00
        sleep(1);
    }

    free(data);
    free(ones);
    return 0;
}