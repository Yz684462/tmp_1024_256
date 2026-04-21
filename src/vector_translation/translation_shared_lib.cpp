#include "vector_translation.h"
#include "utils.h"

namespace BinaryTranslation {
namespace TranslationSharedLib {

void call_translation_func(void *translation_handle, uint64_t fault_addr) {
    std::string func_name = make_func_name(fault_addr);
    void (*translation_func)() = (void(*)())dlsym(translation_handle, func_name.c_str());
    if (translation_func == nullptr) {
        std::cerr << "Failed to load translation function: " << dlerror() << std::endl;
        return;
    }
    translation_func();
}

std::string make_func_name(uint64_t fault_addr) {
    return TranslationSharedLib::translation_func_name_prefix + std::to_string(fault_addr);
}

std::string make_translation_shared_lib_name(int translation_id) {
    return TranslationSharedLib::translation_shared_lib_prefix + std::to_string(translation_id) + ".so";
}

void * TranslationHandleManager::get_current_translation_shared_lib_handle() {
    TranslationId::TranslationIdManager& id_manager = TranslationId::TranslationIdManager::getInstance();
    int current_translation_id = id_manager.get_current_translation_id();
    
    auto it = id_handle_map_.find(current_translation_id);
    if (it != id_handle_map_.end()) {
        return it->second;
    }
    
    return nullptr;
}

void TranslationHandleManager::update_translation_handle(int translation_id, void *new_translation_handle) {
    TranslationId::TranslationIdManager& id_manager = TranslationId::TranslationIdManager::getInstance();
    id_manager.set_current_translation_id(translation_id);
    id_handle_map_[translation_id] = new_translation_handle;
}

void TranslationHandleManager::make_dump_fragments_file(std::vector<std::pair<uint64_t, uint64_t>> ranges, std::string dump_fragments_file_path) {
    std::vector<std::string> dump_fragments;
    Dump::DumpAnalyzer& dump_analyzer = Dump::DumpAnalyzer::getInstance();
    for (const auto& range : ranges) {
        std::string dump_fragment = "";
        int line_number_start = dump_analyzer.addr_to_line_number(range.first);
        int line_number_end = dump_analyzer.addr_to_line_number(range.second);
        for (int line_number = line_number_start; line_number < line_number_end; line_number++) {
            dump_fragment += dump_analyzer.extract_line_by_line_number(line_number);
        }
        dump_fragments.push_back(dump_fragment);
    }
    std::string dump_fragment_concat = dump_analyzer.concat_dump_fragments(dump_fragments);
    dump_analyzer.write_dump_fragment_to_file(dump_fragments_file_path, dump_fragment_concat);
}

void TranslationHandleManager::gen_translation_shared_lib(std::vector<std::pair<uint64_t, uint64_t>> ranges){
    // Prepare parameters
    TranslationId::TranslationIdManager& id_manager = TranslationId::TranslationIdManager::getInstance();
    int translation_id = id_manager.get_current_translation_id();
    std::string dump_fragments_file_path = "dump_fragments_" + std::to_string(translation_id) + ".s";
    make_dump_fragments_file(ranges, dump_fragments_file_path);
    std::string translation_func_names = "";
    for (const auto& range : ranges) {
        translation_func_names += make_func_name(range.first) + ",";
    }
    // Remove trailing comma
    if (!translation_func_names.empty()) {
        translation_func_names.pop_back();
    }
    std::string translation_shared_lib_path = make_translation_shared_lib_name(translation_id);

    std::string command = "python3 scripts/translator.py " + 
        std::to_string(translation_id) + " " + 
        dump_fragments_file_path + " " + 
        translation_func_names + " " + 
        translation_shared_lib_path;
        
    // Execute command
    int result = system(command.c_str());
    if (result != 0) {
        // Handle error if needed
    }

    // Load the generated shared library and update the handle
    void *translation_handle = dlopen(translation_shared_lib_path.c_str(), RTLD_LAZY);
    if (translation_handle == nullptr) {
        std::cerr << "Failed to load translation shared library: " << dlerror() << std::endl;
        return;
    }
    update_translation_handle(translation_id, translation_handle);
}

} // namespace TranslationSharedLib
} // namespace BinaryTranslation