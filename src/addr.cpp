#include "addr.h"
#include "addr_init.h"
#include "addr_analysis.h"
#include "../include/globals.h"
#include "../include/config.h"
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <r_core.h>
#include <r_anal.h>

uint64_t Addr::get_migration_addr(void* migration_handle, const char* migration_func_name, uint64_t migration_offset) {
    // Use dlsym to find function address
    void* func_addr = dlsym(migration_handle, migration_func_name);
    if (!func_addr) {
        return 0;
    }
    
    // Add offset to function address
    return (uint64_t)func_addr + migration_offset;
}

std::vector<std::pair<uint64_t, uint64_t>> Addr::get_translation_ranges(uint64_t addr) {
    printf("[ADDR] Getting translation ranges for address 0x%lx\n", addr);
    
    // Initialize r_core
    RCore *core = AddrCore::init_r_core();
    if (!core) {
        return std::vector<std::pair<uint64_t, uint64_t>>();
    }
    
    // Find function containing address
    RAnalFunction *func = AddrCore::find_func(addr, core);
    if (!func) {
        fprintf(stderr, "No function found containing address 0x%lx\n", addr);
        r_core_free(core);
        return std::vector<std::pair<uint64_t, uint64_t>>();
    }
    
    // Analyze vector register usage and get translation ranges using new algorithm
    std::vector<std::pair<uint64_t, uint64_t>> ranges = AddrAnalysis::analyze_vector_register(core, func);
    
    // Cleanup core
    r_core_free(core);
    
    printf("[ADDR] Translation ranges analysis completed\n");
    printf("  Translation ranges: %zu\n", ranges.size());
    
    return ranges;
}

uint64_t Addr::get_translation_range_end(uint64_t translation_range_begin) {
    // Find the range with matching begin address
    for (const auto& range : global_translation_ranges) {
        if (range.first == translation_range_begin) {
            return range.second;
        }
    }
    return 0;
}

void Addr::set_translation_range_end(uint64_t translation_range_begin, uint64_t translation_range_end) {
    // Check if range already exists
    for (auto& range : global_translation_ranges) {
        if (range.first == translation_range_begin) {
            range.second = translation_range_end;
            return;
        }
    }
    
    // Add new range if not found
    global_translation_ranges.push_back(std::make_pair(translation_range_begin, translation_range_end));
}


