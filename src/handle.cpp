#include "handle.h"
#include "../include/vector_context.h"
#include "../include/thread.h"
#include "../include/cpu.h"
#include "../include/addr.h"
#include "../include/globals.h"
#include "../include/config.h"
#include "../include/asm.h"
#include <cstdlib>
#include <string>
#include <dlfcn.h>
#include <cstring>
#include <cstdio>

void Handle::make_translations(int thread_index) {
    // Call translator.py with thread index and translation ranges (converted to file offsets)
    std::string command = "python3 scripts/translator.py " + std::to_string(thread_index);
    
    // Add translation ranges to command (convert virtual addresses to file offsets)
    for (const auto& range : global_translation_ranges) {
        uint64_t start_offset = range.first - global_migration_lib_base_addr;
        uint64_t end_offset = range.second - global_migration_lib_base_addr;
        command += " " + std::to_string(start_offset) + " " + std::to_string(end_offset);
    }
    
    // Execute command
    int result = system(command.c_str());
    if (result != 0) {
        // Handle error if needed
    }
}

void Handle::load_translation_library(int thread_index) {
    // Load and cache translation library for this thread
    std::string lib_name = "translate_part" + std::to_string(thread_index) + ".so";
    auto it = global_thread_translated_handles.find(thread_index);
    if (it == global_thread_translated_handles.end()) {
        // Load library if not cached
        void* translated_handle = dlopen(lib_name.c_str(), RTLD_LAZY);
        if (translated_handle) {
            global_thread_translated_handles[thread_index] = translated_handle;
        }
    }
}

void Handle::migration_handle(ucontext_t *uc) {
    // Get current thread's index
    int thread_index = ThreadManager::get_or_assign_thread_index();
    
    // Get OS vector context and save to simulated context
    struct __riscv_v_ext_state* os_vector_context = VectorContext::get_os_vector_context(uc);
    VectorContext::save_os_vector_context_to_simulated_vector_context(os_vector_context, thread_index);
    
    // Make translations
    make_translations(thread_index);
    
    // Load translation library
    load_translation_library(thread_index);
   
    // // Switch to regular CPU core
    // switch_cpu_set('x');
}

void Handle::translation_handle(ucontext_t *uc, uintptr_t fault_pc) {
    struct __riscv_v_ext_state *os_vector_context = VectorContext::get_os_vector_context(uc);
    // Get current thread's index (must be assigned by migration_handle first)
    int thread_index = ThreadManager::get_thread_index();
    VectorContext::save_os_vector_context_to_simulated_vector_context(os_vector_context, thread_index);
    
    // Find corresponding translation range (use offset directly)
    std::string func_name;
    uint64_t fault_pc_offset = fault_pc - global_migration_lib_base_addr;
    func_name = "translated_function_" + std::to_string(fault_pc_offset);
    
    // Get cached translation library for this thread
    auto it = global_thread_translated_handles.find(thread_index);
    if (it == global_thread_translated_handles.end()) {
        return; // Translation library not loaded
    }
    void* translated_handle = it->second;
    
    printf("\n[MIGRATION] >>> INSIDE translation_handle for %s <<<\n", func_name.c_str());
    fflush(stdout);
    
    // Get function address directly
    void (*fn)() = (void(*)())dlsym(translated_handle, func_name.c_str());
    if (!fn) {
        return; // Function not found
    }
    // Call the translated function
    fn();
    // Update vector registers from simulated context
    VectorContext::save_simulated_vector_context_to_os_vector_context(os_vector_context, thread_index);
    
    printf("[MIGRATION] >>> translation_handle completed <<<\n\n");
    fflush(stdout);
}


