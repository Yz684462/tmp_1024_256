#ifndef RVMIG_PATCH_H
#define RVMIG_PATCH_H

#include "common.h"
#include <signal.h>

class Patch {
public:
    // Patch function at given address, optionally save original code
    static void patch(uint64_t addr, uint32_t* original_code = nullptr);
    
    // Restore original code at given address
    static void restore(uint64_t addr, uint32_t original_code);
    
    // Handle ebreak exception
    static void ebreak_handler(int sig, siginfo_t *info, void *context);
};

#endif // RVMIG_PATCH_H
