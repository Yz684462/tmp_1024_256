#include "utils.h"
#include <unordered_map>
#include <string>

namespace BinaryTranslation {
    namespace Utils {
        int reg_name_to_num(const std::string& reg_name) {
            static const std::unordered_map<std::string, int> reg_map = {
                {"ra", 1}, {"sp", 2}, {"gp", 3}, {"tp", 4},
                {"t0", 5}, {"t1", 6}, {"t2", 7}, {"s0", 8}, {"s1", 9},
                {"a0", 10}, {"a1", 11}, {"a2", 12}, {"a3", 13}, {"a4", 14},
                {"a5", 15}, {"a6", 16}, {"a7", 17}, {"s2", 18}, {"s3", 19},
                {"s4", 20}, {"s5", 21}, {"s6", 22}, {"s7", 23}, {"s8", 24},
                {"s9", 25}, {"s10", 26}, {"s11", 27}, {"t3", 28}, {"t4", 29},
                {"t5", 30}, {"t6", 31}
            };
            
            auto it = reg_map.find(reg_name);
            if (it != reg_map.end()) {
                return it->second;
            }
            return -1;
        }
    }
}