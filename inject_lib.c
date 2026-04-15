// inject_lib.c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <ucontext.h>
#include <sched.h>
#include <pthread.h>
#include <linux/ptrace.h>
#include <string.h>
#define RISCV_V_MAGIC		0x53465457

// --- Configuration Macros ---

#define VLEN_BITS 1024
#define VLEN_BYTES (VLEN_BITS / 8)
#define ELEN 32
#define SEW_BYTES (ELEN / 8)

#define SIM_VLEN_BITS 256
#define SIM_VLEN_BYTES (SIM_VLEN_BITS / 8)
#define LANES_PER_SIM (VLEN_BYTES / SIM_VLEN_BYTES)

// --- Global State ---
static uint8_t* g_simulated_cpu_state = NULL;

static uint32_t original_ins_a = 0;
static uint32_t original_ins_b = 0;
static uint32_t original_ins_d = 0;

static uintptr_t patch_point_a = 0;
static uintptr_t patch_point_b = 0;
static uintptr_t patch_point_d = 0;
static uintptr_t patch_point_c = 0;


static void* target_func_ptr = NULL;

static void* translated_handle_1 = NULL;
static void* translated_handle_2 = NULL;

// --- Forward Declarations ---
uintptr_t get_func_addr(const char* func_name);
uintptr_t get_func_addr_in_file(void* translated_handle, const char* func_name, const char* file_path);

void my_ebreak_handler(int sig, siginfo_t *info, void *context);
void patch_code(uintptr_t addr);
void restore_code(uintptr_t addr);

void a_handle(ucontext_t *uc);
void b_handle(ucontext_t *uc, void* translated_handle,const char* func_name,const char* file_name);

struct __riscv_v_ext_state * get_vector_context(ucontext_t *uc);
void save_vector_states(ucontext_t *uc);

// 用于切换和查询cpu集合（A100和X100之间切换，但是目前demo里是用标量模拟的，所以没有调用这两个函数）
void switch_cpu_set(char core_type);
void query_cpu();

// --- Signal Handler for EBREAK ---
void my_ebreak_handler(int sig, siginfo_t *info, void *context) {
    ucontext_t *uc = (ucontext_t *)context;
    uintptr_t fault_pc = (uintptr_t)info->si_addr;
    if (fault_pc == patch_point_a) {
        printf("\n[SIGNAL_HANDLER] >>> Caught SIGTRAP (from EBREAK) <<<\n");
        printf("[SIGNAL_HANDLER] Hit patch point A (0x%lx). Starting migration sequence.\n", fault_pc);
        
        // 1. 恢复原指令
        restore_code(patch_point_a);
        
        // 2. 执行 a_handle
        a_handle(uc);

        // 3. 将 PC 设置为 A 点，继续执行原代码
        uc->uc_mcontext.__gregs[REG_PC] = patch_point_a;
        printf("[SIGNAL_HANDLER] Set PC to %p\n", (void*)uc->uc_mcontext.__gregs[REG_PC]);
    } 
    else if (fault_pc == patch_point_b) {
        printf("\n[SIGNAL_HANDLER] >>> Caught SIGTRAP (from EBREAK) <<<\n");
        printf("[SIGNAL_HANDLER] Hit patch point B (0x%lx). Calling b_handle.\n", fault_pc);
        
        // 1. 恢复原指令
        restore_code(patch_point_b);

        // 2. 执行 b_handle
        b_handle(uc,translated_handle_1, "translated_function_1" ,"translate_part1.so");

        // 3. 将 PC 设置为 C 点，继续执行原代码
        uc->uc_mcontext.__gregs[REG_PC] = patch_point_c;
        printf("[SIGNAL_HANDLER] Set PC to C: %p\n", (void*)uc->uc_mcontext.__gregs[REG_PC]);
    }
    else if (fault_pc == patch_point_d) {
        printf("\n[SIGNAL_HANDLER] >>> Caught SIGTRAP (from EBREAK) <<<\n");
        printf("[SIGNAL_HANDLER] Hit patch point D (0x%lx). Calling d_handle.\n", fault_pc);
        
        // 1. 调用处理函数（d不需要取消插桩）
        b_handle(uc,translated_handle_2, "translated_function_2" ,"translate_part2.so");

        // 2. 将 PC 设置为 C 点，继续执行原代码
        uc->uc_mcontext.__gregs[REG_PC] = patch_point_c;
        printf("[SIGNAL_HANDLER] Set PC to C: %p\n", (void*)uc->uc_mcontext.__gregs[REG_PC]);
    }
    else {
        fprintf(stderr, "[SIGNAL_HANDLER] ERROR: Unexpected EBREAK at %p\n", (void*)fault_pc);
        _exit(1);
    }
    printf("[SIGNAL_HANDLER] >>> SIGTRAP handler returning <<<\n\n");
}

