#include "addr_analysis.h"

namespace AddrAnalysis {

// Helper function to initialize instruction pointer and address
Instruction* initialize_instruction_pointer(CodeBlock* start_block, VectorInst* source_inst, uint64_t& current_addr) {
    Instruction* instr_ptr = start_block->instructions[0];
    while(instr_ptr->address != source_inst->address) {
        instr_ptr++;
    }
    instr_ptr++;
    current_addr = std::stoull(instr_ptr->address, nullptr, 16);
    return instr_ptr;
}

// Helper function to check if current address is a source instruction
bool is_source_instruction_at_address(uint64_t current_addr, const std::vector<Source*>& sources) {
    for (Source* other_source : sources) {
        if (other_source->inst_addr == current_addr) {
            return true;
        }
    }
    return false;
}

// Helper function to tag vector instruction and update register sources
void tag_vector_instruction(uint64_t current_addr, Source* source, 
                         std::map<uint64_t, VectorInst*>& insts) {
    auto vec_inst_it = insts.find(current_addr);
    if (vec_inst_it != insts.end()) {
        VectorInst* vec_inst = vec_inst_it->second;
        
        // Check if this instruction uses the target register
        auto reg_it = vec_inst->reg_sources.find(source->target_reg);
        if (reg_it != vec_inst->reg_sources.end()) {
            // Set source to this register (overwrite if exists)
            vec_inst->reg_sources[source->target_reg] = source;
        }
    }
}

// Helper function to find successor blocks and add to worklist
void add_successor_blocks_to_worklist(CodeBlock* current_block, 
                                     std::vector<CodeBlock*>& code_blocks,
                                     std::stack<CodeBlock*>& worklist) {
    for (const auto& jump_target : current_block->jumpto) {
        // Find block with this start address
        for (CodeBlock* block : code_blocks) {
            if (block->startaddr == jump_target) {
                worklist.push(block);
                break;
            }
        }
    }
}

bool is_vector_assignment(const std::string& mnemonic) {
    // RVV 中从非向量源写入向量寄存器的指令集合
    static const std::unordered_set<std::string> vector_assignment_ops = {
        // --- 1. 从内存加载到向量寄存器 (LOAD-FP opcode) ---
        "vle8.v", "vle16.v", "vle32.v", "vle64.v",         // unit-stride load
        "vlse8.v", "vlse16.v", "vlse32.v", "vlse64.v",     // strided load
        "vid.v",

        "vlm.v",                                           // mask load
        "vlre8.v", "vlre16.v", "vlre32.v", "vlre64.v",     // whole register load

        "vluxei8.v", "vluxei16.v", "vluxei32.v", "vluxei64.v",   // indexed unordered load
        "vloxei8.v", "vloxei16.v", "vloxei32.v", "vloxei64.v",   // indexed ordered load

        // fault-only-first variants (also loads)
        "vl8ff.v", "vl16ff.v", "vl32ff.v", "vl64ff.v",

        // segment loads (nf=2~8, example for nf=2)
        "vl2e8.v", "vl2e16.v", "vl2e32.v", "vl2e64.v",
        "vl4e8.v", "vl4e16.v", "vl4e32.v", "vl4e64.v",
        "vl8e8.v", "vl8e16.v", "vl8e32.v", "vl8e64.v",

        // --- 2. 从标量寄存器广播到向量寄存器 (OP-V opcode) ---
        "vmv.v.x",   // vector = scalar register (broadcast)

        // --- 3. 用立即数初始化向量寄存器 (OP-V opcode) ---
        "vmv.v.i"    // vector = imm5 (-16 ~ 15)
    };

    return vector_assignment_ops.count(mnemonic) > 0;
}

bool is_vector_instruction(const std::string& mnemonic) {
    // Check if instruction uses vector registers (mnemonic starts with 'v')
    return mnemonic.find("v") == 0;
}

std::vector<int> parse_vector_operands(const std::vector<std::string>& operands) {
    // Parse vector operands and extract register numbers
    std::vector<int> reg_nums;
    
    if (operands.empty()) {
        return reg_nums;
    }
    
    for (const auto& operand : operands) {
        // Trim whitespace
        std::string trimmed = operand;
        trimmed.erase(0, trimmed.find_first_not_of(" \t"));
        trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
        
        // Check if operand is a vector register
        if (trimmed.find("v") == 0) {
            try {
                std::string reg_str = trimmed.substr(1);
                int reg_num = std::stoi(reg_str);
                if (reg_num >= 0 && reg_num < 32) {
                    reg_nums.push_back(reg_num);
                }
            } catch (const std::exception& e) {
                // Ignore parsing errors
            }
        }
    }
    
    return reg_nums;
}

void create_vector_instruction(Instruction* instr, const std::string& mnemonic, 
                                CodeBlock* block, const std::vector<int>& reg_nums,
                                std::vector<Source*>& sources, std::map<uint64_t, VectorInst*>& insts) {
    // Create vector instruction and handle source creation if needed
    VectorInst* vec_inst = new VectorInst(instr->address, instr->instrlen, mnemonic, block, reg_nums);
    insts[instr->address] = vec_inst;
    if (is_vector_assignment(mnemonic)) {
        int target_reg = 0;
        if (!reg_nums.empty()) {
            target_reg = reg_nums[0]; // First register is target
        }
        Source* source = new Source(instr->address, target_reg);
        sources.push_back(source);
        vec_inst->reg_sources[target_reg] = source;
    }
}

void init_sources_insts(std::vector<CodeBlock*>& code_blocks,
                       std::vector<Source*>& sources, 
                       std::map<uint64_t, VectorInst*>& insts) {
    // Initialize sources and instructions by scanning all code blocks
    for (CodeBlock* block : code_blocks) {
        // Process instructions in this block
        for (Instruction* instr : block->instructions) {
            std::string mnemonic = instr->opcode;
            
            if (is_vector_instruction(mnemonic)) {
                std::vector<int> reg_nums = parse_vector_operands(instr->operands);
                create_vector_instruction(instr, mnemonic, block, reg_nums, sources, insts);
            }
        }
    }
}

void propagate_source_through_blocks(Source* source, VectorInst* source_inst, CodeBlock* start_block, 
                                   std::vector<CodeBlock*>& code_blocks,
                                   std::vector<Source*>& sources,
                                   std::map<uint64_t, VectorInst*>& insts) {
    // DFS traversal of basic blocks
    std::stack<CodeBlock*> worklist;
    std::set<uint64_t> visited;  // Track visited instruction addresses
    worklist.push(start_block);
    
    // Initialize instruction pointer and address
    uint64_t current_addr;
    Instruction* instr_ptr = initialize_instruction_pointer(start_block, source_inst, current_addr);
    
    while (!worklist.empty()) {
        CodeBlock* current_block = worklist.top();
        worklist.pop();
        
        // Reset instruction pointer if we're in a new block
        if (current_addr != source_inst->address + source_inst->size) {
            instr_ptr = current_block->instructions[0];
            current_addr = instr_ptr->address;
        }

        if (visited.count(current_addr)) {
            continue;
        }
        visited.insert(current_addr);

        uint64_t block_end = current_block->endaddr;
        bool path_ended_by_source = false;  // Flag to track if path ended due to source instruction
        
        // Process instructions in current block
        while (true) {
            // Check if this instruction creates a new source (path ends)
            if (is_source_instruction_at_address(current_addr, sources)) {
                path_ended_by_source = true;
                break;
            }
            
            // Tag vector instruction and update register sources
            tag_vector_instruction(current_addr, source, insts);
            
            // Move to next instruction
            if (current_addr >= block_end){
                break
            }
            instr_ptr++;
            current_addr = instr_ptr->address;
        }
        
        // Add successor blocks to worklist only if path didn't end due to source instruction
        if (!path_ended_by_source) {
            add_successor_blocks_to_worklist(current_block, code_blocks, worklist);
        }
    }
}


void tag_sources(std::vector<CodeBlock*>& code_blocks,
                std::vector<Source*>& sources, 
                std::map<uint64_t, VectorInst*>& insts) {
    for (Source* source : sources) {
        VectorInst* source_inst = insts[source->inst_addr];
        if (!source_inst) continue;
        
        CodeBlock* source_block = source_inst->parent_block;
        if (!source_block) continue;
        
        // Propagate source through reachable code blocks
        propagate_source_through_blocks(source, source_inst, source_block, code_blocks, sources, insts);
    }
}

int count_unknown_sources(std::vector<Source*>& sources) {
    // Count sources with unknown attributes
    int count = 0;
    for (Source* source : sources) {
        if (source->attrib == SourceAttrib::UNKNOWN) {
            count++;
        }
    }
    return count;
}

void analyze_source_bit_width(std::map<uint64_t, VectorInst*>& insts) {
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
                return;
            }
        }
    }
}

