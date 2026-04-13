#ifndef RVMIG_BINARY_H
#define RVMIG_BINARY_H

#include "common.h"

// Instruction type definitions
const std::vector<std::string> jmp_instr = {"j", "jal"};
const std::vector<std::string> branch_instr = {"beq", "bne", "beqz", "bnez", "blt", "bge", "bltz", "bgez", "bltu", "bgeu", "blez", "bgtz"};
const std::vector<std::string> return_instr = {"ret"};
const std::vector<std::string> jmp_indirect_instr = {"jalr", "jr"};
const std::vector<std::string> other_instr = {"ebreak"};


// Core classes
class Instruction {
public:
    uint64_t address;
    std::string opcode;
    std::vector<std::string> operands;
    int instrlen;
    std::vector<uint64_t> jumpto;
    std::vector<uint64_t> jumpfrom;
    bool isblockbegin;
    bool isblockend;
    bool isret;

    Instruction(const std::string& opcode, const std::string& operand, 
                uint64_t address = 0x0000, int instrlen = 0);
    
};

class CodeBlock {
public:
    std::vector<Instruction*> instructions;
    uint64_t startaddr;
    uint64_t endaddr;
    std::vector<uint64_t> jumpto;
    std::vector<uint64_t> jumpfrom;

    CodeBlock(const std::vector<Instruction*>& instructions);
};

class Binary {
public:
    std::vector<CodeBlock*> code_blocks;

    Binary(const std::string& dump_path, uint64_t addr);
    ~Binary();
};

// Utility functions
std::vector<std::string> split_operand(const std::string& operand);
bool contains(const std::vector<std::string>& vec, const std::string& str);
std::string extract_func_contain_addr(const std::string& dump_content, uint64_t addr);
void addjumpto_target(Instruction* instr);
void identify_blockend(Instruction* instr);
void update_jump_aim(std::map<uint64_t, Instruction*>& instructions, std::vector<std::pair<std::string, std::string>>& erraddr);
std::pair<std::map<uint64_t, Instruction*>, std::vector<std::pair<std::string, std::string>>> 
parse_objdump_output(const std::string& dump_content);

// Core functions
std::pair<std::vector<Instruction*>, std::vector<std::pair<std::string, std::string>>> 
disam_binary_with_dump_func_contain_addr(const std::string& dump_path, uint64_t addr);

std::vector<CodeBlock*> get_codeblocks_linear(const std::vector<Instruction*>& instructions);

#endif // RVMIG_BINARY_H
