#include "utils.h"
#include <sstream>
#include <algorithm>

namespace BinaryTranslation {

Instruction::Instruction(const std::string& opcode, const std::string& operand, 
uint64_t address, int instrlen): address(address), opcode(opcode), instrlen(instrlen), 
        isblockbegin(false), isblockend(false), isret(false) {
    std::vector<std::string> operandlist;
    std::stringstream ss(operand);
    std::string item;
    
    while (std::getline(ss, item, ',')) {
        // Remove leading and trailing whitespace
        item.erase(0, item.find_first_not_of(" \t"));
        item.erase(item.find_last_not_of(" \t") + 1);
        operandlist.push_back(item);
    }
    this->operands = operandlist;
}

CodeBlock::CodeBlock(const std::vector<Instruction*>& instructions)
    : instructions(instructions) {
    
    if (!instructions.empty()) {
        startaddr = instructions[0]->address;
        endaddr = instructions.back()->address;
        jumpto = instructions.back()->jumpto;
        jumpfrom = instructions[0]->jumpfrom;
    }
}

namespace CodeBlock_SPACE {

std::vector<CodeBlock*> get_codeblocks_linear(const std::vector<Instruction*>& instructions) {
    std::vector<CodeBlock*> codeblocks;
    
    int i = 0;
    int j = 0;
    std::vector<std::vector<Instruction*>> blocks;
    
    while (j < instructions.size()) {
        if (instructions[j]->isblockbegin && i != j) {
            blocks.push_back(std::vector<Instruction*>(instructions.begin() + i, instructions.begin() + j));
            i = j;
        }
        if (instructions[j]->isblockend) {
            blocks.push_back(std::vector<Instruction*>(instructions.begin() + i, instructions.begin() + j + 1));
            i = j + 1;
            j++;
        } else {
            j++;
        }
    }
    
    if (i != j) {
        blocks.push_back(std::vector<Instruction*>(instructions.begin() + i, instructions.begin() + j));
    }
    
    int idx = 0;
    for (const auto& ins : blocks) {
        codeblocks.push_back(new CodeBlock(ins));
        idx++;
    }
    
    // Add last instruction executed in sequence to jumpfrom field of block
    std::vector<std::string> all_jmp_instr;
    all_jmp_instr.insert(all_jmp_instr.end(), jmp_instr.begin(), jmp_instr.end());
    all_jmp_instr.insert(all_jmp_instr.end(), branch_instr.begin(), branch_instr.end());
    all_jmp_instr.insert(all_jmp_instr.end(), return_instr.begin(), return_instr.end());
    all_jmp_instr.insert(all_jmp_instr.end(), jmp_indirect_instr.begin(), jmp_indirect_instr.end());
    
    for (int idx = 1; idx < codeblocks.size(); idx++) {
        Instruction* i = codeblocks[idx]->instructions[0];
        uint64_t preAddr = codeblocks[idx-1]->endaddr;
        std::string preInstrOp = codeblocks[idx-1]->instructions.back()->opcode;
        
        if (std::find(all_jmp_instr.begin(), all_jmp_instr.end(), preInstrOp) == all_jmp_instr.end() && preInstrOp.find("...") == std::string::npos) {
            i->jumpfrom.push_back(preAddr);
        }
    }
    
    return codeblocks;
}

} // namespace CodeBlock
} // namespace BinaryTranslation