void judge_sources(std::vector<Source*>& sources, 
                 std::map<uint64_t, VectorInst*>& insts) {
    while(true){
        int unknown_count_before = count_unknown_sources(sources);
        analyze_source_bit_width(insts);
        int unknown_count_after = count_unknown_sources(sources);
        if(unknown_count_before == unknown_count_after){
            break;
        }
    }
    // 后面没有再把剩余的unknown标记成256，因为实际上不需要判断256，只判断1024
}

bool needs_translation(VectorInst* vec_inst) {
    // Check if instruction needs translation based on register sources
    for (auto& reg_pair : vec_inst->reg_sources) {
        Source* source = reg_pair.second;
        if (source == nullptr || (source && source->attrib == SourceAttrib::BIT_1024)) {
            return true;
        }
    }
    return false;
}

std::vector<std::pair<uint64_t, uint64_t>> group_consecutive_addresses(
    const std::set<uint64_t>& translation_addrs,
    std::map<uint64_t, VectorInst*>& insts) {
    // Group consecutive addresses into ranges using actual instruction lengths
    std::vector<std::pair<uint64_t, uint64_t>> ranges;
    
    if (translation_addrs.empty()) {
        return ranges;
    }
    
    auto it = translation_addrs.begin();
    uint64_t range_start = *it;
    uint64_t prev_addr = *it;
    uint64_t prev_end = prev_addr + insts[prev_addr]->size;
    ++it;
    
    for (; it != translation_addrs.end(); ++it) {
        uint64_t current_addr = *it;
        
        // If current address is not consecutive with previous, end current range
        if (current_addr != prev_end) {
            ranges.push_back(std::make_pair(range_start, prev_end));
            range_start = current_addr;
        }
        
        prev_addr = current_addr;
        prev_end = prev_addr + insts[prev_addr]->size;
    }
    
    // Add the final range
    ranges.push_back(std::make_pair(range_start, prev_end));
    return ranges;
}

