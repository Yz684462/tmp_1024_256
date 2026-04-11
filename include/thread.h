#ifndef THREAD_H
#define THREAD_H

#include <thread>
#include <map>
#include "config.h"

// Global variables for thread management
extern std::map<std::thread::id, int> global_thread_to_index_map;
extern int global_next_context_index;

class ThreadManager {
public:
    // Get thread index only (for other functions)
    static int get_thread_index();
    
    // Get or assign thread index (only for migration_handle)
    static int get_or_assign_thread_index();
};

#endif // THREAD_H
