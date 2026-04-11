#include "patch.h"
#include "../include/globals.h"
#include "../include/handle.h"
#include <sys/mman.h>
#include <unistd.h>
#include <ucontext.h>
#include <algorithm>

void Patch::patch(uint64_t addr, uint32_t* original_code) {
    size_t page_size = getpagesize();
    uintptr_t page_start = addr & ~(page_size - 1);

    // Modify access permissions to make writable
    if (mprotect((void*)page_start, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        return;
    }

    uint32_t* ins_ptr = (uint32_t*)addr;
    
    // Save original code if pointer is provided
    if (original_code != nullptr) {
        *original_code = ins_ptr[0];
    }
    
    // Replace with ebreak
    ins_ptr[0] = 0x00100073; // EBREAK
    
    __builtin___clear_cache((char*)addr, (char*)addr + 4);
    
    // Register modified address in global_patched_addrs
    global_patched_addrs.push_back(addr);
}

void Patch::restore(uint64_t addr, uint32_t original_code) {
    size_t page_size = getpagesize();
    uintptr_t page_start = addr & ~(page_size - 1);

    // Modify access permissions to make writable
    if (mprotect((void*)page_start, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        return;
    }

    uint32_t* ins_ptr = (uint32_t*)addr;
    
    // Restore original code
    ins_ptr[0] = original_code;
    
    __builtin___clear_cache((char*)addr, (char*)addr + 4);
    
    // Remove from global_patched_addrs
    auto it = std::find(global_patched_addrs.begin(), global_patched_addrs.end(), addr);
    if (it != global_patched_addrs.end()) {
        global_patched_addrs.erase(it);
    }
}

void Patch::ebreak_handler(int sig, siginfo_t *info, void *context) {
    ucontext_t *uc = (ucontext_t *)context;
    uintptr_t fault_pc = (uintptr_t)info->si_addr;
    
    // Check if it's migration_addr
    if (fault_pc == global_migration_addr) {
        Handle::migration_handle(uc);
        // TODO: 应该要执行原始代码，实现方式应该类似翻译函数那里，应该可以共用一些代码，暂时先不处理
        uc->uc_mcontext.__gregs[REG_PC] = fault_pc + 4;
    }
    // Check if it's in patched_addrs
    else if (std::find(global_patched_addrs.begin(), global_patched_addrs.end(), fault_pc) != global_patched_addrs.end()) {
            Handle::translation_handle(uc, fault_pc);
            uint64_t range_end = Addr::get_translation_range_end(fault_pc);
            uc->uc_mcontext.__gregs[REG_PC] = range_end;
        }  // Error for other cases
    else {
        _exit(1);
    }
}
