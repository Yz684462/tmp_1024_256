#ifndef TYPES_H
#define TYPES_H

#include <vector>
#include <string>
#include <cstdint>
#include <ucontext.h>

namespace BinaryTranslation {

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

} // namespace BinaryTranslation

#endif // TYPES_H
