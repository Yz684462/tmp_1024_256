#!/bin/bash

export vector_snippet_ranges="0x980,0x984 0x98e,0x996 0x998,0x9a0"

# # --> 新代码的实现 | 编译并挂载bpf程序，需要修改下面的<path_to_vmlinux>
# bpftool btf dump file <path_to_vmlinux> format c > vmlinux.h
# clang -target bpf -D__TARGET_ARCH_RISCV -I/usr/include -O2 -g -c map_def.bpf.c -o map_def.bpf.o
# sudo bpftool prog load map_def.bpf.o /sys/fs/bpf/uprobe_prog pinmaps /sys/fs/bpf/
# # --< 新代码的实现

# --> 旧代码的实现
gcc -g -march=rv64gcv -mabi=lp64d -O3 -rdynamic demo_outside.S -o demo_outside  -Wl,-Bstatic -Wl,-Bdynamic -lpthread -lm # 生成demo_outside
# <-- 旧代码的实现

# # --> 新代码的实现：编译有调用迁移接口的benchmark
# gcc -g -march=rv64gcv -mabi=lp64d -O3 -rdynamic demo_outside_new.S -o demo_outside  -Wl,-Bstatic -Wl,-Bdynamic -lpthread -lm # 生成demo_outside
# # <-- 新代码的实现


objdump -d demo_outside > demo_outside_dump.s

python3 translator.py demo_outside_dump.s translated_lib.so

# --> 旧代码的实现
g++ -D_GNU_SOURCE -march=rv64gcv -shared -fPIC -o inject_lib.so inject_lib.cpp -g -lpthread -ldl # 生成inject_lib.so
# <-- 旧代码的实现

# # --> 新代码的实现：编译使用bpf map的inject_lib
# make
# # <-- 新代码的实现

#  LD_LIBRARY_PATH让动态链接器能找到translated_lib.so
LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH   LD_PRELOAD=./inject_lib.so  ./demo_outside # 运行
