#include "utils.h"
#include <utility>
#include <iostream>
#include <dlfcn.h>
#include <link.h> 
#include <cstring>

namespace BinaryTranslation {
namespace Addr {

AddrManager& AddrManager::getInstance(uint64_t base_addr) {
    static AddrManager instance(base_addr);
    return instance;
}

uint64_t AddrManager::to_abs(uint64_t rela_addr) {
    return rela_addr + base_addr_;
}

uint64_t AddrManager::to_rela(uint64_t abs_addr) {
    return abs_addr - base_addr_;
}

template<typename Func>
static int callback_wrapper(struct dl_phdr_info *info, size_t size, void *data) {
    return (*static_cast<Func*>(data))(info, size);
}

uint64_t get_shared_lib_base_addr(const std::string& shared_lib_name) {
    uint64_t base_addr = 0;
    
    auto callback = [&](struct dl_phdr_info *info, size_t size) -> int {
        if (info->dlpi_name && strstr(info->dlpi_name, shared_lib_name.c_str())) {
            base_addr = info->dlpi_addr;
            return 1;
        }
        return 0;
    };
    
    // 使用包装函数传递 lambda
    dl_iterate_phdr(callback_wrapper<decltype(callback)>, &callback);
    
    return base_addr;
}

} // namespace Addr
} // namespace BinaryTranslation
