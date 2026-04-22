#include "core.h"
#include "utils.h"
#include "vector_translation.h"
#include "globals.h"

namespace BinaryTranslation {
namespace Handler {

void setup_signal_handler() {
    // Set up signal handler for SIGTRAP
    int signal_to_catch = SIGTRAP;
    
    struct sigaction sa;
    sa.sa_sigaction = Handler::ebreak_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    if (sigaction(signal_to_catch, &sa, NULL) != 0) {
        return;
    }
}

void Handler::ebreak_handler(int sig, siginfo_t *info, void *context) {
    
    ucontext_t *uc = (ucontext_t *)context;
    uint64_t fault_pc = (uint64_t)info->si_addr;
    
    Addr::AddrManager &addr_manager = Addr::AddrManager::getInstance();
    Dump::DumpAnalyzer &dump_analyzer = Dump::DumpAnalyzer::getInstance();
    Patch::Patcher &patcher = Patch::Patcher::getInstance();
    
    Instruction *fault_instruction = dump_analyzer.parse_line_at_addr(addr_manager.to_rela(fault_pc));
    
    // Check if it's migration_addr
    if (fault_pc == Migration::migration_addr) {
        Handle::migration_handle(uc, fault_instruction);
        patcher.restore_addr(fault_pc);
        uc->uc_mcontext.__gregs[REG_PC] = fault_pc;
    }
    // Check if it's in patched_addrs
    else if (fault_instruction.opcode == "jal" || fault_instruction.opcode == "jalr") {
        Handle::function_jump_handle(uc, fault_instruction);
        patcher.restore_addr(fault_pc);
        uc->uc_mcontext.__gregs[REG_PC] = fault_pc;
    }  // Error for other cases
    else if (fault_instruction.opcode[0] == "v") {
        Handle::translation_handle(uc, fault_instruction);
        uint64_t range_end = patcher.query_range_end(fault_pc);
        if (range_end == 0) {
            // TODO: handle error
            _exit(1);
        }
        uc->uc_mcontext.__gregs[REG_PC] = range_end;
    }
    else{
        // TODO: handle other cases
        _exit(1);
    }
}

} // namespace Handler
} // namespace BinaryTranslation
