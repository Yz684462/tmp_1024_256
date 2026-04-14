#include "config.h"

// Configuration macro selection
#ifndef CONFIG_GROUP
#define CONFIG_GROUP 1  // Default to group 1
#endif

// Configuration constants definition based on macro selection
#if CONFIG_GROUP == 1
    // Group 1: Default configuration
    const char* config_migration_lib_name = "../llama_cpp_debug/build/bin/libggml-cpu.so.0";
    const char* config_migration_func_name = "";
    const uint64_t config_migration_func_offset = 0x0;
    const char* config_migration_dump_path = "../dump.s";
#elif CONFIG_GROUP == 2
    // Group 2: Alternative configuration 1
    const char* config_migration_lib_name = "../llama_cpp_debug/build/bin/libggml-cpu.so.0";
    const char* config_migration_func_name = "ggml_vec_soft_max_f32";
    const uint64_t config_migration_func_offset = 0x8;
    const char* config_migration_dump_path = "../dump.s";
#else
    #error "Invalid CONFIG_GROUP value. Must be 1 or 2."
#endif
