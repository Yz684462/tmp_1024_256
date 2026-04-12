# RISC-V 向量迁移框架

一个用于 RISC-V 向量寄存器迁移和动态翻译的综合 C++ 框架，支持多线程。

## 概述

本框架提供 RISC-V 向量指令的动态二进制翻译功能，包括：
- **二进制分析**：控制流图构建和函数边界检测
- **向量寄存器分析**：死寄存器消除和生命周期跟踪
- **多线程支持**：线程特定上下文管理和翻译缓存
- **动态翻译**：运行时汇编翻译和编译

## 架构

```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   补丁模块   │    │    地址模块    │    │  处理模块    │
│                │    │                │    │                │
│ • ebreak       │    │ • 二进制       │    │ • 迁移        │
│   处理程序      │    │   分析         │    │   处理程序      │
│                │    │ • CFG          │    │                │
│ • 信号         │    │ • 向量         │    │ • 翻译        │
│   补丁         │    │   寄存器       │    │   处理程序      │
│                │    │   分析         │    │                │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

## 组件

### 1. 补丁模块 (`patch.h`, `patch.cpp`)
**信号处理和代码补丁系统**

```cpp
class Patch {
public:
    static void patch(uint64_t addr);                    // 插入 ebreak 指令
    static void ebreak_handler(int sig, siginfo_t *info, void *ucontext);
};
```

**功能特性：**
- **信号拦截**：捕获 SIGSEGV 进行动态补丁
- **代码注入**：用 `ebreak` 替换目标指令
- **上下文保存**：在信号处理期间维护 CPU 状态
- **分发逻辑**：路由到迁移或翻译处理器

### 2. 地址模块 (`addr.h`, `addr.cpp`, `addr_*.cpp`)
**二进制分析和向量寄存器生命周期管理**

#### 核心分析 (`addr_init.cpp/h`)
```cpp
namespace AddrCore {
    RCore* init_r_core();                    // 初始化 Radare2 分析
    RAnalFunction* find_func(uint64_t addr, RCore *core);
}
```

#### 向量寄存器分析 (`addr_analysis.cpp/h`)
```cpp
namespace AddrAnalysis {
    std::vector<std::pair<uint64_t, uint64_t>> analyze_vector_register(RAnalFunction *func);
    // New algorithm functions
    void init_sources_insts(RAnalFunction *func, std::vector<Source*>& sources, std::vector<VectorInst*>& insts);
    void tag_sources(RAnalFunction *func, std::vector<Source*>& sources, std::vector<VectorInst*>& insts);
    void judge_sources(std::vector<Source*>& sources, std::vector<VectorInst*>& insts);
    std::vector<std::pair<uint64_t, uint64_t>> get_ranges(std::vector<Source*>& sources, std::vector<VectorInst*>& insts);
}
```

**数据结构：**
```cpp
struct Instruction {
    uint64_t addr;
    std::string mnemonic;
    std::vector<std::string> operands;
};

struct CFGBlock {
    uint64_t addr;
    uint64_t size;
    std::vector<Instruction> instructions;
    std::vector<uint64_t> successors;
    std::vector<uint64_t> predecessors;  // 用于反向分析
};

struct CFG {
    std::vector<CFGBlock> blocks;
    uint64_t entry_block_addr;
};
```

**分析算法：**
1. **初始化**：所有向量寄存器标记为 1024 位有效
2. **正向扫描**：顺序指令分析
3. **赋值检测**：识别向量寄存器赋值
4. **依赖跟踪**：构建修改队列
5. **反向分析**：带循环检测的安全遍历
6. **范围生成**：将模拟点转换为翻译范围

**⚠️ 开发状态**：向量寄存器分析基于规范 AI 生成，需要：
- **算法验证**：验证死寄存器消除正确性
- **边界情况测试**：处理复杂控制流模式
- **性能优化**：优化大函数处理
- **集成测试**：使用实际 RISC-V 二进制文件验证

### 3. 处理模块 (`handle.h`, `handle.cpp`)
**运行时迁移和翻译协调**

```cpp
class Handle {
public:
    static void migration_handle(ucontext_t *uc);    // 处理迁移事件
    static void translation_handle(ucontext_t *uc, uintptr_t fault_pc);  // 处理翻译
    static void make_translations(int thread_index);  // 生成翻译
};
```

### 4. 向量上下文模块 (`vector_context.h`, `vector_context.cpp`)
**多线程向量状态管理**

```cpp
class VectorContext {
public:
    static void initialize_vector_context_pool();
    static void save_os_vector_context_to_simulated_vector_context(
        struct __riscv_v_ext_state *os_vector_context, int thread_index);
    static void save_simulated_vector_context_to_os_vector_context(
        struct __riscv_v_ext_state *os_vector_context);
    static int get_or_assign_thread_index();
    static int get_thread_index();
};
```

**多线程功能特性：**
- **连续内存池**：所有线程的 `global_simulated_vector_contexts_pool`
- **线程索引映射**：`std::thread::id` → 索引映射
- **每线程偏移**：`thread_index * VECTOR_CONTEXT_SIZE`
- **线程安全操作**：线程管理的原子操作
- **翻译缓存**：`global_thread_translated_handles` 提高性能

### 5. 配置模块 (`config.h`, `config.cpp`)
**集中配置管理**

```cpp
// 配置常量
extern const char* config_migration_lib_name;
extern const char* config_migration_func_name;
extern const uint64_t config_migration_func_offset;
extern const int MAX_THREADS;
extern const int VECTOR_CONTEXT_SIZE;
```

### 6. 汇编翻译 (`translator.py`)
**基于 Python 的 RISC-V 向量汇编翻译器**

```python
class AssemblyTranslator:
    def __init__(self, thread_index: int)
    def translate_ranges(self, ranges: List[Tuple[int, int]]) -> str
    def compile_to_shared_library(self, asm_content: str) -> str
