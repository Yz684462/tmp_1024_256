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
extern "c"{
    #include "patch.h"
}
#include <map>
#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <cstdlib>
#include <link.h>
#include <mutex>
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
static void *translated_lib_handle = NULL;
static uint64_t translated_lib_base = 0;
uint64_t main_exe_base = 0;
void* simulated_cpu_state;
static bool is_simulated_cpu_state_initialized = false;
// ==> 旧代码的实现：ebreak方式 | 记录迁移点的地址
static uint64_t migration_addr = 0;
// <== 旧代码结束

// --- Utility Functions ---
struct __riscv_v_ext_state * get_vector_context(ucontext_t *uc) {
    struct __riscv_extra_ext_header *ext;
    struct __riscv_v_ext_state *v_ext_state;
    
    ext = (struct __riscv_extra_ext_header *)(&uc->uc_mcontext.__fpregs);
    if (ext->hdr.magic != RISCV_V_MAGIC) {
        fprintf(stderr, "bad vector magic: %x\n", ext->hdr.magic);
        abort();
    }
    
    v_ext_state = (struct __riscv_v_ext_state *)((char *)(ext) + sizeof(*ext));
    return v_ext_state;
}

void save_vector_states(ucontext_t *uc) {
	struct __riscv_v_ext_state *v_ext_state = get_vector_context(uc); 
    simulated_cpu_state = malloc(4192);
    // 保存向量状态
    *(uint64_t*)((uint8_t*)simulated_cpu_state + 0x1000) = (uint64_t)v_ext_state->vstart;
        //vxsat没有处理
        //vxrm没有处理
    *(uint64_t*)((uint8_t*)simulated_cpu_state + 0x1018) = (uint64_t)v_ext_state->vcsr;
    *(uint64_t*)((uint8_t*)simulated_cpu_state + 0x1020) = (uint64_t)v_ext_state->vl;
    *(uint64_t*)((uint8_t*)simulated_cpu_state + 0x1028) = (uint64_t)v_ext_state->vtype;
    *(uint64_t*)((uint8_t*)simulated_cpu_state + 0x1030) = (uint64_t)v_ext_state->vlenb;

    // 保存 v0 到 v31
    int vlen = (uint64_t)v_ext_state->vlenb * 8;
    for(int i = 0; i < 32* vlen / 64; i++){
        *((uint64_t*)simulated_cpu_state + i) = *((uint64_t*)v_ext_state->datap + i);
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

static std::map<uint64_t, void*>& get_addr_func_ptr_map() {
    static std::map<uint64_t, void*> my_map;
    static std::once_flag init_flag;
    std::call_once(init_flag, [](){
        // 确保只初始化一次
    });
    return my_map;
}

static std::map<uint64_t, uint32_t>& get_addr_original_bytes_map() {
    static std::map<uint64_t, uint32_t> my_map;
    static std::once_flag init_flag;
    std::call_once(init_flag, [](){
        // 确保只初始化一次
    });
    return my_map;
}

static std::vector<std::pair<uint64_t, uint64_t>>& get_vector_snippet_ranges() {
    static std::vector<std::pair<uint64_t, uint64_t>> my_vector;
    static std::once_flag init_flag;
    std::call_once(init_flag, [](){
        // 确保只初始化一次
    });
    return my_vector;
}

void patch_code(uint64_t addr) {
    std::cout << "[PATCHER] Attempting to patch code at: 0x" << std::hex << addr << std::dec << " with EBREAK" << std::endl;

    size_t page_size = getpagesize();
    uintptr_t page_start = addr & ~(page_size - 1);

    // 修改访问权限使得可修改
    if (mprotect((void*)page_start, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        perror("mprotect failed during patching");
        exit(EXIT_FAILURE);
    }

    uint32_t *ins_ptr = (uint32_t*)addr;
    // 保存原始指令
    get_addr_original_bytes_map()[addr] = *ins_ptr;
    // 修改成ebreak
    ins_ptr[0] = 0x00100073; // EBREAK
    
    __builtin___clear_cache((char*)addr, (char*)addr + 4);
    std::cout << "[PATCHER] Code successfully patched at: 0x" << std::hex << addr << std::dec << " with EBREAK" << std::endl;
}

void restore_code(uintptr_t addr) {
    std::cout << "[PATCHER] Attempting to restore code at: 0x" << std::hex << addr << std::dec << std::endl;

    size_t page_size = getpagesize();
    uintptr_t page_start = addr & ~(page_size - 1);

    mprotect((void*)page_start, page_size, PROT_READ | PROT_WRITE | PROT_EXEC);

    uint32_t *ins_ptr = (uint32_t*)addr;
    ins_ptr[0] = get_addr_original_bytes_map()[addr];

    __builtin___clear_cache((char*)addr, (char*)addr + 4);
    std::cout << "[PATCHER] Code restored at: 0x" << std::hex << addr << std::dec << std::endl;
}

void parseRangesRegex(const std::string& envValue) {
    // 匹配 "0x[0-9a-fA-F]+,0x[0-9a-fA-F]+" 模式
    std::regex rangePattern(R"((0x[0-9a-fA-F]+),?(0x[0-9a-fA-F]+))");
    std::smatch match;
    std::string::const_iterator searchStart(envValue.cbegin());
    
    while (std::regex_search(searchStart, envValue.cend(), match, rangePattern)) {
        try {
            uint64_t start = std::stoull(match[1].str(), nullptr, 0);
            uint64_t end = std::stoull(match[2].str(), nullptr, 0);
            get_vector_snippet_ranges().push_back(std::make_pair(start, end));
        } catch (const std::exception& e) {
            std::cerr << "解析错误: " << e.what() << std::endl;
        }
        searchStart = match.suffix().first;
    }
    
}

uint64_t get_base_addr_with_dlinfo(void *handle) {
    struct link_map *map;
    
    // 获取 link_map
    if (dlinfo(handle, RTLD_DI_LINKMAP, &map) != 0) {
        fprintf(stderr, "dlinfo failed: %s\n", dlerror());
        return 0;
    }
    
    // link_map 的 l_addr 就是基址
    return (uint64_t)(map->l_addr);
}

void make_addr_func_ptr_map(const std::vector<std::pair<uint64_t, uint64_t>>& ranges) {
    // 遍历ranges，对每个range调用dlopen和dlsym
    
    translated_lib_handle = dlopen("translated_lib.so", RTLD_NOW | RTLD_GLOBAL);
    if (!translated_lib_handle) {
        std::cerr << "Error in dlopen: " << dlerror() << std::endl;
        return;
    }
    for (const auto& range : ranges) {
        std::string func_name = "translated_function_" + std::to_string(range.first);
        void* func_addr = dlsym(translated_lib_handle, func_name.c_str());
        if (!func_addr) {
            std::cerr << "Error in dlsym: " << dlerror() << std::endl;
            dlclose(translated_lib_handle);
            return;
        }
        get_addr_func_ptr_map().insert({range.first, func_addr});
    }
    translated_lib_base = get_base_addr_with_dlinfo(translated_lib_handle);
}

#define STORE(uc, reg, idx) do { \
    __asm__ volatile ( \
        "sw %1, %0" \
        : "=m" (uc->uc_mcontext.__gregs[idx]) \
        : "r" (reg) \
        : "memory" \
    ); \
} while(0)


void tmp_handle_scalar_vsetvl(ucontext_t *uc,uint64_t rela_start_addr){
    if(rela_start_addr == 0x980){
        uc->uc_mcontext.__gregs[12] = *(uint64_t*)((uint8_t*)simulated_cpu_state + 0x1020);        
    }
}

// --- Handler ---
void my_handler(int sig, siginfo_t *info, void *context) {
    std::cout << ">>> SIGTRAP handler starts <<<" << std::endl;
    ucontext_t *uc = (ucontext_t *)context;
    uint64_t fault_pc = (uint64_t)info->si_addr;
    if (!is_simulated_cpu_state_initialized) {
        save_vector_states(uc);
        std::cout << ">>> save_vector_states called <<<" << std::endl;
        is_simulated_cpu_state_initialized = true;
    }
    void (*fn)() = (void(*)())(get_addr_func_ptr_map()[fault_pc - main_exe_base]);
    fflush(stdout);
    fn();
    uint64_t rela_start_addr = fault_pc - main_exe_base;
    tmp_handle_scalar_vsetvl(uc, rela_start_addr);
    uint64_t rela_end_addr = 0;
    for(const auto& range : get_vector_snippet_ranges()) {
        if (rela_start_addr == range.first) {
            rela_end_addr = range.second;
            break;
        }
    }
    if (rela_end_addr != 0) {
        uc->uc_mcontext.__gregs[REG_PC] = rela_end_addr + main_exe_base;
    } else {
        std::cerr << "Error: rela_end_addr is 0" << std::endl;
        // 抛出异常
        throw std::runtime_error("rela_end_addr is 0");
    }

    std::cout << ">>> SIGTRAP handler returning <<<" << std::endl;
}

void setup_handler(){
    struct sigaction sa;
    sa.sa_sigaction = my_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    // --> 旧代码的实现
    if (sigaction(SIGTRAP, &sa, NULL) == -1) {
        perror("sigaction for SIGTRAP failed");
        return;
    }
    // <-- 旧代码的实现
    
    // // --> 新代码的实现 | 使用信号40
    // if (sigaction(40, &sa, NULL) == -1) {
    //     perror("sigaction for SIGTRAP failed");
    //     return;
    // }
    // // <-- 新代码的实现
    std::cout << "[INJECTOR] SIGTRAP handler for EBREAK registered." << std::endl;
}

// --- Library Entry Point ---
__attribute__((constructor))
int init_inject(int argc, char* argv[]) {
    std::cout << "\n[INJECTOR] >>> SHARED LIBRARY CONSTRUCTOR STARTS <<<" << std::endl;

    setup_handler();

    const char* envValue = std::getenv("vector_snippet_ranges");
    if (!envValue) {
        std::cerr << "环境变量vector_snippet_ranges未设置" << std::endl;
        return 1;
    }
    parseRangesRegex(envValue);
    make_addr_func_ptr_map(get_vector_snippet_ranges());
    
    // ==> 旧代码的实现：ebreak方式 | patch迁移点和翻译点
    void *main_exe_handle = dlopen(NULL, RTLD_LAZY);
    if (!main_exe_handle) {
        fprintf(stderr, "Error in dlopen: %s\n", dlerror());
        exit(EXIT_FAILURE);
    }
    main_exe_base = get_base_addr_with_dlinfo(main_exe_handle);
    for(auto& range : get_vector_snippet_ranges()) {
        patch_code(range.first + main_exe_base);
    }
    std::cout << "[INJECTOR] Successfully patched code with EBREAK." << std::endl;
    // <== 旧代码结束


    // // ==> 新代码的实现：map方式 | 将插桩点写入bpf map
    // for(auto& range : vector_snippet_ranges) {
    //     patch_code_map(range.first);
    // }
    // std::cout << "[INJECTOR] Successfully write patch points to BPF map." << std::endl;
    // // <== 新代码结束

    std::cout << "[INJECTOR] >>> SHARED LIBRARY CONSTRUCTOR ENDS <<<" << std::endl;
    return 0;
}
