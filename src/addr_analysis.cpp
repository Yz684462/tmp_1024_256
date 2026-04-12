#include "addr_analysis.h"
#include "../include/globals.h"
#include "../include/config.h"
#include <r_core.h>
#include <r_anal.h>
#include <algorithm>

namespace AddrAnalysis {

bool is_vector_assignment(const std::string& mnemonic) {
    // Check if instruction is a vector assignment (writes to vector register)
    return mnemonic.find("v") == 0 || 
           mnemonic == "vlw.v" || 
           mnemonic == "vsw.v" ||
           mnemonic == "vmv.v.v" ||
           mnemonic == "vadd.vv" ||
           mnemonic == "vsub.vv" ||
           mnemonic == "vmul.vv" ||
           mnemonic == "vdiv.vv" ||
           mnemonic == "vsetvli";
}

bool is_vector_instruction(const std::string& mnemonic) {
    // Check if instruction uses vector registers (mnemonic starts with 'v')
    return mnemonic.find("v") == 0;
}

void init_sources_insts(RAnalFunction *func, 
                       std::vector<Source*>& sources, 
                       std::map<uint64_t, VectorInst*>& insts) {
    // Initialize sources and instructions by scanning all basic blocks
    RListIter *iter;
    RAnalBlock *bb;
    
    r_list_foreach (func->bbs, iter, bb) {
        // Parse instructions in this block using Radare2's analysis
        uint64_t bb_addr = bb->addr;
        uint64_t bb_size = bb->size;
        
        RAnalOp *op;
        uint64_t current_addr = bb_addr;
        
        while (current_addr < bb_addr + bb_size) {
            // Use Radare2 to analyze instruction at current address
            op = r_anal_op(func->anal, current_addr, current_addr - func->addr, R_ARCH_RISCV);
            if (!op) {
                // If analysis fails, assume 4-byte instruction and skip
                current_addr += 4;
                continue;
            }
            
            // Get instruction mnemonic and parse operands as register numbers
            std::string mnemonic = op->mnemonic ? op->mnemonic : "";
            std::vector<int> reg_nums;
            
            // Parse operands to extract vector register numbers
            if (op->opstr) {
                std::string op_str = op->opstr;
                size_t pos = 0;
                while (pos < op_str.length()) {
                    // Skip whitespace
                    while (pos < op_str.length() && isspace(op_str[pos])) pos++;
                    if (pos >= op_str.length()) break;
                    
                    // Find next comma or end
                    size_t end = pos;
                    while (end < op_str.length() && op_str[end] != ',') end++;
                    
                    std::string operand = op_str.substr(pos, end - pos);
                    
                    // Check if operand is a vector register and extract number
                    if (operand.find("v") == 0) {
                        try {
                            std::string reg_str = operand.substr(1);
                            int reg_num = std::stoi(reg_str);
                            if (reg_num >= 0 && reg_num < 32) {
                                reg_nums.push_back(reg_num);
                            }
                        } catch (...) {
                            // Ignore parsing errors
                        }
                    }
                    
                    pos = end + 1; // Skip comma
                }
            }
            
            // Check if this is a vector assignment
            if (is_vector_assignment(mnemonic)) {
                // Extract target register from first operand
                int target_reg = 0;
                if (!reg_nums.empty()) {
                    target_reg = reg_nums[0]; // First register is target
                }
                
                Source* source = new Source(current_addr, target_reg);
                sources.push_back(source);
            }
            
            // If this is also a vector instruction, create vector instruction object
            if (is_vector_instruction(mnemonic)) {
                VectorInst* vec_inst = new VectorInst(current_addr, op->size, mnemonic, bb, reg_nums);
                insts[current_addr] = vec_inst;
            }
            
            // Move to next instruction
            current_addr += op->size;
            r_anal_op_free(op);
        }
    }
}

void tag_sources(RAnalFunction *func,
                  std::vector<Source*>& sources, 
                  std::map<uint64_t, VectorInst*>& insts) {
    // Tag sources using basic block-based DFS scanning
    
    for (Source* source : sources) {
        // Find the instruction that creates this source
        auto inst_it = insts.find(source->inst_addr);
        if (inst_it == insts.end()) continue;
        
        VectorInst* source_inst = inst_it->second;
        RAnalBlock* start_block = source_inst->parent_block;
        
        // DFS traversal of basic blocks
        std::stack<RAnalBlock*> worklist;
        std::set<uint64_t> visited;  // Track visited instruction addresses
        worklist.push(start_block);
        
        while (!worklist.empty()) {
            RAnalBlock* current_block = worklist.top();
            worklist.pop();
            
            // Scan instructions in this basic block
            uint64_t current_addr = current_block->addr;
            uint64_t block_end = current_block->addr + current_block->size;
            bool path_ended_by_source = false;  // Flag to track if path ended due to source instruction
            
            // If this is the source's block, start from the instruction after source
            if (current_block == start_block) {
                current_addr = source->inst_addr + 4; // Move to next instruction
            }
            
            while (current_addr < block_end) {
                // Check if this instruction creates a new source (path ends)
                bool is_source_instruction = false;
                for (Source* other_source : sources) {
                    if (other_source->inst_addr == current_addr) {
                        is_source_instruction = true;
                        break;
                    }
                }
                
                // If this is a source instruction, don't access it and stop this path
                if (is_source_instruction) {
                    path_ended_by_source = true;  // Set flag
                    break; // Stop scanning this path
                }
                
                // Check if this instruction was already visited
                if (visited.count(current_addr)) {
                    break; // Skip if already visited
                }
                visited.insert(current_addr);
                
                // Check if this instruction is in insts
                auto vec_inst_it = insts.find(current_addr);
                if (vec_inst_it != insts.end()) {
                    VectorInst* vec_inst = vec_inst_it->second;
                    
                    // Check if this instruction uses the target register
                    // Use reg_sources to see if this register is used in this instruction
                    auto reg_it = vec_inst->reg_sources.find(source->target_reg);
                    if (reg_it != vec_inst->reg_sources.end()) {
                        // Set source to this register (overwrite if exists)
                        vec_inst->reg_sources[source->target_reg] = source;
                    }
                }
                
                // Move to next instruction
                RAnalOp *next_op = r_anal_op(func->anal, current_addr, current_addr - func->addr, R_ARCH_RISCV);
                if (next_op) {
                    current_addr += next_op->size;
                    r_anal_op_free(next_op);
                } else {
                    current_addr += 4; // Assume 4-byte instruction
                }
            }
            
            // Add successor blocks to worklist only if path didn't end due to source instruction
            if (!path_ended_by_source) {
                for (int i = 0; i < current_block->jumpbb_size; i++) {
                    if (current_block->jumpbb[i]) {
                        worklist.push(current_block->jumpbb[i]);
                    }
                }
                if (current_block->failbb) {
                    worklist.push(current_block->failbb);
                }
            }
        }
    }
}

void judge_sources(std::vector<Source*>& sources, 
                    std::map<uint64_t, VectorInst*>& insts) {
    // Judge source attributes using iterative marking algorithm
    
    while (true) {
        // Count unknown sources before scanning
        int unknown_count_before = 0;
        for (Source* source : sources) {
            if (source->attrib == SourceAttrib::UNKNOWN) {
                unknown_count_before++;
            }
        }
        
        // Scan all instructions to mark sources
        for (auto& pair : insts) {
            VectorInst* vec_inst = pair.second;
            
            // Check each register's source
            for (auto& reg_pair : vec_inst->reg_sources) {
                int reg_num = reg_pair.first;
                Source* reg_source = reg_pair.second;
                
                // Check if this register has empty source or 1024 source
                bool has_empty_or_1024 = false;
                if(reg_source == nullptr || reg_source->attrib == SourceAttrib::BIT_1024) {
                    has_empty_or_1024 = true;
                }
                
                if(has_empty_or_1024){
                    for(auto& reg_pair2 : vec_inst->reg_sources) {
                        if(reg_pair2.second != nullptr) {
                            reg_pair2.second->attrib = SourceAttrib::BIT_1024;
                        }
                    }
                }
            }
        }
        
        // Count unknown sources after scanning
        int unknown_count_after = 0;
        for (Source* source : sources) {
            if (source->attrib == SourceAttrib::UNKNOWN) {
                unknown_count_after++;
            }
        }
        
        // If count didn't change, break the loop
        if (unknown_count_after == unknown_count_before) {
            break;
        }
    }
    
    // Mark remaining unknown sources as 256
    for (Source* source : sources) {
        if (source->attrib == SourceAttrib::UNKNOWN) {
            source->attrib = SourceAttrib::BIT_256;
        }
    }
}

std::vector<std::pair<uint64_t, uint64_t>> get_ranges(
    std::vector<Source*>& sources, 
    std::map<uint64_t, VectorInst*>& insts) {
    // Generate translation ranges based on instruction scanning
    
    std::vector<std::pair<uint64_t, uint64_t>> ranges;
    std::set<uint64_t> translation_addrs;
    
    // Scan all instructions to find those that need translation
    for (auto& pair : insts) {
        VectorInst* vec_inst = pair.second;
        bool needs_translation = false;
        
        // Check if any register has a 1024 source or empty source
        for (auto& reg_pair : vec_inst->reg_sources) {
            Source* source = reg_pair.second;
            if (source == nullptr || (source && source->attrib == SourceAttrib::BIT_1024)) {
                needs_translation = true;
                break;
            }
        }
        
        if (needs_translation) {
            translation_addrs.insert(vec_inst->inst_addr);
        }
    }
    
    // Group consecutive addresses into ranges using actual instruction lengths
    if (translation_addrs.empty()) {
        return ranges;
    }
    
    auto it = translation_addrs.begin();
    uint64_t range_start = *it;
    uint64_t prev_addr = *it;
    uint64_t prev_end = prev_addr + insts[prev_addr]->inst_size;  // End address of previous instruction
    ++it;
    
    for (; it != translation_addrs.end(); ++it) {
        uint64_t current_addr = *it;
        
        // If current address is not consecutive with previous, end current range
        if (current_addr != prev_end) {
            ranges.push_back(std::make_pair(range_start, prev_end));
            range_start = current_addr;
        }
        
        prev_addr = current_addr;
        prev_end = prev_addr + insts[prev_addr]->inst_size;  // Update end address
    }
    
    // Add the final range
    ranges.push_back(std::make_pair(range_start, prev_end));
    
    return ranges;
}

std::vector<std::pair<uint64_t, uint64_t>> analyze_vector_register(RAnalFunction *func) {
    // Main analysis function using new algorithm
    
    if (!func || !func->bbs) {
        printf("[WARNING] Invalid function or no basic blocks\n");
        return std::vector<std::pair<uint64_t, uint64_t>>();
    }
    
    // Step 1: Initialize sources and instructions
    std::vector<Source*> sources;
    std::map<uint64_t, VectorInst*> insts;
    init_sources_insts(func, sources, insts);
    
    printf("[ANALYZE] Initialized %zu sources and %zu vector instructions\n", 
           sources.size(), insts.size());
    
    // Step 2: Tag sources using DFS
    tag_sources(func, sources, insts);
    
    // Step 3: Judge source attributes
    judge_sources(sources, insts);
    
    // Step 4: Generate translation ranges
    std::vector<std::pair<uint64_t, uint64_t>> ranges = get_ranges(sources, insts);
    
    printf("[ANALYZE] Vector register analysis completed\n");
    printf("  Sources created: %zu\n", sources.size());
    printf("  Vector instructions: %zu\n", insts.size());
    printf("  Translation ranges generated: %zu\n", ranges.size());
    
    // Clean up memory
    for (Source* source : sources) {
        delete source;
    }
    for (auto& pair : insts) {
        delete pair.second;
    }
    
    return ranges;
}

} // namespace AddrAnalysis