// --- Core Handler Functions ---
void a_handle(ucontext_t *uc) {
    // a_handle负责保存迁移前向量上下文+patch B和D点
    printf("\n[MIGRATION] >>> INSIDE a_handle <<<\n");
    fflush(stdout);

    save_vector_states(uc);

    patch_code(patch_point_b);
    patch_code(patch_point_d);

    // query_cpu();
    // switch_cpu_set('x');
    // query_cpu();

    printf("[MIGRATION] >>> a_handle completed <<<\n\n");
    fflush(stdout);
}

#define LOAD(reg, idx) \
    do { \
    __asm__ volatile ( \
        "mv " #reg ", %0\n\t" \
        : \
        : "r" (uc->uc_mcontext.__gregs[idx]) \
        : #reg  \
    ); \
} while(0)

#define STORE(reg, idx) \
    do { \
        __asm__ volatile ( \
            "mv %0, " #reg "\n\t" \
            : "=r" (uc->uc_mcontext.__gregs[idx]) \
            : \
            : "memory" \
        ); \
    } while(0)

void b_handle(ucontext_t *uc, void* translated_handle,const char* func_name,const char* file_name)  {    
    if(!strcmp(func_name, "translated_function_1")){
        printf("\n[MIGRATION] >>> INSIDE b_handle <<<\n");
    }
    else if(!strcmp(func_name, "translated_function_2")){
        printf("\n[MIGRATION] >>> INSIDE d_handle <<<\n");
    }
    fflush(stdout);

    // ***== 使用翻译函数模拟逻辑 ==*** 
    uintptr_t addr = get_func_addr_in_file(translated_handle, func_name, file_name);
    void (*fn)() = (void(*)())addr;
    
    // 翻译函数在模拟原本的向量逻辑时，可能会用到除t0~n外的通用寄存器，目前的demo使用了以下寄存器
    
    // 从上下文中恢复通用寄存器
    LOAD(a0, 10);
    LOAD(a1, 11);
    LOAD(a2, 12);
    LOAD(a3, 13);
    LOAD(a4, 14);
    // LOAD(a5, 15);
    // LOAD(a6, 16);
    // LOAD(a7, 17);
    LOAD(s0, 8);
    LOAD(s1, 9);
    // LOAD(s2, 18);
    // LOAD(s3, 19);
    // LOAD(s4, 20);
    // LOAD(s5, 21);
    // LOAD(s6, 22);
    // LOAD(s7, 23);
    // LOAD(s8, 24);
    // LOAD(s9, 25);
    // LOAD(s10, 26);
    // LOAD(s11, 27);
    
    fn();

    // 模拟执行代码里有标量代码，会改变通用寄存器的值，现在将标量寄存器的改变更新上下文
    STORE(a0, 10);
    STORE(a1, 11);
    STORE(a2, 12);
    STORE(a3, 13);
    STORE(a4, 14);
    // STORE(a5, 15);
    // STORE(a6, 16);
    // STORE(a7, 17);
    STORE(s0, 8);
    STORE(s1, 9);
    // STORE(s2, 18);
    // STORE(s3, 19);
    // STORE(s4, 20);
    // STORE(s5, 21);
    // STORE(s6, 22);
    // STORE(s7, 23);
    // STORE(s8, 24);
    // STORE(s9, 25);
    // STORE(s10, 26);
    // STORE(s11, 27);

    // demo中循环里改变的是v8(lmul=2,所以对应的内存是v8和v9)，需要更新上下文里的向量寄存器数据
	struct __riscv_v_ext_state *v_ext_state = get_vector_context(uc);
    for(int i = 0; i < 1024 / 32; i++){
        *((float*)v_ext_state->datap + 8 * 1024 / 32 + i) = *((float*)g_simulated_cpu_state + 8 * 1024 / 32 + i);
        *((float*)v_ext_state->datap + 9 * 1024 / 32 + i) = *((float*)g_simulated_cpu_state + 9 * 1024 / 32 + i);
    }

    // ***== 翻译模拟逻辑结束 ==***

    printf("[MIGRATION] >>> b_handle completed <<<\n\n");
    fflush(stdout);
}


