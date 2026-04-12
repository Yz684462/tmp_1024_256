#ifndef RVMIG_ADDR_ANALYSIS_H
#define RVMIG_ADDR_ANALYSIS_H

#include <cstdint>
#include <vector>
#include <map>
#include <string>
#include <r_anal.h>

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
    uint64_t inst_addr;           // Instruction address
    uint32_t inst_size;           // Instruction length in bytes
    std::string mnemonic;         // Instruction mnemonic
    std::map<int, Source*> reg_sources;  // Register -> Source map
    RAnalBlock* parent_block;     // Parent basic block pointer
    
    VectorInst(uint64_t addr, uint32_t size, const std::string& mnem, RAnalBlock* block, const std::vector<int>& reg_nums) 
        : inst_addr(addr), inst_size(size), mnemonic(mnem), parent_block(block) {
        // Initialize reg_sources with empty sources for all vector registers
        for (int reg_num : reg_nums) {
            if (reg_num >= 0 && reg_num < 32) {
                reg_sources[reg_num] = nullptr;
            }
        }
    }
};

namespace AddrAnalysis {

// Main analysis function - returns translation ranges
std::vector<std::pair<uint64_t, uint64_t>> analyze_vector_register(RAnalFunction *func);

// New algorithm functions
void init_sources_insts(RAnalFunction *func, 
                       std::vector<Source*>& sources, 
                       std::map<uint64_t, VectorInst*>& insts);

void tag_sources(RAnalFunction *func,
                  std::vector<Source*>& sources, 
                  std::map<uint64_t, VectorInst*>& insts);

void judge_sources(std::vector<Source*>& sources, 
                    std::map<uint64_t, VectorInst*>& insts);

std::vector<std::pair<uint64_t, uint64_t>> get_ranges(
    std::vector<Source*>& sources, 
    std::map<uint64_t, VectorInst*>& insts);

// Helper functions
bool is_vector_assignment(const std::string& mnemonic);
bool is_vector_instruction(const std::string& mnemonic);

} // namespace AddrAnalysis

#endif // RVMIG_ADDR_ANALYSIS_H
