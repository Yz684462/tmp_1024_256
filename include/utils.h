#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <cstdint>
#include <string_view>
#include <map>
#include <set>

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

    namespace Utils {
        int reg_name_to_num(std::string reg_name);
    }
    namespace Addr {

        // Address manager singleton
        class AddrManager {
            public:
                static AddrManager& getInstance(uint64_t base_addr = 0);
                
                uint64_t to_abs(uint64_t rela_addr);
                uint64_t to_rela(uint64_t abs_addr);

            private:
                AddrManager(uint64_t base_addr) : base_addr_(base_addr) {}
                ~AddrManager() = default;
                
                // Delete copy constructor and assignment operator
                AddrManager(const AddrManager&) = delete;
                AddrManager& operator=(const AddrManager&) = delete;
                
                uint64_t base_addr_;
        };

        // Address utilities
        uint64_t get_shared_lib_base_addr(const std::string& shared_lib_name);

    } // namespace Addr

    namespace Dump {

        // Dump analyzer singleton
        class DumpAnalyzer {
            public:
                static DumpAnalyzer& getInstance(const std::string& dump_file = "");
                
                std::vector<Instruction*> select_func_content(uint64_t addr_inside);
                std::vector<Instruction*> select_snippet(std::pair<uint64_t, uint64_t> range);
                Instruction* parse_line_at_addr(uint64_t addr);
                Instruction* parse_instr_line(std::string line_content);
                void parse_func_line(std::string line_content);
                std::string concat_dump_fragments(const std::vector<std::string>& fragments);
                void write_dump_fragment_to_file(const std::string& filename, const std::string& content);
                int addr_to_line_number(uint64_t addr);
                std::string extract_line_by_line_number(int line_number);

            private:
                DumpAnalyzer(const std::string& dump_file);
                ~DumpAnalyzer() = default;
                
                // Delete copy constructor and assignment operator
                DumpAnalyzer(const DumpAnalyzer&) = delete;
                DumpAnalyzer& operator=(const DumpAnalyzer&) = delete;
                
            public:
                std::vector<std::string> lines_;
                std::map<uint64_t, Instruction*> parsed_lines_;
                std::set<uint64_t> parsed_func_addrs_;
                std::map<uint64_t, int> addr_to_line_map_;
        };

    } // namespace Dump


    namespace CodeBlock_SPACE {
        // Code block utilities

        inline const std::vector<std::string> jmp_instr = {"j", "jal"};
        inline const std::vector<std::string> branch_instr = {"beq", "bne", "beqz", "bnez", "blt", "bge", "bltz", "bgez", "bltu", "bgeu", "blez", "bgtz"};
        inline const std::vector<std::string> return_instr = {"ret"};
        inline const std::vector<std::string> jmp_indirect_instr = {"jalr", "jr"};
        inline const std::vector<std::string> other_instr = {"ebreak"};

        std::vector<CodeBlock*> get_codeblocks_linear(const std::vector<Instruction*>& instructions);

    } // namespace CodeBlock

} // namespace BinaryTranslation

#endif // UTILS_H
