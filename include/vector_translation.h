#ifndef VECTOR_TRANSLATION_H
#define VECTOR_TRANSLATION_H

#include <vector>
#include <cstdint>
#include <utility>
#include <string>
#include <linux/ptrace.h>
#include "utils.h"

#define _GNU_SOURCE
#include <ucontext.h>
#include <mutex>

namespace BinaryTranslation {
    
    namespace TranslationId {

        // Translation ID manager singleton
        class TranslationIdManager {
            public:
                static TranslationIdManager& getInstance();
                int get_current_translation_id();

            private:
                TranslationIdManager() = default;
                ~TranslationIdManager() = default;
                
                // Delete copy constructor and assignment operator
                TranslationIdManager(const TranslationIdManager&) = delete;
                TranslationIdManager& operator=(const TranslationIdManager&) = delete;
                
                std::map<pid_t, int> pid_tid_map_;
                std::mutex map_mutex_;
                static int translation_id_counter_;
        };

    } // namespace TranslationId

    namespace TranslationRanges {
        // Translation ranges
        std::vector<std::pair<uint64_t, uint64_t>> get_translation_ranges(std::vector<CodeBlock*>& code_blocks, uint64_t addr);
    
    } // namespace TranslationRanges

    namespace TranslationSharedLib {

        // Translation function name prefix
        constexpr const char* translation_func_name_prefix = "translation_func_";
        constexpr const char* translation_assembly_prefix = "translation_asm_";
        constexpr const char* translation_shared_lib_prefix = "translation_lib_";

        // Shared library translation
        void call_translation_func(void *translation_handle, uint64_t fault_addr);
        std::string make_func_name(uint64_t fault_addr);
        std::string make_translation_shared_lib_name(int translation_id);
        
        class TranslationHandleManager {
            public:
            static TranslationHandleManager& getInstance();
                void *get_current_translation_shared_lib_handle();
                void update_translation_handle();
                void gen_translation_shared_lib(std::vector<std::pair<uint64_t, uint64_t>> ranges);
                void compile_translation_shared_lib();
                std::string make_translation_assembly_name(int translation_id);
            
            private:
                TranslationHandleManager() = default;
                ~TranslationHandleManager() = default;
                
                // Delete copy constructor and assignment operator
                TranslationHandleManager(const TranslationHandleManager&) = delete;
                TranslationHandleManager& operator=(const TranslationHandleManager&) = delete;

            public:
                void make_dump_fragments_file(std::vector<std::pair<uint64_t, uint64_t>> ranges, std::string dump_fragments_file_path);

                std::map<int, void*> id_handle_map_;
                std::vector<std::string> assembly_files_;
        };

    } // namespace TranslationSharedLib

    namespace VectorContext {

        // Vector context manager singleton
        class VectorContextManager {

            public:
                static VectorContextManager& getInstance();
                void copy_uc_to_vc(ucontext_t *uc, int translation_id, uint32_t uc_mask);
                void copy_vc_to_uc(int translation_id, ucontext_t *uc, uint32_t vc_mask);
            private:
                VectorContextManager();
                ~VectorContextManager();
                
                // Delete copy constructor and assignment operator
                VectorContextManager(const VectorContextManager&) = delete;
                VectorContextManager& operator=(const VectorContextManager&) = delete;

                
                void* vc_pool_;
        };
            
            
    } // namespace VectorContext
        
} // namespace BinaryTranslation
    
struct __riscv_v_ext_state* get_os_vector_context(ucontext_t *uc);
#endif // VECTOR_TRANSLATION_H
