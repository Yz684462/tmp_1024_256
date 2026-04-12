#include "main.h"
#include "../include/addr.h"
#include "../include/config.h"
#include "../include/globals.h"
#include <cstdlib>
#include <string>
#include <dlfcn.h>
#include <iostream>
#include <signal.h>

// Mock config variables for testing
const char* config_migration_lib_name = "test_migration.so";
const char* config_migration_func_name = "test_migration_func";
const uint64_t config_migration_func_offset = 0x1000;

// Mock global variables for testing
void* global_migration_lib_handle = nullptr;
uint64_t global_migration_lib_base_addr = 0;
uint64_t global_migration_addr = 0;
uint32_t global_migration_code = 0;
void* global_migration_code_handle = nullptr;
std::vector<uint64_t> global_patched_addrs;
std::vector<std::pair<uint64_t, uint64_t>> global_translation_ranges;
void* global_simulated_vector_contexts_pool = nullptr;
std::map<int, void*> global_thread_translated_handles;

// Constructor function that runs automatically when program loads
__attribute__((constructor))
void init() {    
    std::cout << "[TEST] Starting initialization test..." << std::endl;
    
    // Load migration library
    std::cout << "[TEST] Loading migration library: " << config_migration_lib_name << std::endl;
    global_migration_lib_handle = dlopen(config_migration_lib_name, RTLD_LAZY);
    if (!global_migration_lib_handle) {
        std::cout << "[TEST] Warning: Could not load migration library" << std::endl;
        return;
    } else {
        std::cout << "[TEST] Successfully loaded migration library" << std::endl;
        
        // Get migration library base address
        Dl_info dl_info;
        if (dladdr((void*)global_migration_lib_handle, &dl_info) && dl_info.dli_fbase) {
            global_migration_lib_base_addr = (uint64_t)dl_info.dli_fbase;
            std::cout << "[TEST] Migration library base address: 0x" << std::hex << global_migration_lib_base_addr << std::dec << std::endl;
        } else {
            std::cout << "[TEST] Warning: Could not get library base address" << std::endl;
            return;
        }
        
        // Get migration address
        global_migration_addr = Addr::get_migration_addr(global_migration_lib_handle, config_migration_func_name, config_migration_func_offset);
        if (global_migration_addr == 0) {
            std::cout << "[TEST] Warning: Could not get migration address" << std::endl;
            return;
        }
    }
    
    std::cout << "[TEST] Migration address: 0x" << std::hex << global_migration_addr << std::dec << std::endl;
    
    // Get translation ranges at initialization
    std::cout << "[TEST] Getting translation ranges..." << std::endl;
    global_translation_ranges = Addr::get_translation_ranges(global_migration_addr);
    
    std::cout << "[TEST] Found " << global_translation_ranges.size() << " translation ranges:" << std::endl;
    for (size_t i = 0; i < global_translation_ranges.size(); i++) {
        std::cout << "[TEST]   Range " << i << ": [0x" << std::hex << global_translation_ranges[i].first 
                  << ", 0x" << global_translation_ranges[i].second << ")" << std::dec << std::endl;
    }
    
    std::cout << "[TEST] Initialization test completed!" << std::endl;
}
