#include "../include/addr_analysis.h"
#include "../include/globals.h"
#include <algorithm>
#include <stack>
#include <set>
#include <r_anal.h>
#include <iostream>

namespace AddrAnalysis {

bool is_vector_assignment(const std::string& mnemonic) {
    // Check if instruction is a vector assignment (writes to vector register)
    return mnemonic == "vlw.v" || 
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

void init_sources_insts(RCore *core, RAnalFunction *func, 
                       std::vector<Source*>& sources, 
                       std::map<uint64_t, VectorInst*>& insts) {
    // Initialize sources and instructions by scanning all basic blocks
    std::cout << "[DEBUG] init_sources_insts: Starting analysis of function at 0x" 
              << std::hex << func->addr << std::dec << std::endl;
    
    RListIter *iter;
    void * ptr;
    r_list_foreach (func->bbs, iter, ptr) {
        RAnalBlock *bb = (RAnalBlock *)ptr;        
        std::cout << "[DEBUG] init_sources_insts: Processing basic block at 0x" 
                  << std::hex << bb->addr << " (size: " << std::dec << bb->size << ")" << std::endl;
        
        // Parse instructions in this block using Radare2's analysis
        uint64_t bb_addr = bb->addr;
        uint64_t bb_size = bb->size;
        
        uint64_t current_addr = bb_addr;
        
        while (current_addr < bb_addr + bb_size) {
            // Use Radare2 to analyze instruction at current address
            std::cout << "[DEBUG] init_sources_insts: Analyzing instruction at 0x" 
                      << std::hex << current_addr << std::dec << std::endl;
            
            RAnalOp op = {0};
            ut8 buf[32] = {0}; // Buffer for instruction bytes
            r_io_read_at(core->io, current_addr, buf, sizeof(buf));
            int ret = r_anal_op(func->anal, &op, current_addr, buf, sizeof(buf), R_ARCH_OP_MASK_ALL);

            if (ret > 0) {
                
                std::string instruction_str = op.mnemonic;
                std::cout << "[DEBUG] init_sources_insts: Found instruction at 0x" 
                          << std::hex << current_addr << ": " << instruction_str 
                          << " (size: " << std::dec << (int)op.size << ")" << std::endl;
                // Get instruction mnemonic and parse operands as register numbers
                std::string mnemonic = "";
                std::string operands = "";
                
                // 从instruction_str中解析出助记符和操作数
                if(!instruction_str.empty()) {
                    // 解析助记符和操作数
                    size_t space_pos = instruction_str.find(' ');
                    mnemonic = instruction_str.substr(0, space_pos);
                    operands = space_pos != std::string::npos ? instruction_str.substr(space_pos + 1) : "";
                    std::cout << "[DEBUG] init_sources_insts: Parsed mnemonic='" << mnemonic << "', operands='" << operands << "'" << std::endl;
                }
                std::vector<int> reg_nums;
                // If this is a vector instruction, create vector instruction object
                if (is_vector_instruction(mnemonic)) {
                
                    // Check if op_str is valid before processing
                    if (!operands.empty()) {
                        size_t pos = 0;
                        while (pos < operands.length()) {
                            // Skip whitespace
                            while (pos < operands.length() && isspace(operands[pos])) pos++;
                            if (pos >= operands.length()) break;
                            
                            // Find next comma or end
                            size_t end = pos;
                            while (end < operands.length() && operands[end] != ',') end++;
                            
                            std::string operand = operands.substr(pos, end - pos);
                            
                            // Check if operand is a vector register and extract number
                            if (operand.find("v") == 0) {
                                try {
                                    std::string reg_str = operand.substr(1);
                                    int reg_num = std::stoi(reg_str);
                                    if (reg_num >= 0 && reg_num < 32) {
                                        reg_nums.push_back(reg_num);
                                    }
                                } catch (const std::exception& e) {
                                    std::cout << "[DEBUG] init_sources_insts: Error parsing register number: " << e.what() << std::endl;
                                }
                            }
                            
                            pos = end + 1; // Skip comma
                        }
                    } else {
                        std::cout << "[DEBUG] init_sources_insts: op_str is empty" << std::endl;
                    }
                    std::cout << "[DEBUG] init_sources_insts: Register numbers: ";
                    for(int reg_num : reg_nums) {
                        std::cout << reg_num << " ";
                    }
                    std::cout << std::endl;
                    // Check if this is a vector assignment
                    if (is_vector_assignment(mnemonic)) {
                        // Extract target register from first operand
                        int target_reg = 0;
                        if (!reg_nums.empty()) {
                            target_reg = reg_nums[0]; // First register is target
                        }
                        
                        Source* source = new Source(current_addr, target_reg);
                        sources.push_back(source);
                        std::cout << "[DEBUG] init_sources_insts: Created source at 0x" 
                                << std::hex << current_addr << " for register v" << target_reg << std::dec << std::endl;
                    }
                
                    VectorInst* vec_inst = new VectorInst(current_addr, op.size, mnemonic, bb, reg_nums);
                    insts[current_addr] = vec_inst;
                    std::cout << "[DEBUG] init_sources_insts: Created vector instruction at 0x" 
                              << std::hex << current_addr << " with " << reg_nums.size() 
                              << " registers" << std::dec << std::endl;
                }
                
                // Move to next instruction
                current_addr += op.size;
            }
            r_anal_op_fini(&op);
        }
    }
    
    std::cout << "[DEBUG] init_sources_insts: Analysis complete. Found " 
              << sources.size() << " sources and " << insts.size() 
              << " vector instructions" << std::endl;
}

void tag_sources(RCore *core, RAnalFunction *func,
                  std::vector<Source*>& sources, 
                  std::map<uint64_t, VectorInst*>& insts) {
    // Tag sources using basic block-based DFS scanning
    std::cout << "[DEBUG] tag_sources: Starting source tagging for " 
              << sources.size() << " sources" << std::endl;
    
    // Create a map from basic block address to basic block pointer
    std::map<uint64_t, RAnalBlock*> block_map;
    RListIter *iter;
    void *ptr;
    r_list_foreach (func->bbs, iter, ptr) {
        RAnalBlock *bb = (RAnalBlock *)ptr;
        block_map[bb->addr] = bb;
    }

    for (Source* source : sources) {
        // Find the instruction that creates this source
        auto inst_it = insts.find(source->inst_addr);
        if (inst_it == insts.end()) continue;
        
        VectorInst* source_inst = inst_it->second;
        RAnalBlock* start_block = source_inst->parent_block;
        
        std::cout << "[DEBUG] tag_sources: Processing source at 0x" 
                  << std::hex << source->inst_addr << " for register v" 
                  << source->target_reg << std::dec << std::endl;
        
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
                RAnalOp next_op = {0};
                ut8 next_buf[32] = {0};
                r_io_read_at(core->io, current_addr, next_buf, sizeof(next_buf));
                int next_ret = r_anal_op(func->anal, &next_op, current_addr, next_buf, sizeof(next_buf), R_ARCH_OP_MASK_ALL);
                if (next_ret > 0) {
                    current_addr += next_op.size;
                } else {
                    current_addr += 4; // Assume 4-byte instruction
                }
                r_anal_op_fini(&next_op);
            }
            
            // Add successor blocks to worklist only if path didn't end due to source instruction
            if (!path_ended_by_source) {
                if (current_block->jump) {
                    if (block_map.find(current_block->jump) != block_map.end()) {
                        worklist.push(block_map[current_block->jump]);
                    }
                }
                if (current_block->fail) {
                    if (block_map.find(current_block->fail) != block_map.end()) {
                        worklist.push(block_map[current_block->fail]);
                    }
                }
            }
        }
    }
}