std::vector<std::pair<uint64_t, uint64_t>> get_ranges(
    std::vector<Source*>& sources, 
    std::map<uint64_t, VectorInst*>& insts) {
    // Generate translation ranges based on instruction scanning
    std::set<uint64_t> translation_addrs;
    
    // Scan all instructions to find those that need translation
    for (auto& pair : insts) {
        VectorInst* vec_inst = pair.second;
        if (needs_translation(vec_inst)) {
            translation_addrs.insert(vec_inst->address);
        }
    }
    
    return group_consecutive_addresses(translation_addrs, insts);
}

std::vector<std::pair<uint64_t, uint64_t>> analyze_vector_register_binary(std::vector<CodeBlock*>& code_blocks) {
    // Main analysis function using binary-based algorithm
    if (code_blocks.empty()) {
        return std::vector<std::pair<uint64_t, uint64_t>>();
    }
    
    std::vector<Source*> sources;
    std::map<uint64_t, VectorInst*> insts;
    
    // Initialize sources and instructions
    init_sources_insts(code_blocks, sources, insts);
    
    // Tag sources
    tag_sources(code_blocks, sources, insts);
    
    // Judge source attributes
    judge_sources(sources, insts);
    
    // Generate translation ranges
    return get_ranges(sources, insts);
}

} // namespace AddrAnalysis
