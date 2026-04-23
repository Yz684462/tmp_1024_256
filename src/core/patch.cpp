#include "core.h"
#include <sys/mman.h>

namespace BinaryTranslation {
namespace Patch {

Patcher& Patcher::getInstance() {
    static Patcher instance;
    return instance;
}

void Patcher::patch_addr(uint64_t addr) {
    size_t page_size = getpagesize();
    uintptr_t page_start = addr & ~(page_size - 1);

    // Modify access permissions to make writable
    if (mprotect((void*)page_start, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        return;
    }

    auto &dump_analyzer = Dump::DumpAnalyzer::getInstance();
    Instruction* instr = dump_analyzer.parse_line_at_addr(addr);
    int instr_len = instr->instrlen;

    if( instr_len == 2){
        uint16_t* ptr = (uint16_t*)addr;
        *ptr = 0x9002; // 2 bytes ebreak
        addr_patch_data_[addr] = {.original_bytes_16 = 0x0001, .inst_len = 2};
    }
    else if( instr_len == 4){
        uint32_t* ptr = (uint32_t*)addr;
        *ptr = 0x00100073; // 4 bytes ebreak
        addr_patch_data_[addr] = {.original_bytes_32 = 0x00100073, .inst_len = 4};
    }
    else {
        printf("Error: unsupported instruction length: %d\n", instr_len);
        return;
    }

    __builtin___clear_cache((void*)addr, (void*)(addr + instr_len));
}

void Patcher::restore_addr(uint64_t addr) {
    auto it = addr_patch_data_.find(addr);
    if (it == addr_patch_data_.end()) {
        return;
    }
    
    PatchData& patch_data = it->second;
    size_t page_size = getpagesize();
    uintptr_t page_start = addr & ~(page_size - 1);
    
    // Modify access permissions to make writable
    if (mprotect((void*)page_start, page_size, PROT_READ | PROT_WRITE | PROT_EXEC) != 0) {
        return;
    }
    
    if (patch_data.inst_len == 2) {
        uint16_t* ptr = (uint16_t*)addr;
        *ptr = patch_data.original_bytes_16;
    } else if (patch_data.inst_len == 4) {
        uint32_t* ptr = (uint32_t*)addr;
        *ptr = patch_data.original_bytes_32;
    }
    
    __builtin___clear_cache((void*)addr, (void*)(addr + patch_data.inst_len));
    
    addr_patch_data_.erase(it);
}

void Patcher::patch_range(std::pair<uint64_t, uint64_t> range) {
    // FIXME: patch_addr这里不应该保存addr_patch_data_
    patch_addr(range.first);
    range_patched_[range.first] = range.second;
}

uint64_t Patcher::query_range_end(uint64_t start_addr) {
    auto it = range_patched_.find(start_addr);
    if (it == range_patched_.end()) {
        return 0;
    }
    return it->second;
}

} // namespace Patch
} // namespace BinaryTranslation