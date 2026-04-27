# demo_outside.S
# 必须声明 main 为全局符号，否则链接器找不到
.globl main
.type main, @function

# 数据段 (如果需要字符串常量)
.section .rodata

.L.str:
        .asciz  "Sum = %.2f\n"

# 代码段
.text
.align 2

main:
        addi    sp, sp, -32
        sd      ra, 24(sp)
        sd      s0, 16(sp)
        sd      s1, 8(sp)
        lui     a0, 1
        call    malloc
        mv      s0, a0
        lui     a0, 1
        call    malloc
        mv      s1, a0
        li      a0, 0
        li      a1, 1024
        vsetvli a2, zero, e64, m4, ta, ma
        vid.v   v8
        lui     a2, 260096
.LBB1_1:
        vsetvli a3, a1, e64, m4, ta, ma
        slli    a4, a0, 2
        vadd.vi v12, v8, 1
        add     a5, s0, a4
        add     a4, a4, s1
        vsetvli zero, zero, e32, m2, ta, ma
        vfncvt.f.xu.w   v16, v12
        vmv.v.x v12, a2
        add     a0, a0, a3
        sub     a1, a1, a3
        vse32.v v12, (a4)
        vse32.v v16, (a5)
        vsetvli zero, zero, e64, m4, ta, ma
        vadd.vx v8, v8, a3
        bnez    a1, .LBB1_1
        
        li      a0, 0
        li      a1, 1024
        vsetvli a2, zero, e32, m2, ta, ma
        vmv.v.i v8, 0
.LBB1_3:
        vsetvli a2, a1, e8, mf2, ta, ma
        slli    a3, a0, 2
        add     a4, s1, a3
        add     a3, a3, s0
        vle32.v v10, (a4)
        vle32.v v12, (a3)
        sub     a1, a1, a2
        vsetvli zero, zero, e32, m2, tu, ma
        vfmacc.vv       v8, v12, v10
        add     a0, a0, a2
        bnez    a1, .LBB1_3
        vsetvli a0, zero, e32, m2, ta, ma
        vmv.s.x v10, zero
        vfredusum.vs    v8, v8, v10
        vfmv.f.s        fa5, v8


        fcvt.d.s        fa5, fa5
        fmv.x.d a1, fa5
.Lpcrel_hi0:
        auipc   a0, %pcrel_hi(.L.str)
        addi    a0, a0, %pcrel_lo(.Lpcrel_hi0)
        call    printf
        mv      a0, s0
        call    free
        mv      a0, s1
        call    free
        li      a0, 0
        ld      ra, 24(sp)
        ld      s0, 16(sp)
        ld      s1, 8(sp)
        addi    sp, sp, 32
        ret
