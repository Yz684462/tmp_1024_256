#ifndef RVMIG_ADDR_ANALYSIS_H
#define RVMIG_ADDR_ANALYSIS_H

#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <stack>
#include <set>
#include <iostream>
#include <unordered_set>

#include "globals.h"
#include "config.h"
#include "binary.h"

// Source attribute enumeration
enum class SourceAttrib {
    UNKNOWN,   // Unknown attribute
    BIT_1024,  // 1024-bit
    BIT_256    // 256-bit
};

// Source structure - represents a register source
struct Source {
    uint64_t inst_addr;           // Instruction address where source is created
    int target_reg;               // Target register number (0-31)
    SourceAttrib attrib;          // Source attribute
    
    Source(uint64_t addr, int reg) 
        : inst_addr(addr), target_reg(reg), attrib(SourceAttrib::UNKNOWN) {}
};

// Vector instruction structure
struct VectorInst {
    uint64_t address;             // Instruction address
    int size;                    // Instruction length
    std::string mnemonic;          // Instruction mnemonic
    CodeBlock* parent_block;      // Parent basic block pointer
    std::map<int, Source*> reg_sources;  // Register to source mapping
    
    VectorInst(uint64_t addr, int size, const std::string& mnemonic, 
                CodeBlock* block, const std::vector<int>& regs)
        : address(addr), size(size), mnemonic(mnemonic), 
          parent_block(block) {
        // Initialize reg_sources with the provided registers
        for (int reg : regs) {
            reg_sources[reg] = nullptr;
        }
    }
};

namespace AddrAnalysis {

// Function declarations
bool is_vector_assignment(const std::string& mnemonic);
bool is_vector_instruction(const std::string& mnemonic);

// Helper functions
Instruction* initialize_instruction_pointer(CodeBlock* start_block, VectorInst* source_inst, uint64_t& current_addr);
bool is_source_instruction_at_address(uint64_t current_addr, const std::vector<Source*>& sources);
void tag_vector_instruction(uint64_t current_addr, Source* source, 
                         std::map<uint64_t, VectorInst*>& insts);
void add_successor_blocks_to_worklist(CodeBlock* current_block, 
                                     std::vector<CodeBlock*>& code_blocks,
                                     std::stack<CodeBlock*>& worklist);

std::vector<int> parse_vector_operands(const std::vector<std::string>& operands);
void create_vector_instruction(Instruction* instr, const std::string& mnemonic, 
                                CodeBlock* block, const std::vector<int>& reg_nums,
                                std::vector<Source*>& sources, std::map<uint64_t, VectorInst*>& insts);
void init_sources_insts(std::vector<CodeBlock*>& code_blocks,
                       std::vector<Source*>& sources, 
                       std::map<uint64_t, VectorInst*>& insts);

void tag_sources(std::vector<CodeBlock*>& code_blocks,
                std::vector<Source*>& sources, 
                std::map<uint64_t, VectorInst*>& insts);

int count_unknown_sources(std::vector<Source*>& sources);
void analyze_source_bit_width(std::map<uint64_t, VectorInst*>& insts);
void judge_sources(std::vector<Source*>& sources, 
                 std::map<uint64_t, VectorInst*>& insts);
bool needs_translation(VectorInst* vec_inst);

std::vector<std::pair<uint64_t, uint64_t>> get_ranges(
    std::vector<Source*>& sources, 
    std::map<uint64_t, VectorInst*>& insts);

// Core function
std::vector<std::pair<uint64_t, uint64_t>> analyze_vector_register_binary(std::vector<CodeBlock*>& code_blocks);

} // namespace AddrAnalysis

#endif // RVMIG_ADDR_ANALYSIS_H