void judge_sources(std::vector<Source*>& sources, 
                    std::map<uint64_t, VectorInst*>& insts) {
    // Judge source attributes using iterative marking algorithm
    std::cout << "[DEBUG] judge_sources: Starting source attribute judgment" << std::endl;
    
    int iteration = 0;
    while (true) {
        iteration++;
        std::cout << "[DEBUG] judge_sources: Iteration " << iteration << std::endl;
        
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
                Source* reg_source = reg_pair.second;
                
                // Check if this register has empty source or 1024 source
                bool has_empty_or_1024 = false;
                if(reg_source == nullptr || reg_source->attrib == SourceAttrib::BIT_1024) {
                    has_empty_or_1024 = true;
                }
                
                if(has_empty_or_1024){
                    for (auto& inner_reg_pair : vec_inst->reg_sources) {
                        if (inner_reg_pair.second) {
                            inner_reg_pair.second->attrib = SourceAttrib::BIT_1024;
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
    std::cout << "[DEBUG] get_ranges: Generating translation ranges from " 
              << sources.size() << " sources and " << insts.size() 
              << " instructions" << std::endl;
    
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
            std::cout << "[DEBUG] get_ranges: Instruction at 0x" 
                      << std::hex << vec_inst->inst_addr << " needs translation" 
                      << std::dec << std::endl;
        }
    }
    
    std::cout << "[DEBUG] get_ranges: Found " << translation_addrs.size() 
              << " instructions needing translation" << std::endl;
    
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
    
    std::cout << "[DEBUG] get_ranges: Generated " << ranges.size() << " translation ranges:" << std::endl;
    for (size_t i = 0; i < ranges.size(); i++) {
        std::cout << "[DEBUG]   Range " << i << ": [0x" << std::hex << ranges[i].first 
                  << ", 0x" << ranges[i].second << ")" << std::dec << std::endl;
    }
    
    return ranges;
}

std::vector<std::pair<uint64_t, uint64_t>> analyze_vector_register(RCore *core, RAnalFunction *func) {
    // Main analysis function using new algorithm
    
    if (!func || !func->bbs) {
        printf("[WARNING] Invalid function or no basic blocks\n");
        return std::vector<std::pair<uint64_t, uint64_t>>();
    }
    
    // Step 1: Initialize sources and instructions
    std::vector<Source*> sources;
    std::map<uint64_t, VectorInst*> insts;
    init_sources_insts(core, func, sources, insts);
    
    printf("[ANALYZE] Initialized %zu sources and %zu vector instructions\n", 
           sources.size(), insts.size());
    
    // Step 2: Tag sources using DFS
    tag_sources(core, func, sources, insts);
    
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
