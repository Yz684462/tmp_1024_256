#ifndef VECTOR_CONTEXT_H
#define VECTOR_CONTEXT_H

#include <ucontext.h>
#include <vector>
#include <thread>

#define RISCV_V_MAGIC		0x53465457

struct __riscv_v_ext_state;

// Global variables for simulated vector contexts
extern void* global_simulated_vector_contexts_pool;

class VectorContext {
public:
    // Get OS vector context from ucontext
    static struct __riscv_v_ext_state* get_os_vector_context(ucontext_t *uc);
    
    // Save OS vector context to simulated vector context
    static void save_os_vector_context_to_simulated_vector_context(struct __riscv_v_ext_state *os_vector_context, int thread_index);
    
    // Save simulated vector context to OS vector context
    static void save_simulated_vector_context_to_os_vector_context(struct __riscv_v_ext_state *os_vector_context, int thread_index);
    
    // Initialize vector context pool
    static void initialize_vector_context_pool();
};

#endif // VECTOR_CONTEXT_H
