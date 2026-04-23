#include "core.h"
#include "utils.h"
#include <dlfcn.h>
#include <iostream> 

namespace BinaryTranslation {

uint64_t Migration::migration_addr;

void init_migration(){
    // dump_file_name = "dump.s"
    Dump::DumpAnalyzer::getInstance("../dump.s");
    
    // shared_lib_path = "libggml-cpu.so"
    uint64_t base_addr = Addr::get_shared_lib_base_addr("libggml-cpu.so");
    if (base_addr == 0) {
        std::cerr << "Error getting shared library base address" << std::endl;
        return;
    }
    
    auto &addr_manager = Addr::AddrManager::getInstance(base_addr);   
    
    // migration_addr = 0x37056
    void *handle = dlopen("libggml-cpu.so", RTLD_NOLOAD | RTLD_LAZY);
    if(!handle){
        std::cerr << "Error loading shared library" << std::endl;
        return;
    }

    std::cout << "base address is " << std::hex << base_addr << std::dec << std::endl;

    Migration::migration_addr = (uint64_t)dlsym(handle, "ggml_compute_forward_mul_mat");
    std::cout << "migration_addr is " << std::hex << Migration::migration_addr << std::dec << std::endl;
    
    
    Migration::migration_addr = (uint64_t)dlsym(handle, "ggml_graph_compute._omp_fn.0");
    std::cout << "migration_addr is " << std::hex << Migration::migration_addr << std::dec << std::endl;
    
    Migration::migration_addr = (uint64_t)dlsym(handle, "ggml_gemv_q4_K_8x8_q8_K");
    std::cout << "migration_addr is " << std::hex << Migration::migration_addr << std::dec << std::endl;
    
    

    if (!Migration::migration_addr) {
        const char* error = dlerror();
        if (error) {
            std::cerr << "Error finding symbol: " << error << std::endl;
        }
        dlclose(handle);  // 如果不需要了，可以关闭
        return;
    }
    
    // patch migration addr
    auto &patcher = Patch::Patcher::getInstance();
    patcher.patch_addr(Migration::migration_addr);
}

__attribute__((constructor))
void init() {
    Handler::setup_signal_handler();
    init_migration();
}

} // namespace BinaryTranslation