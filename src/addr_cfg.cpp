#include "addr_cfg.h"
#include "../include/globals.h"
#include "../include/config.h"
#include <r_core.h>
#include <r_anal.h>

namespace AddrCFG {

CFG* build_cfg(RAnalFunction *func, RCore *core, uint64_t migration_addr) {
    CFG *cfg = new CFG();
    
    if (!func) {
        return cfg;
    }
    
    printf("[CFG] Building CFG for function %s at 0x%lx\n", func->name, func->addr);
    if (migration_addr > 0) {
        printf("[CFG] Migration address: 0x%lx - will cut CFG\n", migration_addr);
    }
    
    // Get basic blocks
    RList *blocks = func->bbs;
    RListIter *iter;
    void *ptr;
    
    // Find entry block (first block)
    RAnalBlock *entry_block = nullptr;
    bool found_migration_block = false;
    
    r_list_foreach(blocks, iter, ptr) {
        RAnalBlock *block = reinterpret_cast<RAnalBlock*>(ptr);
        
        // Check if this block contains the migration address
        bool should_include_block = true;
        size_t start_inst_index = 0;
        
        if (migration_addr > 0) {
            if (block->addr <= migration_addr && migration_addr < block->addr + block->size) {
                // This block contains the migration address
                found_migration_block = true;
                // Find the instruction index containing migration address
                for (size_t i = 0; i < block->ninstr; ++i) {
                    ut64 instr_addr = block->addr + r_anal_bb_offset_inst(block, i);
                    if (instr_addr >= migration_addr) {
                        start_inst_index = i;
                        break;
                    }
                }
            } else if (block->addr < migration_addr) {
                // This block is before migration address, skip it
                should_include_block = false;
            }
            // If block->addr > migration_addr, include it fully
        }
        
        if (!should_include_block) {
            continue;
        }
        
        // Create CFG block
        CFGBlock cfg_block = create_cfg_block(block, core, start_inst_index);
        
        // If this is the first block we're including, set it as entry
        if (cfg->blocks.empty()) {
            cfg->entry_block_addr = block->addr;
        }
        
        cfg->blocks.push_back(cfg_block);
        
        printf("  Block @ 0x%lx (size=%lu, instr_count=%lu)\n", 
               block->addr, block->size, cfg_block.instructions.size());
    }
    
    if (cfg->blocks.empty()) {
        printf("[WARNING] No blocks included in CFG\n");
        return cfg;
    }
    
    // Build predecessor relationships
    build_predecessor_relationships(cfg);
    
    printf("[CFG] CFG built with %zu blocks", cfg->blocks.size());
    if (migration_addr > 0) {
        printf(" (cut at 0x%lx)", migration_addr);
    }
    printf("\n");
    
    return cfg;
}

CFGBlock create_cfg_block(RAnalBlock *block, RCore *core, size_t start_inst_index = 0) {
    CFGBlock cfg_block;
    cfg_block.addr = block->addr;
    cfg_block.size = block->size;
    
    // Get instructions in this block, starting from start_inst_index
    for (int i = start_inst_index; i < block->ninstr; ++i) {
        ut64 instr_addr = block->addr + r_anal_bb_offset_inst(block, i);
        
        // Get instruction details
        RAnalOp *op = r_core_anal_op(core, instr_addr, R_ARCH_OP_MASK_VAL);
        if (op && op->mnemonic) {
            Instruction instr = create_instruction(op, instr_addr);
            cfg_block.instructions.push_back(instr);
            r_anal_op_free(op);
        }
    }
    
    // Get successors
    if (block->jump != UT64_MAX) {
        cfg_block.successors.push_back(block->jump);
    }
    if (block->fail != UT64_MAX) {
        cfg_block.successors.push_back(block->fail);
    }
    
    return cfg_block;
}

Instruction create_instruction(RAnalOp *op, uint64_t addr) {
    Instruction instr;
    instr.addr = addr;
    instr.mnemonic = std::string(op->mnemonic);
    
    // Get operands
    for (int j = 0; j < op->n_op; ++j) {
        if (op->ops[j].type != R_ANAL_OP_TYPE_NULL) {
            instr.operands.push_back(std::string(op->ops[j].buf));
        }
    }
    
    return instr;
}

void build_predecessor_relationships(CFG *cfg) {
    for (auto& block : cfg->blocks) {
        for (uint64_t succ_addr : block.successors) {
            // Find successor block and add current block as predecessor
            for (auto& succ_block : cfg->blocks) {
                if (succ_block.addr == succ_addr) {
                    succ_block.predecessors.push_back(block.addr);
                    break;
                }
            }
        }
    }
}

} // namespace AddrCFG
