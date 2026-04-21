#include "utils.h"
#include "globals.h"
#include <utility>
#include <fstream>
#include <iostream>
#include <regex>
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace BinaryTranslation {
namespace Dump {

DumpAnalyzer& DumpAnalyzer::getInstance(const std::string& dump_file) {
    static DumpAnalyzer instance(dump_file);
    return instance;
}

DumpAnalyzer::DumpAnalyzer(const std::string& dump_file) {
    std::ifstream file(dump_file);
    if (!file.is_open()) {
        std::cout << "[TEST] Warning: Could not open dump file: " << dump_file << std::endl;
        return;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        lines_.push_back(line);
    }
    file.close();
}

std::vector<std::string> DumpAnalyzer::select_func_content(uint64_t addr_inside){
    
}

std::vector<Instruction*> DumpAnalyzer::select_snippet(std::pair<uint64_t, uint64_t> range){
    std::vector<Instruction*> result;
    uint64_t start_addr = range.first;
    uint64_t end_addr = range.second;
    
    // Iterate through address range and collect instructions
    for (uint64_t addr = start_addr; addr <= end_addr; ) {
        Instruction* instr = parse_line_at_addr(addr);
        if (instr != nullptr) {
            result.push_back(instr);
            // Move to next instruction address
            addr += instr->instrlen;
        } else {
            // Error: no instruction found at this address
            std::cerr << "[ERROR] No instruction found at address 0x" << std::hex << addr << std::endl;
            throw std::runtime_error("Failed to find instruction in address range");
        }
    }
    
    return result;
}

Instruction* DumpAnalyzer::parse_line_at_addr(uint64_t addr){
    // First check if the address is already in parsed_lines_
    auto it = parsed_lines_.find(addr);
    if (it != parsed_lines_.end()) {
        return it->second;
    }
    
    // If not found, parse each line until we find the address
    for (int line_number = 0; line_number < lines_.size(); line_number++) {
        const std::string& line = lines_[line_number];
        Instruction* instr = parse_instr_line(line);
        if (instr != nullptr) {
            addr_to_line_map_[instr->address] = line_number;
            if (instr->address == addr) {
                return instr;
            } else if (instr->address > addr) {
                // If we've passed the target address, no need to continue
                break;
            }
        } else {
            // If not an instruction line, try parsing as function line
            parse_func_line(line);
        }
    }
    
    // If still not found, return nullptr
    return nullptr;
}

Instruction* DumpAnalyzer::parse_instr_line(std::string line_content){
    std::regex pattern(R"(^\s*([0-9a-fA-F]+):\s+([0-9a-fA-F]{4,8})\s+([\w.]+)\s*(.*)$)");
    std::smatch match;
    
    if (std::regex_match(line_content, match, pattern)) {
        std::string address_str = match[1].str();
        uint64_t address = std::stoull(address_str, nullptr, 16);
        std::string machine_code = match[2].str();
        
        // Remove spaces from machine code and convert to lowercase
        machine_code.erase(std::remove(machine_code.begin(), machine_code.end(), ' '), machine_code.end());
        std::transform(machine_code.begin(), machine_code.end(), machine_code.begin(), ::tolower);
        int instrlen = machine_code.length() / 2;

        std::string opcode = match[3].str();
        std::string operand_str = match[4].str();
        
        // Remove comments after # or <
        size_t pos = operand_str.find('#');
        if (pos != std::string::npos) {
            operand_str = operand_str.substr(0, pos);
        }
        pos = operand_str.find('<');
        if (pos != std::string::npos) {
            operand_str = operand_str.substr(0, pos);
        }
        operand_str.erase(0, operand_str.find_first_not_of(" \t"));
        operand_str.erase(operand_str.find_last_not_of(" \t") + 1);
        
        Instruction* instr = new Instruction(opcode, operand_str, address, instrlen);
        parsed_lines_[address] = instr;
        return instr;
    }
    
    // Return nullptr if pattern doesn't match
    return nullptr;
}

void DumpAnalyzer::parse_func_line(std::string line_content) {
    std::regex func_pattern(R"(^\s*([0-9a-fA-F]+)\s+<.*>.*:$)");
    std::smatch match;
    
    if (std::regex_match(line_content, match, func_pattern)) {
        std::string func_addr_str = match[1].str();
        uint64_t func_addr = std::stoull(func_addr_str, nullptr, 16);
        parsed_func_addrs_.insert(func_addr);  // Mark that we found a function at this address
    }
}

std::string DumpAnalyzer::concat_dump_fragments(const std::vector<std::string>& fragments) {
    std::string result;
    for (const auto& fragment : fragments) {
        result += fragment;
        if (&fragment != &fragments.back()) {
            result += "\n";  // Add newline between fragments
        }
    }
    return result;
}

void DumpAnalyzer::write_dump_fragment_to_file(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[ERROR] Failed to open file: " << filename << std::endl;
        throw std::runtime_error("Failed to open file for writing");
    }
    
    file << content;
    file.close();
}

int DumpAnalyzer::addr_to_line_number(uint64_t addr) {
    auto it = addr_to_line_map_.find(addr);
    if (it != addr_to_line_map_.end()) {
        return it->second;
    }
    parse_line_at_addr(addr);
    it = addr_to_line_map_.find(addr);
    if (it != addr_to_line_map_.end()) {
        return it->second;
    }
    return -1;  // Return -1 if address not found
}

std::string DumpAnalyzer::extract_line_by_line_number(int line_number) {
    if (line_number < 0 || line_number >= lines_.size()) {
        return "";
    }
    return lines_[line_number];
}

} // namespace Dump
} // namespace BinaryTranslation