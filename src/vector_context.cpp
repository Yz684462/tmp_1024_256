#include "vector_context.h"
#include "../include/globals.h"
#include "../include/config.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <linux/ptrace.h>

#define RISCV_V_MAGIC		0x53465457

struct __riscv_v_ext_state* VectorContext::get_os_vector_context(ucontext_t *uc) {
    struct __riscv_extra_ext_header *ext;
    struct __riscv_v_ext_state *v_ext_state;
    
    ext = (void *)(&uc->uc_mcontext.__fpregs);
    if (ext->hdr.magic != RISCV_V_MAGIC) {
        fprintf(stderr, "bad vector magic: %x\n", ext->hdr.magic.magic);
        abort();
    }
    
    v_ext_state = (void *)((char *)(ext) + sizeof(*ext));
    return v_ext_state;
}

void VectorContext::initialize_vector_context_pool() {
    if (global_simulated_vector_contexts_pool == nullptr) {
        // Allocate continuous memory pool for all vector contexts
        size_t pool_size = MAX_THREADS * VECTOR_CONTEXT_SIZE;
        global_simulated_vector_contexts_pool = malloc(pool_size);
        if (!global_simulated_vector_contexts_pool) {
            fprintf(stderr, "Failed to allocate vector context pool\n");
            abort();
        }
    }
}

void VectorContext::save_os_vector_context_to_simulated_vector_context(struct __riscv_v_ext_state *os_vector_context, int thread_index) {
    // Calculate the address of this thread's vector context
    void* simulated_context = (char*)global_simulated_vector_contexts_pool + thread_index * VECTOR_CONTEXT_SIZE;

        // Save vector state
    *(uint64_t*)(simulated_context + 0x1000) = (uint64_t)os_vector_context->vstart;
        //vxsat not handled
        //vxrm not handled
    *(uint64_t*)(simulated_context + 0x1018) = (uint64_t)os_vector_context->vcsr;
    *(uint64_t*)(simulated_context + 0x1020) = (uint64_t)os_vector_context->vl;
    *(uint64_t*)(simulated_context + 0x1028) = (uint64_t)os_vector_context->vtype;
    *(uint64_t*)(simulated_context + 0x1030) = (uint64_t)os_vector_context->vlenb;

    // Save v0 to v31
    int vlen = (uint64_t)os_vector_context->vlenb * 8;
    size_t total_size = 32 * vlen;
    memcpy(simulated_context, os_vector_context->vdata, total_size);
}

void VectorContext::save_simulated_vector_context_to_os_vector_context(struct __riscv_v_ext_state *os_vector_context, int thread_index) {
    
    // Calculate the address of this thread's vector context
    void* simulated_context = (char*)global_simulated_vector_contexts_pool + thread_index * VECTOR_CONTEXT_SIZE;

    // Restore vector state from simulated context
    os_vector_context->vstart = *(uint64_t*)(simulated_context + 0x1000);
    //vxsat not handled
    //vxrm not handled
    os_vector_context->vcsr = *(uint64_t*)(simulated_context + 0x1018);
    os_vector_context->vl = *(uint64_t*)(simulated_context + 0x1020);
    os_vector_context->vtype = *(uint64_t*)(simulated_context + 0x1028);
    os_vector_context->vlenb = *(uint64_t*)(simulated_context + 0x1030);
    
    // Restore vector data using same logic as save
    int vlen = (uint64_t)os_vector_context->vlenb * 8;
    size_t total_size = 32 * vlen;
    memcpy(os_vector_context->vdata, simulated_context, total_size);
}
