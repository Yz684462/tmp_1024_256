Translation ID: 1
Dump file: ../test/test_dump_file_for_translator.s
Function names: translation_func_a7964, translation_func_a79e6, translation_func_a7a3c
Output file: translation_lib_1.so
Combined translated assembly written to /tmp/tmpodd9xtjr.s
Successfully compiled translation_lib_1.so

Translation completed successfully:     .text

    .globl  translation_func_a7964
    .type   translation_func_a7964, @function
	.extern global_simulated_vector_contexts_pool
translation_func_a7964:
    # 保存 RVV 翻译块现场: vsetvli
    addi    sp, sp, -48
    sd      t0, 0(sp)
    sd      t1, 8(sp)
    sd      t2, 16(sp)
    sd      t3, 24(sp)
    sd      t6, 32(sp)
    # 块内重新加载 global_simulated_vector_contexts_pool 基地址到 t6
    la      t6, global_simulated_vector_contexts_pool
	li	t0, 4192
	add	t6, t6, t0
    # vsetvli zero,a3,e32,m8,tu,ma翻译开始: vlmax=256
    li      t0, 147       # t0 = vtype
    li      t3, 4136
    add     t3, t6, t3
    sd      t0, 0(t3)              # CSR.vtype = vtype
    li      t0, 256          # t0 = VLMAX
    mv      t1, a3            # t1 = a3
    mv      t2, t0               # t2 = VLMAX
    bltu    t0, t1, 1f           # if VLMAX < rs1 skip
    mv      t2, t1               # t2 = rs1
1:
    li      t3, 4128
    add     t3, t6, t3
    sd      t2, 0(t3)              # CSR.vl = t2
    # vsetvli zero,a3,e32,m8,tu,ma翻译结束: vlmax=256
    # 恢复 RVV 翻译块现场: vsetvli
    ld      t0, 0(sp)
    ld      t1, 8(sp)
    ld      t2, 16(sp)
    ld      t3, 24(sp)
    ld      t6, 32(sp)
    addi    sp, sp, 48

    # 保存 RVV 翻译块现场: vfmacc.vv
    addi    sp, sp, -64
    sd      t0, 0(sp)
    sd      t1, 8(sp)
    sd      t2, 16(sp)
    sd      t3, 24(sp)
    sd      t6, 32(sp)
    fsd     ft0, 40(sp)
    fsd     ft1, 48(sp)
    fsd     ft2, 56(sp)
    # 块内重新加载 global_simulated_vector_contexts_pool 基地址到 t6
    la      t6, global_simulated_vector_contexts_pool
	li	t0, 4192
	add	t6, t6, t0
# vfmacc.vv v8, v24, v16
# Semantic: v8[i] += v24[i] * v16[i]
# SEW = 32-bit, vl 从 CSR 动态读取

    li      t1, 4128
    add     t1, t6, t1
    ld      t0, 0(t1)  # t0 = vl (from CSR)

.L_vfmacc_vv_loop_0:
    beqz    t0, .L_vfmacc_vv_loop_0_end

    # 计算元素索引和偏移
    addi    t2, t0, -1
    slli    t2, t2, 2           # t2 = i * 4 (32-bit)

    # 加载 v16[i] 到 ft0
    li      t1, 2048
    add     t3, t6, t1
    add     t3, t3, t2
    flw     ft0, 0(t3)

    # 加载 v24[i] 到 ft1
    li      t1, 3072
    add     t3, t6, t1
    add     t3, t3, t2
    flw     ft1, 0(t3)

    # 加载 v8[i] 到 ft2
    li      t1, 1024
    add     t3, t6, t1
    add     t3, t3, t2
    flw     ft2, 0(t3)

    fmadd.s ft2, ft0, ft1, ft2  # ft2 = ft0 * ft1 + ft2

    li      t1, 1024
    add     t3, t6, t1
    add     t3, t3, t2
    fsw     ft2, 0(t3)

    addi    t0, t0, -1
    j       .L_vfmacc_vv_loop_0

.L_vfmacc_vv_loop_0_end:
    # 恢复 RVV 翻译块现场: vfmacc.vv
    fld     ft0, 40(sp)
    fld     ft1, 48(sp)
    fld     ft2, 56(sp)
    ld      t0, 0(sp)
    ld      t1, 8(sp)
    ld      t2, 16(sp)
    ld      t3, 24(sp)
    ld      t6, 32(sp)
    addi    sp, sp, 64

    ret

    .size   translation_func_a7964, .-translation_func_a7964


    .text

    .globl  translation_func_a79e6
    .type   translation_func_a79e6, @function
	.extern global_simulated_vector_contexts_pool