// --- Utility Functions ---
struct __riscv_v_ext_state * get_vector_context(ucontext_t *uc) {
    struct __riscv_extra_ext_header *ext;
    struct __riscv_v_ext_state *v_ext_state;
    
    ext = (void *)(&uc->uc_mcontext.__fpregs);
    if (ext->hdr.magic != RISCV_V_MAGIC) {
        fprintf(stderr, "bad vector magic: %x\n", ext->hdr.magic);
        abort();
    }
    
    v_ext_state = (void *)((char *)(ext) + sizeof(*ext));
    return v_ext_state;
}

void save_vector_states(ucontext_t *uc) {
	struct __riscv_v_ext_state *v_ext_state = get_vector_context(uc); 

    // 保存向量状态
    *(uint64_t*)(g_simulated_cpu_state + 0x1000) = (uint64_t)v_ext_state->vstart;
        //vxsat没有处理
        //vxrm没有处理
    *(uint64_t*)(g_simulated_cpu_state + 0x1018) = (uint64_t)v_ext_state->vcsr;
    *(uint64_t*)(g_simulated_cpu_state + 0x1020) = (uint64_t)v_ext_state->vl;
    *(uint64_t*)(g_simulated_cpu_state + 0x1028) = (uint64_t)v_ext_state->vtype;
    *(uint64_t*)(g_simulated_cpu_state + 0x1030) = (uint64_t)v_ext_state->vlenb;

    // 保存 v0 到 v31
    int vlen = (uint64_t)v_ext_state->vlenb * 8;
    for(int i = 0; i < 32* vlen / 64; i++){
        *((uint64_t*)g_simulated_cpu_state + i) = *((uint64_t*)v_ext_state->datap + i);
    }

}

