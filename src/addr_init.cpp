#include "addr_init.h"
#include "../include/globals.h"
#include "../include/config.h"
#include <dlfcn.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>
#include <set>
#include <r_core.h>
#include <r_anal.h>

namespace AddrCore {

RCore* init_r_core() {
    // Initialize r_core
    RCore *core = r_core_new();
    if (!core) {
        fprintf(stderr, "Failed to create RCore\n");
        return nullptr;
    }
    
    // Open and load shared library
    if (!r_core_file_open(core, config_migration_lib_name, R_PERM_R, 0)) {
        fprintf(stderr, "Failed to open migration library: %s\n", config_migration_lib_name);
        r_core_free(core);
        return nullptr;
    }
    r_core_bin_load(core, nullptr, 0);
    
    // Set architecture to RISC-V
    r_config_set(core->config, "asm.arch", "riscv");
    r_config_set_i(core->config, "asm.bits", 64);
    r_config_set(core->config, "bin.relocs.apply", "true");
    r_config_set(core->config, "bin.cache", "true");
    
    return core;
}

RAnalFunction* find_func(uint64_t addr, RCore *core) {
    r_core_cmdf(core, "af @ 0x%llx", rel_pc);

    // 3. Now get the function that contains 'addr'
    RAnalFunction *fcn = r_anal_get_fcn_in(core->anal, addr, R_ANAL_FCN_TYPE_FCN);
    if (!fcn) {
        return nullptr;
    }

    return fcn;
}

} // namespace AddrCore
