    .text

    .globl  translated_function_2432
    .type   translated_function_2432, @function
	.extern simulated_cpu_state
translated_function_2432:
    # 保存 RVV 翻译块现场: vsetvli
    addi    sp, sp, -48
    sd      t0, 0(sp)
    sd      t1, 8(sp)
    sd      t2, 16(sp)
    sd      t3, 24(sp)
    sd      t6, 32(sp)
    # 块内重新加载 simulated_cpu_state 基地址到 t6
    la      t6, simulated_cpu_state
    # vsetvli a2,a1,e8,mf2,ta,ma翻译开始: vlmax=64
    li      t0, 197       # t0 = vtype
    li      t3, 4136
    add     t3, t6, t3
    sd      t0, 0(t3)              # CSR.vtype = vtype
    li      t0, 64          # t0 = VLMAX
    mv      t1, a1            # t1 = a1
    mv      t2, t0               # t2 = VLMAX
    bltu    t0, t1, 1f           # if VLMAX < rs1 skip
    mv      t2, t1               # t2 = rs1
1:
    li      t3, 4128
    add     t3, t6, t3
    sd      t2, 0(t3)              # CSR.vl = t2
    mv      a2, t2             # a2 = vl
    # vsetvli a2,a1,e8,mf2,ta,ma翻译结束: vlmax=64
    # 恢复 RVV 翻译块现场: vsetvli
    ld      t0, 0(sp)
    ld      t1, 8(sp)
    ld      t2, 16(sp)
    ld      t3, 24(sp)
    ld      t6, 32(sp)
    addi    sp, sp, 48

    ret

    .size   translated_function_2432, .-translated_function_2432

    .text

    .globl  translated_function_2446
    .type   translated_function_2446, @function
	.extern simulated_cpu_state
translated_function_2446:
    # 保存 RVV 翻译块现场: vle32.v
    addi    sp, sp, -48
    sd      t0, 0(sp)
    sd      t1, 8(sp)
    sd      t2, 16(sp)
    sd      t3, 24(sp)
    sd      t4, 32(sp)
    sd      t6, 40(sp)
    # 块内重新加载 simulated_cpu_state 基地址到 t6
    la      t6, simulated_cpu_state
    # vle32.v: v10 = mem[a4]
    # 从 CSR 加载 vl
    li      t1, 4128
    add     t1, t6, t1
    ld      t0, 0(t1)  # t0 = vl (from CSR)
.L_vle32_v_loop_0:
    beqz    t0, .L_vle32_v_loop_0_end
    addi    t2, t0, -1
    slli    t2, t2, 2           # t2 = i * 4 (32-bit)
    add     t3, a4, t2
    lw      t4, 0(t3)
    li      t1, 1280
    add     t3, t6, t1
    add     t3, t3, t2
    sw      t4, 0(t3)
    addi    t0, t0, -1
    j       .L_vle32_v_loop_0
.L_vle32_v_loop_0_end:
    # 恢复 RVV 翻译块现场: vle32.v
    ld      t0, 0(sp)
    ld      t1, 8(sp)
    ld      t2, 16(sp)
    ld      t3, 24(sp)
    ld      t4, 32(sp)
    ld      t6, 40(sp)
    addi    sp, sp, 48

    # 保存 RVV 翻译块现场: vle32.v
    addi    sp, sp, -48
    sd      t0, 0(sp)
    sd      t1, 8(sp)
    sd      t2, 16(sp)
    sd      t3, 24(sp)
    sd      t4, 32(sp)
    sd      t6, 40(sp)
    # 块内重新加载 simulated_cpu_state 基地址到 t6
    la      t6, simulated_cpu_state
    # vle32.v: v12 = mem[a3]
    # 从 CSR 加载 vl
    li      t1, 4128
    add     t1, t6, t1
    ld      t0, 0(t1)  # t0 = vl (from CSR)
