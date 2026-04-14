#include "addr_analysis.h"

namespace AddrAnalysis {

// Helper function to initialize instruction pointer and address
Instruction* initialize_instruction_pointer(CodeBlock* start_block, VectorInst* source_inst, uint64_t& current_addr) {
    
    // Find the instruction with source_inst->address using index traversal
    size_t source_idx = 0;
    bool found = false;
    
    for (size_t i = 0; i < start_block->instructions.size(); i++) {
        if (start_block->instructions[i]->address == source_inst->address) {
            source_idx = i;
            found = true;
            break;
        }
    }
    
    if (!found) {
        std::cout << "[DEBUG][initialize_instruction_pointer] ERROR: Source instruction not found in block!" << std::endl;
        current_addr = 0;
        return nullptr;
    }
    
    // Return the next instruction after source instruction
    if (source_idx + 1 >= start_block->instructions.size()) {
        std::cout << "[DEBUG][initialize_instruction_pointer] ERROR: Source instruction is last in block!" << std::endl;
        current_addr = 0;
        return nullptr;
    }
    
    Instruction* instr_ptr = start_block->instructions[source_idx + 1];
    current_addr = instr_ptr->address;
    
    std::cout << "[DEBUG][initialize_instruction_pointer] current_addr=0x" << std::hex << current_addr << std::dec <<std::endl;
    return instr_ptr;
}

// Helper function to check if current address is a source instruction with matching target_reg
bool is_matching_source_instruction_at_address(uint64_t current_addr, const std::vector<Source*>& sources, Source* current_source) {
    for (Source* other_source : sources) {
        if (other_source->inst_addr == current_addr) {
            // Check if the target_reg matches the current source's target_reg
            if (other_source->target_reg == current_source->target_reg) {
                return true;
            }
            else {
                return false;
            }
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
        
        std::cout << "\t[DEBUG][tag_vector_instruction] Found vector instruction at 0x" << std::hex << current_addr 
                  << ", mnemonic=" << vec_inst->mnemonic << ", target_reg=" << source->target_reg << std::dec << std::endl;
        
        // Check if this instruction uses the target register
        auto reg_it = vec_inst->reg_sources.find(source->target_reg);
        if (reg_it != vec_inst->reg_sources.end()) {
            // Set source to this register (overwrite if exists)
            vec_inst->reg_sources[source->target_reg] = source;
            std::cout << "\t[DEBUG][tag_vector_instruction] Successfully tagged register v" << source->target_reg << " with source at 0x" 
                      << std::hex << source->inst_addr << std::dec << std::endl;
        } else {
            std::cout << "\t[DEBUG][tag_vector_instruction] Register v" << source->target_reg << " not used in this instruction" << std::endl;
        }
    }
}

// Helper function to find successor blocks and add to worklist
void add_successor_blocks_to_worklist(CodeBlock* current_block, 
                                     std::vector<CodeBlock*>& code_blocks,
                                     std::stack<CodeBlock*>& worklist) {
    for (const auto& jump_target : current_block->jumpto) {
        
        // Find block with this start address
        bool found = false;
        for (CodeBlock* block : code_blocks) {
            if (block->startaddr == jump_target) {
                worklist.push(block);
                found = true;
                break;
            }
        }
        
        if (!found) {
            std::cout << "[DEBUG][add_successor_blocks_to_worklist]   ERROR: Target block not found for 0x" 
                      << std::hex << jump_target << std::dec << std::endl;
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

        "vl2re32.v","vl1re64.v",

        // --- 2. 从标量寄存器广播到向量寄存器 (OP-V opcode) ---
        "vmv.v.x",   // vector = scalar register (broadcast)
        "vfmv.s.f",
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
    
    std::cout << std::endl;
    
    if (is_vector_assignment(mnemonic)) {
        int target_reg = 0;
        if (!reg_nums.empty()) {
            target_reg = reg_nums[0];
        }
        Source* source = new Source(instr->address, target_reg);
        sources.push_back(source);
        vec_inst->reg_sources[target_reg] = source;
        
        std::cout << "[DEBUG][create_vector_instruction] Created source at 0x" << std::hex << instr->address << " for register v" 
                  << target_reg << std::dec << std::endl;
    }
}

void init_sources_insts(std::vector<CodeBlock*>& code_blocks,
                       std::vector<Source*>& sources, 
                       std::map<uint64_t, VectorInst*>& insts) {
    std::cout << "[DEBUG][init_sources_insts] Processing " << code_blocks.size() << " code blocks" << std::endl;
    
    // Initialize sources and instructions by scanning all code blocks
    for (CodeBlock* block : code_blocks) {
        std::cout << "[DEBUG][init_sources_insts] Processing block with " << block->instructions.size() << " instructions" << std::endl;
        
        for (Instruction* instr : block->instructions) {
            std::string mnemonic = instr->opcode;
            
            if (is_vector_instruction(mnemonic)) {
                std::cout << "[DEBUG][init_sources_insts] Found vector instruction: " << mnemonic << " at 0x" << std::hex << instr->address << std::dec << std::endl;
                std::vector<int> reg_nums = parse_vector_operands(instr->operands);
                create_vector_instruction(instr, mnemonic, block, reg_nums, sources, insts);
            }
        }
    }
    
    std::cout << "[DEBUG][init_sources_insts] Created " << sources.size() << " sources and " << insts.size() << " vector instructions\n\n\n" << std::endl;
}

void propagate_source_through_blocks(Source* source, VectorInst* source_inst, CodeBlock* start_block, 
                                   std::vector<CodeBlock*>& code_blocks,
                                   std::vector<Source*>& sources,
                                   std::map<uint64_t, VectorInst*>& insts) {
    std::cout << "[DEBUG][propagate_source_through_blocks] Starting propagation from source at 0x" << std::hex 
              << source->inst_addr << " for register v" << source->target_reg << std::dec << std::endl;
    
    // DFS traversal of basic blocks
    std::stack<CodeBlock*> worklist;
    std::set<uint64_t> visited;  // Track visited instruction addresses
    worklist.push(start_block);
    
    // Initialize instruction pointer and address
    uint64_t current_addr;
    Instruction* instr_ptr = initialize_instruction_pointer(start_block, source_inst, current_addr);
    
    int block_count = 0;
    while (!worklist.empty()) {
        CodeBlock* current_block = worklist.top();
        worklist.pop();
        
        std::cout << "[DEBUG][propagate_source_through_blocks] Processing block " << block_count++ << ": 0x" << std::hex << current_block->startaddr 
                  << " - 0x" << current_block->endaddr << std::dec << std::endl;
        
        // Reset instruction pointer if we're in a new block
        if (current_addr != source_inst->address + source_inst->size) {
            instr_ptr = current_block->instructions[0];
            current_addr = instr_ptr->address;
            std::cout << "[DEBUG][propagate_source_through_blocks] Reset to block start: current_addr=0x" << std::hex << current_addr << std::dec << std::endl;
        }

        if (visited.count(current_addr)) {
            std::cout << "[DEBUG][propagate_source_through_blocks] Already visited address 0x" << std::hex << current_addr << ", skipping" << std::dec << std::endl;
            continue;
        }
        visited.insert(current_addr);

        uint64_t block_end = current_block->endaddr;
        bool path_ended_by_source = false;  // Flag to track if path ended due to source instruction
        
        std::cout << "[DEBUG][propagate_source_through_blocks] Processing instructions from 0x" << std::hex << current_addr 
                  << " to 0x" << block_end << std::dec << std::endl;
        
        // Process instructions in current block using safe index traversal
        size_t start_idx = 0;
        bool found_start = false;
        
        // Find starting instruction index
        for (size_t i = 0; i < current_block->instructions.size(); i++) {
            if (current_block->instructions[i]->address == current_addr) {
                start_idx = i;
                found_start = true;
                break;
            }
        }
        
        if (!found_start) {
            std::cout << "[DEBUG][propagate_source_through_blocks] ERROR: Starting instruction not found in block!" << std::endl;
            continue;
        }
        
        // Process instructions from start_idx to end of block
        for (size_t i = start_idx; i < current_block->instructions.size(); i++) {
            Instruction* instr = current_block->instructions[i];
            current_addr = instr->address;

            // Check if this instruction creates a new source (path ends)
            if (is_matching_source_instruction_at_address(current_addr, sources, source)) {
                path_ended_by_source = true;
                break;
            }
            
            // Tag vector instruction and update register sources
            tag_vector_instruction(current_addr, source, insts);
            
            // Move to next instruction - loop will handle it
            if (i + 1 >= current_block->instructions.size()) {
                break;
            }
        }
        
        // Add successor blocks to worklist only if path didn't end due to source instruction
        if (!path_ended_by_source) {
            add_successor_blocks_to_worklist(current_block, code_blocks, worklist);
        } else {
            std::cout << "[DEBUG][propagate_source_through_blocks] Path ended, not adding successors" << std::endl;
        }
    }
    
    std::cout << "[DEBUG][propagate_source_through_blocks] Completed processing " << block_count << " blocks\n" << std::endl;
}


void tag_sources(std::vector<CodeBlock*>& code_blocks,
                std::vector<Source*>& sources, 
                std::map<uint64_t, VectorInst*>& insts) {
    std::cout << "[DEBUG][tag_sources] Processing " << sources.size() << " sources\n" << std::endl;
    
    for (Source* source : sources) {
        std::cout << "[DEBUG][tag_sources] Processing source at 0x" << std::hex << source->inst_addr 
                  << " for register v" << source->target_reg << std::dec << std::endl;
        
        VectorInst* source_inst = insts[source->inst_addr];
        if (!source_inst) {
            std::cout << "[DEBUG][tag_sources] No vector instruction found for source at 0x" << std::hex << source->inst_addr << std::dec << std::endl;
            continue;
        }
        
        CodeBlock* source_block = source_inst->parent_block;
        if (!source_block) {
            std::cout << "[DEBUG][tag_sources] No parent block found for source instruction at 0x" << std::hex << source->inst_addr << std::dec << std::endl;
            continue;
        }
        
        std::cout << "[DEBUG][tag_sources] Starting propagation for source at 0x" << std::hex << source->inst_addr 
                  << " in block 0x" << source_block->startaddr << std::dec << std::endl;
        
        propagate_source_through_blocks(source, source_inst, source_block, code_blocks, sources, insts);
    }
    
    std::cout << "[DEBUG][tag_sources] Completed processing all sources\n\n\n" << std::endl;
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
                std::cout << "[DEBUG][analyze_source_bit_width] inst at " << std::hex << vec_inst->address << std::dec << " has empty or 1024 reg " << reg_pair.first << std::endl;
            }
            
            if(has_empty_or_1024){
                for (auto& inner_reg_pair : vec_inst->reg_sources) {
                    if (inner_reg_pair.second) {
                        std::cout << "[DEBUG][analyze_source_bit_width] Setting source at 0x" << std::hex << inner_reg_pair.second->inst_addr 
                                  << " to BIT_1024" << std::dec << std::endl;
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
    std::cout << "[DEBUG][judge_sources] Completed processing all sources\n\n\n" << std::endl;
}

bool needs_translation(VectorInst* vec_inst) {
    // Check if instruction needs translation based on register sources
    for (auto& reg_pair : vec_inst->reg_sources) {
        Source* source = reg_pair.second;
        if (source == nullptr || (source && source->attrib == SourceAttrib::BIT_1024)) {
            if (source){
                std::cout << "[DEBUG][needs_translation] inst at " << std::hex << vec_inst->address << std::dec << " needs translation, source at 0x" << std::hex << source->inst_addr << std::dec << " reg is " << reg_pair.first << std::endl;
            }
            else{
                std::cout << "[DEBUG][needs_translation] inst at " << std::hex << vec_inst->address << std::dec << " needs translation, source is null, reg is " << reg_pair.first << std::endl;
            }
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
    
    auto ranges = group_consecutive_addresses(translation_addrs, insts);
    std::cout << "[DEBUG][get_ranges] Generated " << ranges.size() << " translation ranges\n\n\n" << std::endl;
    return ranges;
}

std::vector<std::pair<uint64_t, uint64_t>> analyze_vector_register_binary(std::vector<CodeBlock*>& code_blocks) {
    std::cout << "[DEBUG][analyze_vector_register_binary] Starting analysis with " << code_blocks.size() << " code blocks" << std::endl;
    
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
