// inject_lib.c
#include <stdint.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <ucontext.h>
#include <linux/ptrace.h>
extern "C"{
    #include "patch.h"
}
#include <map>
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <link.h>
#include <mutex>
#include <fstream>
#include <sstream> 
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
volatile  uint64_t main_exe_base = 0;
static uint8_t *simulated_cpu_state_ptr;
static bool is_simulated_cpu_state_initialized = false;

// --- Utility Functions ---
struct __riscv_v_ext_state * get_vector_context(ucontext_t *uc) {
    struct __riscv_extra_ext_header *ext;
    struct __riscv_v_ext_state *v_ext_state;
    
    ext = (struct __riscv_extra_ext_header *)(&uc->uc_mcontext.__fpregs);
    if (ext->hdr.magic != RISCV_V_MAGIC) {
        std::cout << "bad vector magic: " <<  ext->hdr.magic << std::endl;
        abort();
    }
    
    v_ext_state = (struct __riscv_v_ext_state *)((char *)(ext) + sizeof(*ext));
    return v_ext_state;
}

void save_vector_states(ucontext_t *uc) {
	struct __riscv_v_ext_state *v_ext_state = get_vector_context(uc); 
    // 保存向量状态
    *(uint64_t*)(simulated_cpu_state_ptr + 0x1000) = (uint64_t)v_ext_state->vstart;
        //vxsat没有处理
        //vxrm没有处理
    *(uint64_t*)(simulated_cpu_state_ptr + 0x1018) = (uint64_t)v_ext_state->vcsr;
    *(uint64_t*)(simulated_cpu_state_ptr + 0x1020) = (uint64_t)v_ext_state->vl;
    *(uint64_t*)(simulated_cpu_state_ptr + 0x1028) = (uint64_t)v_ext_state->vtype;
    *(uint64_t*)(simulated_cpu_state_ptr + 0x1030) = (uint64_t)v_ext_state->vlenb;
    std::cout << "vector states:" << std::endl;
    std::cout << "\tvstart = " << (uint64_t)v_ext_state->vstart << std::endl;
    std::cout << "\tvcsr = " << (uint64_t)v_ext_state->vcsr << std::endl;
    std::cout << "\tvl = " << (uint64_t)v_ext_state->vl << std::endl;
    std::cout << "\tvtype = " << (uint64_t)v_ext_state->vtype << std::endl;
    std::cout << "\tvlenb = " << (uint64_t)v_ext_state->vlenb << std::endl;

    if(v_ext_state->vlenb != 128){
        std::cout << "错误：vlenb应该是128，现在只支持1024到256" << std::endl;
    }

    // 保存 v0 到 v31
    int vlen = (uint64_t)v_ext_state->vlenb * 8;
    for(int i = 0; i < 32* vlen / 64; i++){
        *((uint64_t*)simulated_cpu_state_ptr + i) = *((uint64_t*)v_ext_state->datap + i);
    }

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
        std::cout << "mprotect failed during patching" << std::endl; 
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
    std::istringstream iss(envValue);
    std::string token;
    
    while (iss >> token) {
        size_t comma = token.find(',');
        if (comma == std::string::npos) {
            std::cerr << "Invalid range format: " << token << " (missing comma)" << std::endl;
            continue;
        }
        
        std::string start_hex = token.substr(0, comma);
        std::string end_hex = token.substr(comma + 1);
        
        uint64_t start = std::stoull(start_hex, nullptr, 0);
        uint64_t end = std::stoull(end_hex, nullptr, 0);
        
        get_vector_snippet_ranges().push_back({start, end});
    }
}

