#ifndef RVMIG_ADDR_H
#define RVMIG_ADDR_H

#include "common.h"
#include "addr_analysis.h"
#include "binary.h"
#include "globals.h"
#include "config.h"

#include <dlfcn.h>
#include <cstdio>

namespace Addr {

// Function declarations
void init_migration(const char* migration_lib_name, const char* migration_func_name, uint64_t migration_offset);

void init_translation_ranges(uint64_t addr);

uint64_t get_translation_range_end(uint64_t translation_range_begin);

void set_translation_range_end(uint64_t translation_range_begin, uint64_t translation_range_end);

} // namespace Addr

#endif // RVMIG_ADDR_H
