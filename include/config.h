#ifndef RVMIG_CONFIG_H
#define RVMIG_CONFIG_H

#include "common.h"
// Configuration constants
extern const char* config_migration_lib_name;
extern const char* config_migration_func_name;
extern const uint64_t config_migration_func_offset;
extern const char* config_migration_dump_path;

// System constants
#define MAX_THREADS 64
#define VECTOR_CONTEXT_SIZE 4192

#endif // RVMIG_CONFIG_H
