#include "main.h"
#include "../include/patch.h"
#include "../include/handle.h"
#include "../include/vector_context.h"
#include "../include/addr.h"
#include "../include/config.h"
#include <cstdlib>
#include <string>
#include <dlfcn.h>
#include <iostream>
#include <signal.h>

// Constructor function that runs automatically when program loads
__attribute__((constructor))
void init() {
    // Set up signal handler for SIGTRAP
    struct sigaction sa;
    sa.sa_sigaction = Patch::ebreak_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGTRAP, &sa, NULL) != 0) {
        return;
    }

    // Load migration library
    global_migration_lib_handle = dlopen(config_migration_lib_name, RTLD_LAZY);
    if (!global_migration_lib_handle) {
        return;
    }
    
    // Get migration library base address
    Dl_info dl_info;
    if (dladdr((void*)global_migration_lib_handle, &dl_info) && dl_info.dli_fbase) {
        global_migration_lib_base_addr = (uint64_t)dl_info.dli_fbase;
    } else {
        return;
    }
    
    // Get migration address
    global_migration_addr = Addr::get_migration_addr(global_migration_lib_handle, config_migration_func_name, config_migration_func_offset);
    if (global_migration_addr == 0) {
        return;
    }
    
    // Generate shared library for original instruction at migration address
    std::string inst_command = "python3 scripts/inst_to_so.py 0x" + std::to_string(global_migration_addr - global_migration_lib_base_addr);
    int inst_result = system(inst_command.c_str());
    if (inst_result != 0) {
        // Handle error if needed
    }
    
    // Load the generated shared library
    std::string lib_path = "content_" + std::to_string(global_migration_addr - global_migration_lib_base_addr, 16) + ".so";
    global_migration_code_handle = dlopen(lib_path.c_str(), RTLD_LAZY);
    if (!global_migration_code_handle) {
        std::cerr << "Error loading migration code library: " << dlerror() << std::endl;
        return;
    }
    
    // Initialize vector context pool
    VectorContext::initialize_vector_context_pool();
    
    // Get translation ranges at initialization
    global_translation_ranges = Addr::get_translation_ranges(global_migration_addr);
    
    // Patch migration address
    Patch::patch(global_migration_addr, &global_migration_code);
    
}
