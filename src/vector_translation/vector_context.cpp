#include "vector_translation.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define _GNU_SOURCE
#include <ucontext.h>
#include <linux/ptrace.h>


#define RISCV_V_MAGIC	0x53465457
#define MAX_THREADS 64  
#define VECTOR_CONTEXT_SIZE 4192

struct __riscv_v_ext_state* get_os_vector_context(ucontext_t *uc) {
    struct __riscv_extra_ext_header *ext;
    struct __riscv_v_ext_state *v_ext_state;
    
    ext = (struct __riscv_extra_ext_header *)(&uc->uc_mcontext.__fpregs);
    if (ext->hdr.magic != RISCV_V_MAGIC) {
        fprintf(stderr, "bad vector magic: %x\n", ext->hdr.magic);
        abort();
    }
    
    v_ext_state = (struct __riscv_v_ext_state *)((char *)(ext) + sizeof(*ext));
    return v_ext_state;
}

namespace BinaryTranslation {
namespace VectorContext {

VectorContextManager::VectorContextManager() {
    if (vc_pool_ == nullptr) {
        // Allocate continuous memory pool for all vector contexts
        size_t pool_size = MAX_THREADS * VECTOR_CONTEXT_SIZE;
        vc_pool_ = malloc(pool_size);
        if (!vc_pool_) {
            fprintf(stderr, "Failed to allocate vector context pool\n");
            abort();
        }
    }
}

VectorContextManager::~VectorContextManager() {
    if (vc_pool_) {
        free(vc_pool_);
    }
}

VectorContextManager& VectorContextManager::getInstance() {
    static VectorContextManager instance;
    return instance;
}


void VectorContextManager::copy_uc_to_vc(ucontext_t *uc, int translation_id, uint32_t uc_mask) {
    // Get OS vector context
    struct __riscv_v_ext_state* os_vector_context = get_os_vector_context(uc);
    
    // Save OS vector context to simulated context
    uint8_t* simulated_context = (uint8_t*)vc_pool_ + translation_id * VECTOR_CONTEXT_SIZE;

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
    memcpy(simulated_context, os_vector_context->datap, total_size);
}

void VectorContextManager::copy_vc_to_uc(int translation_id, ucontext_t *uc, uint32_t vc_mask) {
    // Get OS vector context
    struct __riscv_v_ext_state* os_vector_context = get_os_vector_context(uc);
    
    // Restore simulated vector context to OS vector context
    uint8_t* simulated_context = (uint8_t*)vc_pool_ + translation_id * VECTOR_CONTEXT_SIZE;

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
    memcpy(os_vector_context->datap, simulated_context, total_size);
}

} // namespace VectorContext
} // namespace BinaryTranslation