```

**功能特性：**
- **多范围支持**：处理多个地址范围
- **线程特定偏移**：处理每线程向量上下文
- **汇编修改**：将 `simulated_cpu_state` 替换为 `global_simulated_vector_contexts_pool`
- **立即数范围处理**：为大偏移正确生成 `addi` 指令
- **动态编译**：运行时生成共享库

## 多线程架构

```
线程 0                  线程 1                  线程 N
┌─────────┐            ┌─────────┐            ┌─────────┐
│ 上下文   │            │ 上下文   │            │ 上下文   │
│ 池[0]   │            │ 池[1]   │            │ 池[N]   │
│ 4192B   │            │ 4192B   │            │ 4192B   │
└─────────┘            └─────────┘            └─────────┘
    │                       │                       │
    └───────────────────────┴───────────────────────┘
                global_simulated_vector_contexts_pool
                (连续内存：N × 4192B)
```

**线程管理：**
- **索引分配**：首次访问时自动线程索引分配
- **内存布局**：固定大小上下文的连续内存池
- **偏移计算**：`thread_index * VECTOR_CONTEXT_SIZE`
- **翻译缓存**：每线程编译的翻译库
- **线程安全**：并发访问的原子操作

## 构建和使用

### 依赖项
```bash
# 必需库
sudo apt-get install libradare2-dev  # 二进制分析的 Radare2
sudo apt-get install gcc             # C++ 编译器
sudo apt-get install python3           # Python 翻译器
```

### 编译
```bash
# 构建框架
make

# 或手动编译
g++ -std=c++11 -I./include \
    -o migration_framework \
    src/main.cpp src/patch.cpp src/addr.cpp src/addr_init.cpp \
    src/addr_analysis.cpp src/handle.cpp \
    src/vector_context.cpp src/config.cpp src/globals.cpp \
    -lr_core -lr_anal -ldl
```

### 运行
```bash
# 加载迁移库并开始监控
./migration_framework

# 框架将：
# 1. 加载 libmigration.so
# 2. 补丁迁移函数
# 3. 监控 ebreak 异常
# 4. 根据需要执行动态翻译
```

## 开发状态

### ✅ 已完成功能
- [x] **模块化架构**：清晰的关注点分离
- [x] **多线程支持**：线程特定上下文和缓存
- [x] **二进制分析**：Radare2 集成的 CFG 构建
- [x] **汇编翻译**：支持多线程的基于 Python 的翻译器
- [x] **信号处理**：健壮的异常处理和分发
- [x] **配置管理**：集中配置系统
- [x] **内存管理**：高效的向量上下文池化

### 🚧 开发中
- [ ] **向量寄存器分析**：算法验证和测试
- [ ] **翻译范围生成**：优化范围选择
- [ ] **性能优化**：大函数处理
- [ ] **错误处理**：全面的错误恢复
- [ ] **测试框架**：单元和集成测试

### ⚠️ 已知限制
- **分析复杂性**：当前向量分析需要验证
- **内存开销**：每线程上下文分配（每线程 4KB）
- **编译时间**：动态翻译增加运行时开销
- **调试支持**：翻译代码的调试能力有限

## 文件结构

```
demo_for_llamacpp/
├── include/                    # 头文件
│   ├── addr.h                 # 地址分析接口
│   ├── addr_*.h              # 模块化地址分析头文件
│   ├── handle.h               # 处理器接口
│   ├── vector_context.h       # 向量上下文管理
│   ├── config.h               # 配置常量
│   ├── globals.h              # 全局变量
│   ├── patch.h                # 补丁接口
│   └── asm.h                  # 汇编宏
├── src/                        # 源文件
│   ├── addr.cpp               # 主地址分析
│   ├── addr_*.cpp             # 模块化地址分析
│   ├── handle.cpp             # 处理器实现
│   ├── vector_context.cpp     # 向量上下文管理
│   ├── config.cpp             # 配置
│   ├── globals.cpp            # 全局初始化
│   ├── patch.cpp              # 补丁实现
│   └── main.cpp               # 入口点
├── translator.py               # 汇编翻译器
├── Makefile                   # 构建配置
└── README.md                  # 本文件
```

## 贡献

### 代码标准
- **C++11**：现代 C++ 特性和模式
- **RAII**：适当的资源管理
- **命名空间**：逻辑代码组织
- **错误处理**：全面的错误检查
- **文档**：清晰的函数和类文档

### 测试
```bash
# 运行基本功能测试
make test

# 测试向量寄存器分析
./test_vector_analysis

# 测试多线程
./test_threading
```

## 许可证

本框架专为 RISC-V 向量指令迁移和动态二进制翻译的研究和教育目的而设计。
