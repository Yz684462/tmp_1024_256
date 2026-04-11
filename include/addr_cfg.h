#ifndef ADDR_CFG_H
#define ADDR_CFG_H

#include <cstdint>
#include <vector>
#include <string>
#include <r_anal.h>

// Forward declarations
class RCore;
struct RAnalBlock;
struct RAnalOp;

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

namespace AddrCFG {

// Build control flow graph from function, optionally cut at migration address
CFG* build_cfg(RAnalFunction *func, RCore *core, uint64_t migration_addr = 0);

// Create CFG block from RAnalBlock, optionally start from specific instruction
CFGBlock create_cfg_block(RAnalBlock *block, RCore *core, size_t start_inst_index = 0);

// Create instruction from RAnalOp
Instruction create_instruction(RAnalOp *op, uint64_t addr);

// Build predecessor relationships between blocks
void build_predecessor_relationships(CFG *cfg);

} // namespace AddrCFG

#endif // ADDR_CFG_H
