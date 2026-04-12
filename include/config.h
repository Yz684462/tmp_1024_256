#ifndef RVMIG_CONFIG_H
#define RVMIG_CONFIG_H

// Configuration constants
extern const char* config_migration_lib_name;
extern const char* config_migration_func_name;
extern const uint64_t config_migration_func_offset;

// System constants
#define MAX_THREADS 64
#define VECTOR_CONTEXT_SIZE 4192

#endif // RVMIG_CONFIG_H