uint64_t get_base_addr_with_dlinfo(void *handle) {
    struct link_map *map;
    
    // 获取 link_map
    if (dlinfo(handle, RTLD_DI_LINKMAP, &map) != 0) {
        std::cout << "dlinfo failed: " << dlerror() << std::endl;
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

void tmp_handle_scalar_vsetvl(ucontext_t *uc,uint64_t rela_start_addr){
    // -->新代码的实现
    if(rela_start_addr == 0xb8e){
        uc->uc_mcontext.__gregs[12] = *(uint64_t*)(simulated_cpu_state_ptr + 0x1020);        
    }
    else if(rela_start_addr == 0xb96){
        uc->uc_mcontext.__gregs[12] = *(uint64_t*)(simulated_cpu_state_ptr + 0x1020);        
    }
    else if(rela_start_addr == 0xbba){
        uc->uc_mcontext.__gregs[10] = *(uint64_t*)(simulated_cpu_state_ptr + 0x1020);
    }   
    // <--新代码的实现


    // // -->旧代码的实现
    // // if(rela_start_addr == 0x994){
    // //     uc->uc_mcontext.__gregs[12] = *(uint64_t*)(simulated_cpu_state_ptr + 0x1020);        
    // // }
    // // else if(rela_start_addr == 0x99c){
    // //     uc->uc_mcontext.__gregs[12] = *(uint64_t*)(simulated_cpu_state_ptr + 0x1020);        
    // // }
    // // else if(rela_start_addr == 0x9c0){
    // //     uc->uc_mcontext.__gregs[10] = *(uint64_t*)(simulated_cpu_state_ptr + 0x1020);
    // // }   
    
    // // 无while版本
    // if(rela_start_addr == 0x978){
    //     uc->uc_mcontext.__gregs[12] = *(uint64_t*)(simulated_cpu_state_ptr + 0x1020);        
    // }
    // else if(rela_start_addr == 0x980){
    //     uc->uc_mcontext.__gregs[12] = *(uint64_t*)(simulated_cpu_state_ptr + 0x1020);        
    // }
    // else if(rela_start_addr == 0x9a4){
    //     uc->uc_mcontext.__gregs[10] = *(uint64_t*)(simulated_cpu_state_ptr + 0x1020);
    // }   
    // // <--旧代码的实现
}

// --- Handler ---
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

void print_vreg(int vreg, int data_type){
    if (data_type == 0){
        float *vreg_ptr = (float *)(simulated_cpu_state_ptr + 128 * vreg);
        std::cout << std::dec << "simulated v" << vreg << " data:" << std::endl;
        for(int i = 0; i < 1024 * 8 / 32 / 8; i++){
            std::cout << vreg_ptr[i] << " ";
        }
        std::cout << std::endl;
    }
}

void debug_print(ucontext_t *uc, uint64_t rela_start_addr){
    std::cout << "offset = 0x" << std::hex << rela_start_addr << std::endl;
    std::cout << std::dec << "vl = " << *(uint64_t*)(simulated_cpu_state_ptr + 0x1020) 
        << " vtype = " <<  *(uint64_t*)(simulated_cpu_state_ptr + 0x1028) << std::endl;  
    if ( rela_start_addr == 0x9aa){
        std::cout << "a0 = " << uc->uc_mcontext.__gregs[10] << std::endl;
        std::cout << "a1 = " << uc->uc_mcontext.__gregs[11] << std::endl;
        std::cout << "a2 = " << uc->uc_mcontext.__gregs[12] << std::endl;
        std::cout << "a3 = " << uc->uc_mcontext.__gregs[13] << std::endl;
        std::cout << "a4 = " << uc->uc_mcontext.__gregs[14] << std::endl;
        std::cout << "a3 data: " << std::endl;
        float *data_ptr = (float *)uc->uc_mcontext.__gregs[13];
        for(int i =0; i < 16; i++){
            std::cout << data_ptr[i] << " ";
        }
        std::cout << std::endl;
        std::cout << "a4 data: " << std::endl;
        data_ptr = (float *)uc->uc_mcontext.__gregs[14];
        for(int i =0; i < 16; i++){
            std::cout << data_ptr[i] << " ";
        }
        std::cout << std::endl;
    }

    print_vreg(8,0);
    print_vreg(10,0);
    print_vreg(12,0);
    std::cout << std::endl;
}

void my_handler(int sig, siginfo_t *info, void *context) {
    static int count_handler_enter = 0;
    ++count_handler_enter;
    if(count_handler_enter < 5){
        std::cout << ">>> SIGTRAP handler enters <<<" << std::endl;
    }
    else if (count_handler_enter == 5){
        std::cout << "..." << std::endl;
    }
    ucontext_t *uc = (ucontext_t *)context;
    uint64_t fault_pc = (uint64_t)info->si_addr;
    if (!is_simulated_cpu_state_initialized) {
        save_vector_states(uc);
        std::cout << ">>>      initialize simulated_cpu_state <<<" << std::endl;
        is_simulated_cpu_state_initialized = true;
    }
    void (*fn)() = (void(*)())(get_addr_func_ptr_map()[fault_pc - main_exe_base]);
    uint64_t tmp_main_exe = main_exe_base;
    LOAD(a0, 10);
    LOAD(a1, 11);
    LOAD(a2, 12);
    LOAD(a3, 13);
    LOAD(a4, 14);

    fn();

    STORE(a0, 10);
    STORE(a1, 11);
    STORE(a2, 12);
    STORE(a3, 13);
    STORE(a4, 14);

    main_exe_base = tmp_main_exe;
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
        //DEBUG:打印pc
        std::cout << "[DEBUG] handler return pc = " << std::hex << rela_end_addr + main_exe_base << std::endl;
    } else {
        std::cout << "rela_start_addr = " << rela_start_addr << std::endl;
        std::cerr << "Error: rela_end_addr is 0" << std::endl;
        // 抛出异常
        throw std::runtime_error("rela_end_addr is 0");
    }

    // debug_print(uc, rela_start_addr);

    if (rela_start_addr == 0xc22){
        print_vreg(8, 0);
    }
}

void setup_handler(){
    struct sigaction sa;
    sa.sa_sigaction = my_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    // // --> 旧代码的实现
    // if (sigaction(SIGTRAP, &sa, NULL) == -1) {
    //     perror("sigaction for SIGTRAP failed");
    //     return;
    // }
    // // <-- 旧代码的实现
    
    // --> 新代码的实现 | 使用信号40
    if (sigaction(40, &sa, NULL) == -1) {
        perror("sigaction for SIGTRAP failed");
        return;
    }
    // <-- 新代码的实现
    std::cout << "[INJECTOR] SIGTRAP handler for EBREAK registered." << std::endl;
}

void load_simulated_data_ptr(){
    void* data_handle = dlopen("./libdata.so", RTLD_NOW | RTLD_GLOBAL);
    simulated_cpu_state_ptr = (uint8_t *)dlsym(data_handle, "simulated_cpu_state");
}

// --- Library Entry Point ---
__attribute__((constructor))
int init_inject(int argc, char* argv[]) {
    std::cout << "\n[INJECTOR] >>> SHARED LIBRARY CONSTRUCTOR STARTS <<<" << std::endl;
    load_simulated_data_ptr();
    setup_handler();

    const char* envValue = std::getenv("vector_snippet_ranges");
    if (!envValue) {
        std::cerr << "环境变量vector_snippet_ranges未设置" << std::endl;
        return 1;
    }
    parseRangesRegex(envValue);
    make_addr_func_ptr_map(get_vector_snippet_ranges());
    
    // // ==> 旧代码的实现：ebreak方式 | patch迁移点和翻译点
    // void *main_exe_handle = dlopen(NULL, RTLD_LAZY);
    // if (!main_exe_handle) {
    //     std::cout << "Error in dlopen: " << dlerror() << std::endl;
    //     exit(EXIT_FAILURE);
    // }
    // main_exe_base = get_base_addr_with_dlinfo(main_exe_handle);
    // for(auto& range : get_vector_snippet_ranges()) {
    //     patch_code(range.first + main_exe_base);
    // }
    // std::cout << "[INJECTOR] Successfully patched code with EBREAK." << std::endl;
    // // <== 旧代码结束


    // ==> 新代码的实现：map方式 | 将插桩点写入bpf map
    //打印history.txt文件，读取里面是否有当前程序路径
    std::string current_path = argv[0];
    std::ifstream history_file("history.txt");
    std::string line;
    bool skip = false;
    while (std::getline(history_file, line)) {
        if (line.find(current_path) != std::string::npos) {
            skip = true;
            break;
        }
    }
    history_file.close();
    
    if (skip) {
        std::cout << "[INJECTOR] Skipping patching code with BPF map." << std::endl;
    }
    else{
        // 写入历史记录
        std::ofstream history_file("history.txt", std::ios::app);
        history_file << current_path << std::endl;
        history_file.close();
        
        // 写入bpf map
        for(auto& range : get_vector_snippet_ranges()) {
            patch_code_map(range.first);
        }
        patch_code_map(0);
        std::cout << "[INJECTOR] Successfully write patch points to BPF map." << std::endl;
    }
    // <== 新代码结束

    std::cout << "[INJECTOR] >>> SHARED LIBRARY CONSTRUCTOR ENDS <<<" << std::endl;
    return 0;
}
