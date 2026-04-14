#include "main.h"
#include "../include/patch.h"
#include "../include/handle.h"
#include "../include/vector_context.h"
#include "../include/addr.h"
#include "../include/config.h"
#include "../include/globals.h"
#include <cstdlib>
#include <string>
#include <dlfcn.h>
#include <iostream>
#include <signal.h>
#define _GNU_SOURCE
#include <link.h>

// Helper function to set up signal handler
void setup_signal_handler() {
    // Set up signal handler for SIGTRAP
    struct sigaction sa;
    sa.sa_sigaction = Patch::ebreak_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGTRAP, &sa, NULL) != 0) {
        return;
    }
}

// Helper function to generate and load shared library
void generate_and_load_migration_code() {
    // Generate shared library for original instruction at migration address
    std::string output_file = "migration_original_code" + ".so";
    
    std::string inst_command = "python3 scripts/inst_to_so.py " + std::string(config_migration_dump_path) + " 0x" + std::to_string(global_migration_addr - global_migration_lib_base_addr) + " " + global_migration_original_code_func_name + " " + output_file;
    int inst_result = system(inst_command.c_str());
    if (inst_result != 0) {
        // Handle error if needed
    }
    
    // Load the generated shared library
    global_migration_code_handle = dlopen(output_file.c_str(), RTLD_LAZY);
    if (!global_migration_code_handle) {
        std::cerr << "Error loading migration code library: " << dlerror() << std::endl;
        return;
    }
}

// Constructor function that runs automatically when program loads
__attribute__((constructor))
void init() {    
    // Initialize migration (function now handles library opening and base address calculation)
    Addr::init_migration(config_migration_lib_name, config_migration_func_name, config_migration_func_offset);
    
    // Initialize translation ranges at initialization
    Addr::init_translation_ranges(global_migration_addr);
    
    // Set up signal handler
    setup_signal_handler();
    
    // Initialize vector context pool
    VectorContext::initialize_vector_context_pool();
    
    // Generate and load migration code
    generate_and_load_migration_code();
    
    // Patch migration address
    Patch::patch(global_migration_addr, &global_migration_code);
    
}
