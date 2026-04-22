#include "core.h"
#include "utils.h"
#include "vector_translation.h"

namespace BinaryTranslation {
namespace Handle {

void migration_handle(ucontext_t *uc, Instruction *fault_instruction) {
    VectorContext::VectorContextManager::getInstance().copy_uc_to_vc(uc, 0, 0xFFFFFFFF);
    
    auto &addr_manager = Address::AddressManager::getInstance();
    uint64_t rela_addr = addr_manager.to_rela(fault_instruction->pc);
    handle_translation_function(rela_addr);
}

void translation_handle(ucontext_t *uc, Instruction *fault_instruction) {
    // // 现在所有向量指令都模拟执行，所以不需要将vc状态复制回uc
    // auto& vc_manager = VectorContext::VectorContextManager::getInstance();
    // vc_manager.copy_uc_to_vc(uc, 0, 0xFFFFFFFF);
    TranslationSharedLib::call_translation_func(fault_instruction->pc);
    // vc_manager.copy_vc_to_uc(uc, 0, 0xFFFFFFFF);
}

void function_jump_handle(ucontext_t *uc, Instruction *fault_instruction) {
    auto &addr_manager = Address::AddressManager::getInstance();
    
    uint64_t target_addr = 0;
    if (fault_instruction->opcode == "jal"){
        target_addr = std::stoull(fault_instruction->operands[0], nullptr, 16);
    }
    else if(fault_instruction->opcode == "jalr"){
        std::string target_reg = fault_instruction->operands[1];
        // TODO: get target address from target_reg
        int target_reg_index = Utils::reg_name_to_num(target_reg);
        if (target_reg_index == -1) {
            printf("Error: invalid register name: %s\n", target_reg.c_str());
            return;
        }
        target_addr = uc->__gregs[target_reg_index];
        target_addr = addr_manager.to_rela(target_addr);
    }
    else {
        printf("Error: unsupported opcode: %s\n", fault_instruction->opcode.c_str());
        return;
    }
    if (target_addr == 0) {
        printf("Error: invalid target address: 0\n");
        return;
    }
    handle_translation_function(target_addr);
}

void handle_translation_function(uint64_t addr){
    auto& dump_analyzer = Dump::DumpAnalyzer::getInstance();
    auto& translation_handle_manager = TranslationSharedLib::TranslationHandleManager::getInstance();
    auto& patcher = Patch::Patcher::getInstance();
    auto& addr_manager = Address::AddressManager::getInstance();
    
    std::vector<Instruction*> insts = dump_analyzer.select_func_content(addr);
    auto codeblocks = CodeBlock::get_codeblocks_linear(insts);
    auto ranges = TranslationRanges::get_translation_ranges(codeblocks, addr);

    translation_handle_manager.gen_translation_shared_lib(ranges);

    for(auto& range : ranges) {
        auto& abs_range = std::make_pair(addr_manager.to_abs(range.first), addr_manager.to_abs(range.second));
        patcher.patch_range(abs_range);
    }
}

} // namespace Handle
} // namespace BinaryTranslation