translation_func_a79e6:
    # 保存 RVV 翻译块现场: vsetvli
    addi    sp, sp, -32
    sd      t0, 0(sp)
    sd      t2, 8(sp)
    sd      t3, 16(sp)
    sd      t6, 24(sp)
    # 块内重新加载 global_simulated_vector_contexts_pool 基地址到 t6
    la      t6, global_simulated_vector_contexts_pool
	li	t0, 4192
	add	t6, t6, t0
    # vsetvli a4,zero,e8,m2,ta,ma翻译开始: vlmax=256
    li      t0, 193       # t0 = vtype
    li      t3, 4136
    add     t3, t6, t3
    sd      t0, 0(t3)              # CSR.vtype = vtype
    li      t2, 256          # t2 = VLMAX
    li      t3, 4128
    add     t3, t6, t3
    sd      t2, 0(t3)              # CSR.vl = VLMAX
    mv      a4, t2             # a4 = vl
    # vsetvli a4,zero,e8,m2,ta,ma翻译结束: vlmax=256
    # 恢复 RVV 翻译块现场: vsetvli
    ld      t0, 0(sp)
    ld      t2, 8(sp)
    ld      t3, 16(sp)
    ld      t6, 24(sp)
    addi    sp, sp, 32

    ret

    .size   translation_func_a79e6, .-translation_func_a79e6


    .text

    .globl  translation_func_a7a3c
    .type   translation_func_a7a3c, @function
	.extern global_simulated_vector_contexts_pool
translation_func_a7a3c:
    # 保存 RVV 翻译块现场: vsetvli
    addi    sp, sp, -48
    sd      t0, 0(sp)
    sd      t1, 8(sp)
    sd      t2, 16(sp)
    sd      t3, 24(sp)
    sd      t6, 32(sp)
    # 块内重新加载 global_simulated_vector_contexts_pool 基地址到 t6
    la      t6, global_simulated_vector_contexts_pool
	li	t0, 4192
	add	t6, t6, t0
    # vsetvli zero,a3,e32,m8,ta,ma翻译开始: vlmax=256
    li      t0, 211       # t0 = vtype
    li      t3, 4136
    add     t3, t6, t3
    sd      t0, 0(t3)              # CSR.vtype = vtype
    li      t0, 256          # t0 = VLMAX
    mv      t1, a3            # t1 = a3
    mv      t2, t0               # t2 = VLMAX
    bltu    t0, t1, 1f           # if VLMAX < rs1 skip
    mv      t2, t1               # t2 = rs1
1:
    li      t3, 4128
    add     t3, t6, t3
    sd      t2, 0(t3)              # CSR.vl = t2
    # vsetvli zero,a3,e32,m8,ta,ma翻译结束: vlmax=256
    # 恢复 RVV 翻译块现场: vsetvli
    ld      t0, 0(sp)
    ld      t1, 8(sp)
    ld      t2, 16(sp)
    ld      t3, 24(sp)
    ld      t6, 32(sp)
    addi    sp, sp, 48

    # 保存 RVV 翻译块现场: vfredusum.vs
    addi    sp, sp, -64
    sd      t0, 0(sp)
    sd      t1, 8(sp)
    sd      t2, 16(sp)
    sd      t3, 24(sp)
    sd      t6, 32(sp)
    fsd     ft0, 40(sp)
    fsd     ft1, 48(sp)
    # 块内重新加载 global_simulated_vector_contexts_pool 基地址到 t6
    la      t6, global_simulated_vector_contexts_pool
	li	t0, 4192
	add	t6, t6, t0
    # vfredusum.vs: v1[0] = v8[0] + Σv1[i]
    # 加载初始值 v8[0]
    li      t3, 1024
    add     t3, t6, t3
    flw     ft0, 0(t3)    # ft0 = vs1[0]
    li      t1, 4128
    add     t1, t6, t1
    ld      t0, 0(t1)  # t0 = vl (from CSR)
.L_vfredusum_loop_0:
    beqz    t0, .L_vfredusum_loop_0_end
    addi    t2, t0, -1
    slli    t2, t2, 2           # t2 = i * 4 (32-bit)
    li      t3, 128
    add     t3, t6, t3
    add     t3, t3, t2
    flw     ft1, 0(t3)    # ft1 = vs2[i]
    fadd.s  ft0, ft0, ft1    # ft0 += ft1
    addi    t0, t0, -1
    j       .L_vfredusum_loop_0
.L_vfredusum_loop_0_end:
    # 写回结果到 v1[0]
    li      t3, 128
    add     t3, t6, t3
    fsw     ft0, 0(t3)    # vd[0] = ft0
    # 恢复 RVV 翻译块现场: vfredusum.vs
    fld     ft0, 40(sp)
    fld     ft1, 48(sp)
    ld      t0, 0(sp)
    ld      t1, 8(sp)
    ld      t2, 16(sp)
    ld      t3, 24(sp)
    ld      t6, 32(sp)
    addi    sp, sp, 64

    ret

    .size   translation_func_a7a3c, .-translation_func_a7a3c