.L_vle32_v_loop_1:
    beqz    t0, .L_vle32_v_loop_1_end
    addi    t2, t0, -1
    slli    t2, t2, 2           # t2 = i * 4 (32-bit)
    add     t3, a3, t2
    lw      t4, 0(t3)
    li      t1, 1536
    add     t3, t6, t1
    add     t3, t3, t2
    sw      t4, 0(t3)
    addi    t0, t0, -1
    j       .L_vle32_v_loop_1
.L_vle32_v_loop_1_end:
    # 恢复 RVV 翻译块现场: vle32.v
    ld      t0, 0(sp)
    ld      t1, 8(sp)
    ld      t2, 16(sp)
    ld      t3, 24(sp)
    ld      t4, 32(sp)
    ld      t6, 40(sp)
    addi    sp, sp, 48

    ret

    .size   translated_function_2446, .-translated_function_2446

    .text

    .globl  translated_function_2456
    .type   translated_function_2456, @function
	.extern simulated_cpu_state
translated_function_2456:
    # 保存 RVV 翻译块现场: vsetvli
    addi    sp, sp, -32
    sd      t0, 0(sp)
    sd      t3, 8(sp)
    sd      t6, 16(sp)
    # 块内重新加载 simulated_cpu_state 基地址到 t6
    la      t6, simulated_cpu_state
    # vsetvli zero,zero,e32,m2,tu,ma翻译开始: vlmax=64
    li      t0, 145       # t0 = vtype
    li      t3, 4136
    add     t3, t6, t3
    sd      t0, 0(t3)              # CSR.vtype = vtype
    # 保持 vl 不变
    # vsetvli zero,zero,e32,m2,tu,ma翻译结束: vlmax=64
    # 恢复 RVV 翻译块现场: vsetvli
    ld      t0, 0(sp)
    ld      t3, 8(sp)
    ld      t6, 16(sp)
    addi    sp, sp, 32

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
    # 块内重新加载 simulated_cpu_state 基地址到 t6
    la      t6, simulated_cpu_state
# vfmacc.vv v8, v12, v10
# Semantic: v8[i] += v12[i] * v10[i]
# SEW = 32-bit, vl 从 CSR 动态读取

    li      t1, 4128
    add     t1, t6, t1
    ld      t0, 0(t1)  # t0 = vl (from CSR)

.L_vfmacc_vv_loop_0:
    beqz    t0, .L_vfmacc_vv_loop_0_end

    # 计算元素索引和偏移
    addi    t2, t0, -1
    slli    t2, t2, 2           # t2 = i * 4 (32-bit)

    # 加载 v10[i] 到 ft0
    li      t1, 1280
    add     t3, t6, t1
    add     t3, t3, t2
    flw     ft0, 0(t3)

    # 加载 v12[i] 到 ft1
    li      t1, 1536
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

    .size   translated_function_2456, .-translated_function_2456

    .text

    .globl  translated_function_2468
    .type   translated_function_2468, @function
	.extern simulated_cpu_state
translated_function_2468:
    # 保存 RVV 翻译块现场: vsetvli
    addi    sp, sp, -32
    sd      t0, 0(sp)
    sd      t2, 8(sp)
    sd      t3, 16(sp)
    sd      t6, 24(sp)
    # 块内重新加载 simulated_cpu_state 基地址到 t6
    la      t6, simulated_cpu_state
    # vsetvli a0,zero,e32,m2,ta,ma翻译开始: vlmax=64
    li      t0, 209       # t0 = vtype
    li      t3, 4136
    add     t3, t6, t3
    sd      t0, 0(t3)              # CSR.vtype = vtype
    li      t2, 64          # t2 = VLMAX
    li      t3, 4128
    add     t3, t6, t3
    sd      t2, 0(t3)              # CSR.vl = VLMAX
    mv      a0, t2             # a0 = vl
    # vsetvli a0,zero,e32,m2,ta,ma翻译结束: vlmax=64
    # 恢复 RVV 翻译块现场: vsetvli
    ld      t0, 0(sp)
    ld      t2, 8(sp)
    ld      t3, 16(sp)
    ld      t6, 24(sp)
    addi    sp, sp, 32

    # 保存 RVV 翻译块现场: vmv.s.x
    addi    sp, sp, -16
    sd      t0, 0(sp)
    sd      t6, 8(sp)
    # 块内重新加载 simulated_cpu_state 基地址到 t6
    la      t6, simulated_cpu_state
    # vmv.s.x: v10[0] = zero
    li      t0, 1280
    add     t0, t6, t0
    sw      zero, 0(t0)
    # 恢复 RVV 翻译块现场: vmv.s.x
    ld      t0, 0(sp)
    ld      t6, 8(sp)
    addi    sp, sp, 16

    ret

    .size   translated_function_2468, .-translated_function_2468

    .text

    .globl  translated_function_2476
    .type   translated_function_2476, @function
	.extern simulated_cpu_state
