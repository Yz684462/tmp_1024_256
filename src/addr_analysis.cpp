#include "addr_analysis.h"
#include "../include/globals.h"
#include "../include/config.h"
#include <r_core.h>
#include <r_anal.h>
#include <algorithm>

namespace AddrAnalysis {

std::vector<std::pair<uint64_t, uint64_t>> analyze_vector_register(CFG *cfg) {
    printf("[ANALYZE] Vector register analysis\n");
    
    if (!cfg) {
        return std::vector<std::pair<uint64_t, uint64_t>>();
    }
    
    if (cfg->blocks.empty()) {
        printf("[WARNING] CFG has no blocks\n");
        return std::vector<std::pair<uint64_t, uint64_t>>();
    }
    
    // 步骤1：寄存器段初始化
    std::map<int, RegisterSegment*> reg_segs = initial_reg_segs(cfg);
    
    // 步骤2：顺序扫描算法
    sequential_scan(reg_segs);
    
    // 步骤3：完成寄存器段处理
    finalize_register_segments(reg_segs);
    
    // 步骤4：生成翻译范围
    std::vector<std::pair<uint64_t, uint64_t>> ranges = generate_translation_ranges(reg_segs);
    
    printf("[ANALYZE] Vector register analysis completed\n");
    printf("  Register segments created: %zu\n", reg_segs.size());
    printf("  Translation ranges generated: %zu\n", ranges.size());
    
    // 清理内存
    for (auto& pair : reg_segs) {
        delete pair.second;
    }
    
    return ranges;
}

bool is_vector_assignment(const Instruction& inst) {
    // 判断指令是否是向量赋值指令
    return inst.mnemonic.find("v") == 0 || 
           inst.mnemonic == "vlw.v" || 
           inst.mnemonic == "vsw.v" ||
           inst.mnemonic == "vmv.v.v" ||
           inst.mnemonic == "vadd.vv" ||
           inst.mnemonic == "vsub.vv" ||
           inst.mnemonic == "vmul.vv" ||
           inst.mnemonic == "vdiv.vv" ||
           inst.mnemonic == "vsetvli";
}

std::vector<RegSegAttrib> query_inst(const Instruction& inst, const std::multimap<uint64_t, RegisterSegment*>& reg_segs) {
    // Query instruction's register attributes
    std::vector<RegSegAttrib> attribs;
    
    for (const auto& operand : inst.operands) {
        if (operand.find("v") == 0) {
            try {
                std::string reg_str = operand.substr(1);
                int reg_num = std::stoi(reg_str);
                if (reg_num >= 0 && reg_num < 32) {
                    // Find segment for this register - look for most recent segment
                    RegisterSegment* latest_seg = nullptr;
                    uint64_t latest_addr = 0;
                    
                    for (const auto& pair : reg_segs) {
                        if (pair.second->reg_num == reg_num && pair.first <= inst.addr) {
                            if (pair.first > latest_addr) {
                                latest_addr = pair.first;
                                latest_seg = pair.second;
                            }
                        }
                    }
                    
                    if (latest_seg) {
                        attribs.push_back(latest_seg->attrib);
                    }
                }
            } catch (...) {
                // Ignore parsing errors
            }
        }
    }
    
    return attribs;
}

int count_unknown(const std::multimap<uint64_t, RegisterSegment*>& reg_segs) {
    // Count how many segments have unknown attribute
    int count = 0;
    for (const auto& pair : reg_segs) {
        if (pair.second->attrib == RegSegAttrib::UNKNOWN) {
            count++;
        }
    }
    return count;
}

uint64_t find_branch_merge_block(uint64_t branch1_addr, uint64_t branch2_addr, CFG* cfg, const std::set<uint64_t>& visited_blocks) {
    // Find merge block of two branches by traversing CFG
    std::vector<uint64_t> worklist1 = {branch1_addr};
    std::vector<uint64_t> worklist2 = {branch2_addr};
    std::set<uint64_t> reachable_from_branch1;
    std::set<uint64_t> reachable_from_branch2;
    
    // Find all blocks reachable from branch1
    while (!worklist1.empty()) {
        uint64_t current = worklist1.back();
        worklist1.pop_back();
        
        if (reachable_from_branch1.find(current) != reachable_from_branch1.end()) {
            continue;
        }
        
        reachable_from_branch1.insert(current);
        
        // Find current block
        const CFGBlock* current_block = nullptr;
        for (const auto& block : cfg->blocks) {
            if (block.addr == current) {
                current_block = &block;
                break;
            }
        }
        
        if (current_block) {
            for (uint64_t succ_addr : current_block->successors) {
                if (reachable_from_branch1.find(succ_addr) == reachable_from_branch1.end()) {
                    worklist1.push_back(succ_addr);
                }
            }
        }
    }
    
    // Find all blocks reachable from branch2
    while (!worklist2.empty()) {
        uint64_t current = worklist2.back();
        worklist2.pop_back();
        
        if (reachable_from_branch2.find(current) != reachable_from_branch2.end()) {
            continue;
        }
        
        reachable_from_branch2.insert(current);
        
        // Find current block
        const CFGBlock* current_block = nullptr;
        for (const auto& block : cfg->blocks) {
            if (block.addr == current) {
                current_block = &block;
                break;
            }
        }
        
        if (current_block) {
            for (uint64_t succ_addr : current_block->successors) {
                if (reachable_from_branch2.find(succ_addr) == reachable_from_branch2.end()) {
                    worklist2.push_back(succ_addr);
                }
            }
        }
    }
    
    // Find first block reachable from both branches (excluding the branch blocks themselves)
    for (uint64_t addr : reachable_from_branch1) {
        if (addr != branch1_addr && addr != branch2_addr && 
            reachable_from_branch2.find(addr) != reachable_from_branch2.end()) {
            // Skip if already visited
            if (visited_blocks.find(addr) == visited_blocks.end()) {
                return addr;
            }
        }
    }
    
    return 0; // No merge block found
}

std::multimap<uint64_t, RegisterSegment*> initial_reg_segs(CFG *cfg) {
    // Register segment initialization algorithm - one instruction address maps to multiple register segments
    std::multimap<uint64_t, RegisterSegment*> reg_segs;
    std::set<uint64_t> visited_blocks;
    
    if (cfg->blocks.empty()) {
        printf("[WARNING] CFG has no blocks\n");
        return reg_segs;
    }
    
    // Initialize register segments: for each register, create a segment with only the first instruction of entry block, attribute = 1024
    // Use instruction address as key, multiple registers can map to same instruction address
    for (int i = 0; i < 32; ++i) {
        if (!cfg->blocks.empty() && !cfg->blocks[0].instructions.empty()) {
            const Instruction* first_inst = &(cfg->blocks[0].instructions[0]);
            RegisterSegment* seg = new RegisterSegment(i, RegSegAttrib::BIT_1024, first_inst->addr);
            seg->instructions.push_back(first_inst);
            seg->end_addr = first_inst->addr;
            reg_segs.insert({first_inst->addr, seg});  // Use multimap for one-to-many mapping
        }
    }
    
    // Start from entry block and scan control flow graph
    std::vector<uint64_t> worklist;
    worklist.push_back(cfg->entry_block_addr);
    visited_blocks.insert(cfg->entry_block_addr);
    
    while (!worklist.empty()) {
        uint64_t current_block_addr = worklist.back();
        worklist.pop_back();
        
        // Find current block
        const CFGBlock* current_block = nullptr;
        for (const auto& block : cfg->blocks) {
            if (block.addr == current_block_addr) {
                current_block = &block;
                break;
            }
        }
        
        if (!current_block) continue;
        
        // Visit current block, scan instructions one by one
        for (const auto& inst : current_block->instructions) {
            if (is_vector_assignment(inst)) {
                // If it's a vector assignment, fill the target register's segment completely
                int target_reg = -1;
                for (const auto& operand : inst.operands) {
                    if (operand.find("v") == 0) {
                        try {
                            std::string reg_str = operand.substr(1);
                            int reg_num = std::stoi(reg_str);
                            if (reg_num >= 0 && reg_num < 32) {
                                target_reg = reg_num;
                                break;
                            }
                        } catch (...) {
                            // Ignore parsing errors
                        }
                    }
                }
                
                if (target_reg >= 0 && target_reg < 32) {
                    // Find and fill current segment for this register
                    RegisterSegment* current_seg = nullptr;
                    auto range = reg_segs.equal_range(inst.addr);
                    for (auto it = range.first; it != range.second; ++it) {
                        if (it->second->reg_num == target_reg) {
                            current_seg = it->second;
                            break;
                        }
                    }
                    
                    // If no segment found for this register at this instruction, look for previous segment
                    if (!current_seg) {
                        for (auto it = reg_segs.rbegin(); it != reg_segs.rend(); ++it) {
                            if (it->second->reg_num == target_reg && it->first < inst.addr) {
                                current_seg = it->second;
                                break;
                            }
                        }
                    }
                    
                    // Fill current segment
                    if (current_seg) {
                        current_seg->end_addr = inst.addr;
                    }
                    
                    // Add another register segment, instructions start from this vector assignment instruction, segment attribute is unknown
                    RegisterSegment* new_seg = new RegisterSegment(target_reg, RegSegAttrib::UNKNOWN, inst.addr);
                    new_seg->instructions.push_back(&inst);
                    reg_segs.insert({inst.addr, new_seg});  // Use multimap for one-to-many mapping
                }
            } else {
                // Non-assignment instruction, add to all unknown segments
                for (auto& pair : reg_segs) {
                    if (pair.second->attrib == RegSegAttrib::UNKNOWN) {
                        pair.second->instructions.push_back(&inst);
                        pair.second->end_addr = inst.addr;
                    }
                }
            }
        }
        
        // Determine next block to visit based on current block's exits
        if (current_block->successors.size() == 2) {
            // Two exits - handle branch logic
            bool both_unvisited = true;
            bool one_visited = false;
            
            for (uint64_t succ_addr : current_block->successors) {
                if (visited_blocks.find(succ_addr) != visited_blocks.end()) {
                    both_unvisited = false;
                    one_visited = true;
                } else {
                    if (one_visited) {
                        // Found unvisited block
                        worklist.push_back(succ_addr);
                        visited_blocks.insert(succ_addr);
                    }
                }
            }
            
            if (both_unvisited) {
                // Branch - blocks on branches are not visited, find merge block
                uint64_t merge_block = find_branch_merge_block(current_block->successors[0], current_block->successors[1], cfg, visited_blocks);
                if (merge_block != 0) {
                    worklist.push_back(merge_block);
                    visited_blocks.insert(merge_block);
                }
                continue;
            }
        } else if (current_block->successors.size() == 1) {
            // One exit, scanning process continues from this exit
            uint64_t succ_addr = current_block->successors[0];
            if (visited_blocks.find(succ_addr) == visited_blocks.end()) {
                worklist.push_back(succ_addr);
                visited_blocks.insert(succ_addr);
            }
        }
    }
    
    // Fill all currently unfilled register segments to the last instruction
    for (auto& pair : reg_segs) {
        if (pair.second->attrib == RegSegAttrib::UNKNOWN && pair.second->instructions.empty()) {
            // Find the last instruction
            if (!cfg->blocks.empty()) {
                const CFGBlock& last_block = cfg->blocks.back();
                if (!last_block.instructions.empty()) {
                    const Instruction* last_inst = &(last_block.instructions.back());
                    pair.second->instructions.push_back(last_inst);
                    pair.second->end_addr = last_inst->addr;
                }
            }
        }
    }
    
    return reg_segs;
}

void sequential_scan(std::multimap<uint64_t, RegisterSegment*>& reg_segs) {
    // Sequential scan algorithm
    while (true) {
        int unknown_cnt_before = count_unknown(reg_segs);
        
        // Process all unknown segments
        for (auto& pair : reg_segs) {
            RegisterSegment* reg_seg = pair.second;
            if (reg_seg->attrib == RegSegAttrib::UNKNOWN) {
                // Query each instruction in the segment
                for (const Instruction* inst : reg_seg->instructions) {
                    std::vector<RegSegAttrib> attribs = query_inst(*inst, reg_segs);
                    
                    // If any attribute is 1024, set current segment to 1024
                    if (std::find(attribs.begin(), attribs.end(), RegSegAttrib::BIT_1024) != attribs.end()) {
                        reg_seg->attrib = RegSegAttrib::BIT_1024;
                        break;
                    }
                }
            }
        }
        
        int unknown_cnt_after = count_unknown(reg_segs);
        
        // If unknown segment count hasn't changed, exit loop
        if (unknown_cnt_after == unknown_cnt_before) {
            break;
        }
    }
    
    // Set all remaining unknown segments to 256
    for (auto& pair : reg_segs) {
        if (pair.second->attrib == RegSegAttrib::UNKNOWN) {
            pair.second->attrib = RegSegAttrib::BIT_256;
        }
    }
}

void finalize_register_segments(std::multimap<uint64_t, RegisterSegment*>& reg_segs) {
    // Finalize register segment processing
    for (auto& pair : reg_segs) {
        RegisterSegment* seg = pair.second;
        if (seg->instructions.empty()) {
            // If segment has no instructions, set to 256
            seg->attrib = RegSegAttrib::BIT_256;
        }
    }
}

std::vector<std::pair<uint64_t, uint64_t>> generate_translation_ranges(
    const std::multimap<uint64_t, RegisterSegment*>& reg_segs) {
    // Generate translation ranges - works on already cut CFG
    std::vector<std::pair<uint64_t, uint64_t>> ranges;
    std::set<std::pair<uint64_t, uint64_t>> unique_ranges;
    
    for (const auto& pair : reg_segs) {
        const RegisterSegment* seg = pair.second;
        if (seg->attrib == RegSegAttrib::BIT_1024) {
            // For 1024 segments, generate translation ranges
            if (!seg->instructions.empty()) {
                uint64_t start_addr = seg->start_addr;
                uint64_t end_addr = seg->end_addr;
                ranges.push_back(std::make_pair(start_addr, end_addr));
            }
        }
    }
    return ranges;
}

} // namespace AddrAnalysis
