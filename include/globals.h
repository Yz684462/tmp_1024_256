#ifndef GLOBALS_H
#define GLOBALS_H

#include <cstdint>
#include <vector>
#include <map>
#include <thread>
#include "config.h"

// Global variables
extern void* global_migration_lib_handle;
extern uint64_t global_migration_lib_base_addr;  // Migration library base address
extern uint64_t global_migration_addr;
extern uint32_t global_migration_code;  // Original code at migration address
extern void* global_migration_code_handle;  // Handle for migration code shared library
extern std::vector<uint64_t> global_patched_addrs;
extern std::vector<std::pair<uint64_t, uint64_t>> global_translation_ranges;
extern "C" void* global_simulated_vector_contexts_pool;  // Continuous memory pool
extern std::map<int, void*> global_thread_translated_handles;  // Translation library handles for each thread index

#endif // GLOBALS_H