translated_function_2476:
    # 保存 RVV 翻译块现场: vfredusum.vs
    addi    sp, sp, -64
    sd      t0, 0(sp)
    sd      t1, 8(sp)
    sd      t2, 16(sp)
    sd      t3, 24(sp)
    sd      t6, 32(sp)
    fsw     ft0, 40(sp)
    fsw     ft1, 48(sp)
    # 块内重新加载 simulated_cpu_state 基地址到 t6
    la      t6, simulated_cpu_state
    # vfredusum.vs: v8[0] = v10[0] + Σv8[i]
    # 加载初始值 v10[0]
    li      t3, 1280
    add     t3, t6, t3
    flw     ft0, 0(t3)    # ft0 = vs2[0]
    li      t1, 4128
    add     t1, t6, t1
    ld      t0, 0(t1)  # t0 = vl (from CSR)
.L_vfredusum_loop_0:
    beqz    t0, .L_vfredusum_loop_0_end
    addi    t2, t0, -1
    slli    t2, t2, 2           # t2 = i * 8 (64-bit)
    li      t3, 1024
    add     t3, t6, t3
    add     t3, t3, t2
    flw     ft1, 0(t3)    # ft1 = vs1[i]
    fadd.s  ft0, ft0, ft1    # ft0 += ft1
    addi    t0, t0, -1
    j       .L_vfredusum_loop_0
.L_vfredusum_loop_0_end:
    # 写回结果到 v8[0]
    li      t3, 1024
    add     t3, t6, t3
    fsw     ft0, 0(t3)    # vd[0] = ft0
    # 恢复 RVV 翻译块现场: vfredusum.vs
    flw     ft0, 40(sp)
    flw     ft1, 48(sp)
    ld      t0, 0(sp)
    ld      t1, 8(sp)
    ld      t2, 16(sp)
    ld      t3, 24(sp)
    ld      t6, 32(sp)
    addi    sp, sp, 64

    ret

    .size   translated_function_2476, .-translated_function_2476

    .text

    .globl  translated_function_2480
    .type   translated_function_2480, @function
	.extern simulated_cpu_state
translated_function_2480:
    # 保存 RVV 翻译块现场: vfmv.f.s
    addi    sp, sp, -16
    sd      t0, 0(sp)
    sd      t6, 8(sp)
    # 块内重新加载 simulated_cpu_state 基地址到 t6
    la      t6, simulated_cpu_state
    # vfmv.f.s: fa5 = v8[0]
    # 与 QEMU trans_vfmv_f_s 一致：直接读取源向量第 0 个元素，不依赖 vl
    # SEW=64 时使用 fld，直接装载双精度值
    li      t0, 1024
    add     t0, t6, t0
    flw     fa5, 0(t0)
    # 恢复 RVV 翻译块现场: vfmv.f.s
    ld      t0, 0(sp)
    ld      t6, 8(sp)
    addi    sp, sp, 16

    fcvt.d.s fa5, fa5
    fmv.x.d  a1, fa5
    ret

    .size   translated_function_2480, .-translated_function_2480

