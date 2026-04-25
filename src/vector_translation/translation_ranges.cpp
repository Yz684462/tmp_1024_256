#include "vector_translation.h"
#include <vector>
#include <utility>

namespace BinaryTranslation {
namespace TranslationRanges {

bool need_translation(Instruction *inst) {
    std::string opcode = inst->opcode;
    if(opcode[0] == 'v') {
        return true;
    }
    return false;
}

std::vector<std::pair<uint64_t, uint64_t>> group_consecutive_addresses(
    const std::set<std::pair<uint64_t, int>>& translation_addr_sizes) {
    // Group consecutive addresses into ranges using actual instruction lengths
    std::vector<std::pair<uint64_t, uint64_t>> ranges;
    
    if (translation_addr_sizes.empty()) {
        return ranges;
    }
    
    auto it = translation_addr_sizes.begin();
    uint64_t range_start = it->first;
    uint64_t prev_addr = it->first;
    uint64_t prev_end = prev_addr + it->second;
    ++it;
    
    for (; it != translation_addr_sizes.end(); ++it) {
        uint64_t current_addr = it->first;
        
        // If current address is not consecutive with previous, end current range
        if (current_addr != prev_end) {
            ranges.push_back(std::make_pair(range_start, prev_end));
            range_start = current_addr;
        }
        
        prev_addr = current_addr;
        prev_end = prev_addr + it->second;
    }
    
    // Add the final range
    ranges.push_back(std::make_pair(range_start, prev_end));
    return ranges;
}

std::vector<std::pair<uint64_t, uint64_t>> get_translation_ranges(std::vector<CodeBlock*>& code_blocks, uint64_t addr){
    std::set<std::pair<uint64_t, int>> translation_addr_sizes;
    
    for(CodeBlock *block : code_blocks) {
        for(Instruction *inst : block->instructions) {
            if(need_translation(inst)) {
                translation_addr_sizes.insert(std::make_pair(inst->address, inst->instrlen));
            }
        }
    }
    
    std::vector<std::pair<uint64_t, uint64_t>> ranges = group_consecutive_addresses(translation_addr_sizes);

    return ranges;
}

} // namespace TranslationRanges
} // namespace BinaryTranslation