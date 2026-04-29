#!/bin/bash

# # --> 旧代码的实现
# echo $$ > /proc/set_ai_thread

# export vector_snippet_ranges="0x994,0x9a0 0x99c,0x9a0  0x9aa,0x9b2 0x9b4,0x9bc 0x9c0,0x9d8" # while 版本
# # export vector_snippet_ranges="0x978,0x984 0x98e,0x996 0x998,0x9a0 0x9a4,0x9bc" # 无while版本 | 要翻译的代码不能有读写s0寄存器的标量代码，所以不能按vsetvl分割原代码

# gcc -g -march=rv64gcv -mabi=lp64d -O3 -rdynamic demo_outside_while.S -o demo_outside  -Wl,-Bstatic -Wl,-Bdynamic -lpthread -lm # 生成demo_outside | while版本
# # gcc -g -march=rv64gcv -mabi=lp64d -O3 -rdynamic demo_outside.S -o demo_outside  -Wl,-Bstatic -Wl,-Bdynamic -lpthread -lm # 生成demo_outside | 非while版本

# objdump -d demo_outside > demo_outside_dump.s

# python3 translator.py demo_outside_dump.s translated_lib.so

# g++ -D_GNU_SOURCE -march=rv64gcv -shared -fPIC -o inject_lib.so inject_lib.cpp -ldl # 生成inject_lib.so

# gcc -shared -fPIC -o libdata.so data.S

# LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH   LD_PRELOAD=./inject_lib.so  ./demo_outside # 运行

# # <-- 旧代码的实现


# --> 新代码的实现
# echo $$ > /proc/set_ai_thread # 启用1024bit向量 | 但是内核已提前启用？

# # 这部分不要，内核已实现
# bpftool btf dump file <path_to_vmlinux> format c > vmlinux.h
# clang -target bpf -D__TARGET_ARCH_RISCV -I/usr/include -O2 -g -c map_def.bpf.c -o map_def.bpf.o
# sudo bpftool prog load map_def.bpf.o /sys/fs/bpf/uprobe_prog pinmaps /sys/fs/bpf/

export vector_snippet_ranges="0xbf6,0xc02 0xbfe,0xc02 0xc0c,0xc14 0xc16,0xc1e 0xc22,0xc3a" # while 版本

gcc -g -march=rv64gcv -mabi=lp64d -O3 -rdynamic demo_outside_while_new.S -o demo_outside  -Wl,-Bstatic -Wl,-Bdynamic -lpthread -lm # 生成demo_outside | while版本
# gcc -g -march=rv64gcv -mabi=lp64d -O3 -rdynamic demo_outside_new.S -o demo_outside  -Wl,-Bstatic -Wl,-Bdynamic -lpthread -lm # 生成demo_outside | 非while版本

objdump -d demo_outside > demo_outside_dump.s

python3 translator.py demo_outside_dump.s translated_lib.so

make

gcc -shared -fPIC -o libdata.so data.S

LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH   LD_PRELOAD=./inject_lib.so  ./demo_outside # 运行

# <-- 新代码的实现
