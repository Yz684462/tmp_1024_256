#include "addr.h"

void Addr::init_migration(const char* migration_lib_name, const char* migration_func_name, uint64_t migration_offset) {
    // Open migration library
    void* migration_handle = dlopen(migration_lib_name, RTLD_LAZY);
    if (!migration_handle) {
        std::cerr << "Failed to open library: " << migration_lib_name << std::endl;
        return;
    }
    
    // Get migration library base address
    struct link_map *map;
    uint64_t lib_base_addr = 0;
    if (dlinfo(migration_handle, RTLD_DI_LINKMAP, &map) == 0) {
        lib_base_addr = (uint64_t)map->l_addr;
    } else {
        dlclose(migration_handle);
        return;
    }

    // Use dlsym to find function address
    void* func_addr = dlsym(migration_handle, migration_func_name);
    if (!func_addr) {
        std::cerr << "Failed to find function: " << migration_func_name << std::endl;
        dlclose(migration_handle);
        return;
    }
    
    // Add offset to function address
    uint64_t result_addr = (uint64_t)func_addr + migration_offset;
    
    // Set global variables
    if (global_migration_lib_handle == nullptr) {
        global_migration_lib_handle = migration_handle;
        global_migration_lib_base_addr = lib_base_addr;
        global_migration_addr = result_addr;
    }
}

void Addr::init_translation_ranges(uint64_t addr) {
    printf("[ADDR] Getting translation ranges for address 0x%lx\n", addr);
    uint64_t rela_addr = addr - global_migration_lib_base_addr;
    Binary func_bin(config_migration_dump_path,  rela_addr);
    // Analyze vector register usage using new algorithm
    std::vector<std::pair<uint64_t, uint64_t>> ranges = AddrAnalysis::analyze_vector_register_binary(func_bin.code_blocks);
    for (auto& range : ranges){
        range.first += global_migration_lib_base_addr;
        range.second += global_migration_lib_base_addr;
    }
    printf("[ADDR] Translation ranges analysis completed\n");
    printf("  Translation ranges: %zu\n", ranges.size());
    
    // Set global translation ranges
    global_translation_ranges = ranges;
}

uint64_t Addr::get_translation_range_end(uint64_t translation_range_begin) {
    // Find range with matching begin address
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
