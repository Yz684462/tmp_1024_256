#ifndef RVMIG_HANDLE_H
#define RVMIG_HANDLE_H

#include "common.h"
#include <ucontext.h>

class Handle {
public:
    // Handle migration operations
    static void migration_handle(ucontext_t *uc);
    
    // Handle translation operations
    static void translation_handle(ucontext_t *uc, uintptr_t fault_pc);
};

#endif // RVMIG_HANDLE_H
