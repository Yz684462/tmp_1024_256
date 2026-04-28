#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char** argv){
    std::string current_path = argv[0];
    std::ifstream history_file("history.txt");
    std::string line;
    bool skip = false;
    while (std::getline(history_file, line)) {
        if (line.find(current_path) != std::string::npos) {
            skip = true;
            break;
        }
    }
    history_file.close();
    
    if (skip) {
        std::cout << "[INJECTOR] Skipping patching code with BPF map." << std::endl;
    }
    else{
        // 写入历史记录
        std::ofstream history_file("history.txt", std::ios::app);
        history_file << current_path << std::endl;
        history_file.close();
        
        // 写入bpf map
        // for(auto& range : get_vector_snippet_ranges()) {
        //     patch_code_map(range.first);
        // }
        std::cout << "[INJECTOR] Successfully write patch points to BPF map." << std::endl;
    }
    return 0;
}