void switch_cpu_set(char core_type){
    pid_t pid = getpid();
    FILE* file = NULL;
    char proc_path_ai[] = "/proc/set_ai_thread";
    char proc_path_regular[] = "/proc/set_regular_thread";

    // 以写入模式打开目标文件
    if('a' ==  core_type){
        file = fopen(proc_path_ai, "w");
    }
    else if('x' == core_type){
        file = fopen(proc_path_regular, "w");
    }
    else{
        return;
    }
    if (file == NULL) {
        // 打开文件失败，通常意味着没有权限或文件不存在
        perror("错误: 无法打开 /proc/set_ai_thread");
        return;
    }

    // 将PID格式化并写入文件
    // 使用 %d 格式符处理常见的整数型PID
    if (fprintf(file, "%d", pid) < 0) {
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
    sched_yield();
    // 操作成功完成
    return;
}

void query_cpu(){
    int current_cpu = sched_getcpu();
    printf("[QUERY_CPU] current CPU id is: %d\n", current_cpu);
}

uintptr_t get_func_addr(const char* func_name) {
    void* handle = dlopen(NULL, RTLD_LAZY);
    if (!handle) {
        fprintf(stderr, "Error in dlopen: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }
    void* addr = dlsym(handle, func_name);
    if (!addr) {
        fprintf(stderr, "Error in dlsym: %s\n", dlerror());
        dlclose(handle);
        exit(EXIT_FAILURE);
    }
    dlclose(handle);
    return (uintptr_t)addr;
}

uintptr_t get_func_addr_in_file(void* translated_handle, const char* func_name, const char* file_path) {
    void* addr = dlsym(translated_handle, func_name);
    if (!addr) {
        fprintf(stderr, "Error in dlsym: %s\n", dlerror());
        dlclose(translated_handle);
        exit(EXIT_FAILURE);
    }
    return (uintptr_t)addr;
}

void patch_code(uintptr_t addr) {
    printf("[PATCHER] Attempting to patch code at: 0x%lx with EBREAK\n", addr);
    fflush(stdout);

    size_t page_size = getpagesize();
    uintptr_t page_start = addr & ~(page_size - 1);

    // 修改访问权限使得可修改
    if (mprotect((void*)page_start, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        perror("mprotect failed during patching");
        exit(EXIT_FAILURE);
    }

    uint32_t* ins_ptr = (uint32_t*)addr;

    // 保存原指令
    if (addr == patch_point_a) {
        original_ins_a = ins_ptr[0];
    } else if (addr == patch_point_b) {
        original_ins_b = ins_ptr[0];
    } else if (addr == patch_point_d) {
        original_ins_d = ins_ptr[0];
    } 
    
    // 修改成ebreak
    ins_ptr[0] = 0x00100073; // EBREAK
    
    __builtin___clear_cache((char*)addr, (char*)addr + 4);
    if (addr == patch_point_a) {
        printf("[PATCHER] Code successfully patched at A.\n");
    } else if (addr == patch_point_b) {
        printf("[PATCHER] Code successfully patched at B.\n");
    }
    else{
        printf("[PATCHER] Code successfully patched at: 0x%lx with EBREAK.\n", addr);
    }

    fflush(stdout);
}

void restore_code(uintptr_t addr) {
    printf("[PATCHER] Attempting to restore code at: 0x%lx\n", addr);
    fflush(stdout);

    size_t page_size = getpagesize();
    uintptr_t page_start = addr & ~(page_size - 1);

    mprotect((void*)page_start, page_size, PROT_READ | PROT_WRITE | PROT_EXEC);

    uint32_t* ins_ptr = (uint32_t*)addr;

    if (addr == patch_point_a) {
        ins_ptr[0] = original_ins_a;
    } else if (addr == patch_point_b) {
        ins_ptr[0] = original_ins_b;
    } else if (addr == patch_point_d) {
        ins_ptr[0] = original_ins_d;
    }

    __builtin___clear_cache((char*)addr, (char*)addr + 4);
    printf("[PATCHER] Code restored at: 0x%lx\n", addr);
    fflush(stdout);
}

// --- Library Entry Point ---
__attribute__((constructor))
int init_inject(int argc, char* argv[]) {
    printf("\n[INJECTOR] >>> SHARED LIBRARY CONSTRUCTOR STARTS <<<\n");
    fflush(stdout);

    struct sigaction sa;
    sa.sa_sigaction = my_ebreak_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGTRAP, &sa, NULL) == -1) {
        perror("sigaction for SIGTRAP failed");
        return -1;
    }
    printf("[INJECTOR] SIGTRAP handler for EBREAK registered.\n");
    
    translated_handle_1 = dlopen("./translate_part1.so", RTLD_NOW | RTLD_GLOBAL);
    if (!translated_handle_1) {
        fprintf(stderr, "Error in dlopen: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }
    translated_handle_2 = dlopen("./translate_part2.so", RTLD_LAZY | RTLD_GLOBAL);
    if (!translated_handle_2) {
        fprintf(stderr, "Error in dlopen: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }
    
    g_simulated_cpu_state = (uint8_t*)dlsym(translated_handle_1, "simulated_cpu_state");
    if (!g_simulated_cpu_state) {
        fprintf(stderr, "Failed to get simulated_cpu_state: %s\n", dlerror());
        exit(1);
    }
    else{
        printf("address of g_simulated_cpu_state is %p\n", g_simulated_cpu_state);
    }

    const char* target_func_name = getenv("TARGET_FUNC") ? getenv("TARGET_FUNC") : "main";

    target_func_ptr = (void*)get_func_addr(target_func_name);
    if (!target_func_ptr) {
        fprintf(stderr, "Error: Failed to find target function: %s\n", target_func_name);
        return -1;
    }


    int OFFSET_A = (int)strtol(getenv("migration_point_A"), NULL, 0);
    int OFFSET_B = (int)strtol(getenv("translate_point_begin_B"), NULL, 0);
    int OFFSET_D = (int)strtol(getenv("translate_point_begin_D"), NULL, 0);
    int OFFSET_C = (int)strtol(getenv("translate_point_end_C"), NULL, 0);
    int main_pc = (int)strtol(getenv("migration_point_main"), NULL, 0);

    patch_point_a = (uintptr_t)target_func_ptr + OFFSET_A - main_pc;
    patch_point_b = (uintptr_t)target_func_ptr + OFFSET_B - main_pc;
    patch_point_d = (uintptr_t)target_func_ptr + OFFSET_D - main_pc;
    patch_point_c = (uintptr_t)target_func_ptr + OFFSET_C - main_pc;

    printf("[INJECTOR] Calculated patch points:\n");
    printf("  A: 0x%lx\n", patch_point_a);
    printf("  B: 0x%lx\n", patch_point_b);
    printf("  C: 0x%lx\n", patch_point_c);
    printf("  D: 0x%lx\n", patch_point_d);


    // 在构造函数中给 A 点打上 EBREAK
    patch_code(patch_point_a);
    printf("[INJECTOR] Successfully patched code at A with EBREAK.\n");

    //query_cpu();

    printf("[INJECTOR] >>> SHARED LIBRARY CONSTRUCTOR ENDS <<<\n\n");
    fflush(stdout);
    return 0;
}
