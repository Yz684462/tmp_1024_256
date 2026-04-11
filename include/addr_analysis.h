#ifndef ADDR_ANALYSIS_H
#define ADDR_ANALYSIS_H

#include <cstdint>
#include <vector>
#include <string>
#include <set>
#include <map>

// Forward declarations
struct CFG;
struct CFGBlock;
struct Instruction;

// 寄存器段属性枚举
enum class RegSegAttrib {
    BIT_1024,    // 1024位可取值
    BIT_256,     // 256位
    UNKNOWN      // 未知
};

// 寄存器段结构
struct RegisterSegment {
    int reg_num;                              // 寄存器号 (0-31)
    RegSegAttrib attrib;                      // 段属性
    uint64_t start_addr;                      // 段起始地址
    uint64_t end_addr;                        // 段结束地址
    std::vector<const Instruction*> instructions;  // 段内指令列表
    
    RegisterSegment(int num, RegSegAttrib attr, uint64_t start) 
        : reg_num(num), attrib(attr), start_addr(start), end_addr(start) {}
};

namespace AddrAnalysis {

// 主要分析函数 - 返回翻译范围
std::vector<std::pair<uint64_t, uint64_t>> analyze_vector_register(CFG *cfg);

// 辅助函数
bool is_vector_assignment(const Instruction& inst);
std::vector<RegSegAttrib> query_inst(const Instruction& inst, const std::multimap<uint64_t, RegisterSegment*>& reg_segs);
int count_unknown(const std::multimap<uint64_t, RegisterSegment*>& reg_segs);
uint64_t find_branch_merge_block(uint64_t branch1_addr, uint64_t branch2_addr, CFG* cfg, const std::set<uint64_t>& visited_blocks);

// 寄存器段初始化算法 - 一条指令地址映射到多个寄存器段
std::multimap<uint64_t, RegisterSegment*> initial_reg_segs(CFG *cfg);

// 顺序扫描算法
void sequential_scan(std::multimap<uint64_t, RegisterSegment*>& reg_segs);

// 工具函数
RegSegAttrib get_reg_seg_attrib(int reg_num, const std::multimap<uint64_t, RegisterSegment*>& reg_segs);
void set_reg_seg_attrib(int reg_num, RegSegAttrib attrib, std::multimap<uint64_t, RegisterSegment*>& reg_segs);
void finalize_register_segments(std::multimap<uint64_t, RegisterSegment*>& reg_segs);

// 翻译范围生成
std::vector<std::pair<uint64_t, uint64_t>> generate_translation_ranges(
    const std::multimap<uint64_t, RegisterSegment*>& reg_segs);

} // namespace AddrAnalysis

#endif // ADDR_ANALYSIS_H
