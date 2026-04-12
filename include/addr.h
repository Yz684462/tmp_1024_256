#ifndef RVMIG_ADDR_H
#define RVMIG_ADDR_H

#include <cstdint>
#include <vector>
#include <string>
#include <r_anal.h>

// Instruction structure
struct Instruction {
    uint64_t addr;
    std::string mnemonic;
    std::vector<std::string> operands;
};

// CFG block structure
struct CFGBlock {
    uint64_t addr;
    uint64_t size;
    std::vector<Instruction> instructions;
    std::vector<uint64_t> successors;
    std::vector<uint64_t> predecessors;
};

// Control Flow Graph structure
struct CFG {
    std::vector<CFGBlock> blocks;
    uint64_t entry_block_addr;
};

class Addr {
public:
    // Get migration address
    static uint64_t get_migration_addr(void* migration_handle, const char* migration_func_name, uint64_t migration_offset);
    
    // Get translation ranges for given address
    static std::vector<std::pair<uint64_t, uint64_t>> get_translation_ranges(uint64_t addr);
    
    // Get translation range end address
    static uint64_t get_translation_range_end(uint64_t translation_range_begin);
    
    // Set translation range end address
    static void set_translation_range_end(uint64_t translation_range_begin, uint64_t translation_range_end);
};

#endif // RVMIG_ADDR_H
