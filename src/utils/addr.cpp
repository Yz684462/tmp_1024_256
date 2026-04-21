#include "utils.h"
#include "globals.h"
#include <dlfcn.h>
#include <utility>
#include <iostream>

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

uint64_t get_shared_lib_base_addr(void *shared_lib_handle) {
    struct link_map *map;
    if (dlinfo(shared_lib_handle, RTLD_DI_LINKMAP, &map) == 0) {
        return (uint64_t)map->l_addr;
    } else {
        std::cout << "[TEST] Warning: Could not get library base address: " << dlerror() << std::endl;
        return 0;
    }
}

} // namespace Addr
} // namespace BinaryTranslation
