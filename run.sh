#!/bin/bash

export migration_point_main=0x914
export migration_point_A=0x996
export translate_point_begin_B=0x998
export translate_point_end_C=0x9a2
export translate_point_begin_D=0x980

gcc -g -march=rv64gcv -mabi=lp64d -O3 -rdynamic demo_outside.S -o demo_outside  -Wl,-Bstatic -lopenblas -Wl,-Bdynamic -lpthread -lm # 生成demo_outside

objdump -d demo_outside > demo_outside_dump.s

python3 translate.py $translate_point_begin_B $translate_point_end_C demo_outside_dump.s  translated_function_1 translate_part1.so  # 生成translate.so

python3 translate.py $translate_point_begin_D $translate_point_end_C demo_outside_dump.s translated_function_2 translate_part2.so # 生成translate.so

echo $$ > /proc/set_ai_thread
gcc -O0 -D_GNU_SOURCE -march=rv64gcv -shared -fPIC -o inject_lib.so inject_lib.c -O2 -nostartfiles -g -lpthread # 生成inject_lib.so

# # ##  LD_LIBRARY_PATH让动态链接器能找到hello_add_translate.so
LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH   LD_PRELOAD=./inject_lib.so  ./demo_outside # 运行