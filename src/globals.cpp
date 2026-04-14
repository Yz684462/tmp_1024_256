#include "../include/globals.h"

// Global variables definition
void* global_migration_lib_handle = nullptr;
uint64_t global_migration_lib_base_addr = 0;
uint64_t global_migration_addr = 0;
uint32_t global_migration_code = 0;  // Original code at migration address
void* global_migration_code_handle = nullptr;  // Handle for migration code shared library
const std::string global_migration_original_code_func_name = "migration_original_code_func";  // Migration original code function name
std::vector<uint64_t> global_patched_addrs;
std::vector<std::pair<uint64_t, uint64_t>> global_translation_ranges;
void* global_simulated_vector_contexts_pool = nullptr;
std::map<int, void*> global_thread_translated_handles;
