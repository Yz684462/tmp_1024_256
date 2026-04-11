#include "thread.h"
#include "../include/globals.h"
#include <cstdio>
#include <cstdlib>

// Global variables definition
std::map<std::thread::id, int> global_thread_to_index_map;
int global_next_context_index = 0;

int ThreadManager::get_thread_index() {
    std::thread::id tid = std::this_thread::get_id();
    
    // Check if thread has an assigned index
    auto it = global_thread_to_index_map.find(tid);
    if (it != global_thread_to_index_map.end()) {
        return it->second;
    }
    
    // Thread not found - this should not happen if migration_handle was called first
    fprintf(stderr, "Thread index not found - migration_handle must be called first\n");
    abort();
}

int ThreadManager::get_or_assign_thread_index() {
    std::thread::id tid = std::this_thread::get_id();
    
    // Check if thread already has an assigned index
    auto it = global_thread_to_index_map.find(tid);
    if (it != global_thread_to_index_map.end()) {
        return it->second;
    }
    
    // Check if we have available slots
    if (global_next_context_index >= MAX_THREADS) {
        fprintf(stderr, "Maximum thread limit reached\n");
        abort();
    }
    
    // Assign new index for this thread
    int index = global_next_context_index++;
    global_thread_to_index_map[tid] = index;
    return index;
}
