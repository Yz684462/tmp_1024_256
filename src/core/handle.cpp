#include "core.h"
#include "vector_translation.h"

namespace BinaryTranslation {
namespace Handle {

void migration_handle(ucontext_t *uc, Instruction *fault_instruction) {
    auto &vector_context_manager =  VectorContext::VectorContextManager::getInstance();
    auto &addr_manager = BinaryTranslation::Addr::AddrManager::getInstance();

    vector_context_manager.copy_uc_to_vc(uc, 0, 0xFFFFFFFF);
    uint64_t rela_addr = addr_manager.to_rela(fault_instruction->address);
    handle_translation_function(rela_addr);
}

void translation_handle(ucontext_t *uc, Instruction *fault_instruction) {
    // // 现在所有向量指令都模拟执行，所以不需要将vc状态复制回uc
    // auto& vc_manager = VectorContext::VectorContextManager::getInstance();
    // vc_manager.copy_uc_to_vc(uc, 0, 0xFFFFFFFF);
    auto &translation_handle_manager = TranslationSharedLib::TranslationHandleManager::getInstance();
    
    void *translation_handle = translation_handle_manager.get_current_translation_shared_lib_handle();
    TranslationSharedLib::call_translation_func(translation_handle, fault_instruction->address);
    // vc_manager.copy_vc_to_uc(uc, 0, 0xFFFFFFFF);
}

void function_jump_handle(ucontext_t *uc, Instruction *fault_instruction) {
    // generate new assembly file
    uint64_t target_addr = get_function_jump_target(uc, fault_instruction);
    handle_translation_function(target_addr);
}

uint64_t get_function_jump_target(ucontext_t *uc, Instruction *fault_instruction) {
    auto &addr_manager = BinaryTranslation::Addr::AddrManager::getInstance();
    
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
            return 0;
        }
        target_addr = uc->uc_mcontext.__gregs[target_reg_index];
        target_addr = addr_manager.to_rela(target_addr);
    }
    else {
        printf("Error: unsupported opcode: %s\n", fault_instruction->opcode.c_str());
        return 0;
    }
    if (target_addr == 0) {
        printf("Error: invalid target address: 0\n");
        return 0;
    }
    return target_addr;
}

void handle_translation_function(uint64_t addr){
    auto& dump_analyzer = Dump::DumpAnalyzer::getInstance();
    auto& translation_handle_manager = TranslationSharedLib::TranslationHandleManager::getInstance();
    auto& patcher = Patch::Patcher::getInstance();
    auto& addr_manager = BinaryTranslation::Addr::AddrManager::getInstance();
    
    std::vector<Instruction*> insts = dump_analyzer.select_func_content(addr);
    auto codeblocks = CodeBlock_SPACE::get_codeblocks_linear(insts);
    auto ranges = TranslationRanges::get_translation_ranges(codeblocks, addr);

    translation_handle_manager.gen_translation_shared_lib(ranges);

    for(auto& range : ranges) {
        auto abs_range = std::make_pair(addr_manager.to_abs(range.first), addr_manager.to_abs(range.second));
        patcher.patch_range(abs_range);
    }

    // compile and load new shared library
    translation_handle_manager.compile_translation_shared_lib();
    translation_handle_manager.update_translation_handle();
}

} // namespace Handle
} // namespace BinaryTranslation