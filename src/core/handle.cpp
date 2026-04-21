#include "core.h"
#include "utils.h"
#include "vector_translation.h"

namespace BinaryTranslation {
namespace Handle {

void migration_handle(ucontext_t *uc, Instruction *fault_instruction) {
    VectorContext::VectorContextManager::getInstance().copy_uc_to_vc(uc, 0, 0xFFFFFFFF);
    handle_translation_function(fault_instruction->pc);
}

void translation_handle(ucontext_t *uc, Instruction *fault_instruction) {
    auto& vc_manager = VectorContext::VectorContextManager::getInstance();
    vc_manager.copy_uc_to_vc(uc, 0, 0xFFFFFFFF);
    TranslationSharedLib::call_translation_func(fault_instruction->pc);
    vc_manager.copy_vc_to_uc(uc, 0, 0xFFFFFFFF);
}

void function_jump_handle(ucontext_t *uc, Instruction *fault_instruction) {
    uint64_t target_addr = 0;
    if (fault_instruction->opcode == "jal"){
        target_addr = std::stoull(fault_instruction->operands[1], nullptr, 16);
    }
    else if(fault_instruction->opcode == "jalr"){
        std::string target_reg = fault_instruction->operands[0];
        // TODO: get target address from target_reg
        int target_reg_index = Utils::reg_name_to_num(target_reg);
        if (target_reg_index == -1) {
            return;
        }
        target_addr = uc->__gregs[target_reg_index];
    }
    if (target_addr == 0) {
        return;
    }
    handle_translation_function(target_addr);
}

void handle_translation_function(uint64_t addr){
    auto& dump_analyzer = Dump::DumpAnalyzer::getInstance();
    std::vector<Instruction*> insts = dump_analyzer.select_func_content(addr);
    auto codeblocks = CodeBlock::get_codeblocks_linear(insts);
    auto ranges = TranslationRanges::get_translation_ranges(codeblocks, addr);

    auto& translation_handle_manager = TranslationSharedLib::TranslationHandleManager::getInstance();
    translation_handle_manager.gen_translation_shared_lib(ranges);

    auto& patcher = Patch::Patcher::getInstance();
    for(auto& range : ranges) {
        patcher.patch_range(range);
    }
}

} // namespace Handle
} // namespace BinaryTranslation