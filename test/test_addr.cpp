#include "../include/addr.h"
#include "../include/config.h"
#include "../include/globals.h"
#include <cstdlib>
#include <string>
#include <dlfcn.h>
#include <iostream>
#include <signal.h>
#include <link.h>

int main() {    
    std::cout << "[TEST] Starting initialization test..." << std::endl;
    
    // Load migration library
    std::cout << "[TEST] Loading migration library: " << config_migration_lib_name << std::endl;
    global_migration_lib_handle = dlopen(config_migration_lib_name, RTLD_LAZY);
    if (!global_migration_lib_handle) {
        std::cout << "[TEST] Warning: Could not load migration library" << std::endl;
        return 1;
    } else {
        std::cout << "[TEST] Successfully loaded migration library" << std::endl;
        
        // Get migration library base address
        struct link_map *map;
        if (dlinfo(global_migration_lib_handle, RTLD_DI_LINKMAP, &map) == 0) {
            global_migration_lib_base_addr = (uint64_t)map->l_addr;
            std::cout << "[TEST] Migration library base address: 0x" << std::hex << global_migration_lib_base_addr << std::dec << std::endl;
        } else {
            std::cout << "[TEST] Warning: Could not get library base address: " << dlerror() << std::endl;
            return 1;
        }
        
        // Get migration address
        global_migration_addr = Addr::get_migration_addr(global_migration_lib_handle, config_migration_func_name, config_migration_func_offset);
        if (global_migration_addr == 0) {
            std::cout << "[TEST] Warning: Could not get migration address" << std::endl;
            return 1;
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
    
    return 0;
}
