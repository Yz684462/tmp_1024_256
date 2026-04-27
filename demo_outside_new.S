.section .rodata

.L__const.call_migration.proc_path_ai:
        .asciz  "/proc/set_ai_thread"

.L.str:
        .asciz  "w"

.L.str.1:
        .asciz  "\351\224\231\350\257\257: \346\227\240\346\263\225\346\211\223\345\274\200 /proc/set_ai_thread"

.L.str.2:
        .asciz  "%d"

.L.str.3:
        .asciz  "\351\224\231\350\257\257: \345\206\231\345\205\245 PID \345\210\260 /proc/set_ai_thread \345\244\261\350\264\245"

.L.str.4:
        .asciz  "\351\224\231\350\257\257: \345\205\263\351\227\255 /proc/set_ai_thread \346\226\207\344\273\266\345\244\261\350\264\245"

.L.str.5:
        .asciz  "Sum = %.2f\n"

.globl call_migration
.type call_migration, @function

.text
.align 2

call_migration:
        addi    sp, sp, -32
        sd      ra, 24(sp)
        sd      s0, 16(sp)
        sd      s1, 8(sp)
        call    getpid
        mv      s1, a0
.Lpcrel_hi0:
        auipc   a0, %pcrel_hi(.L__const.call_migration.proc_path_ai)
.Lpcrel_hi1:
        auipc   a1, %pcrel_hi(.L.str)
        addi    a0, a0, %pcrel_lo(.Lpcrel_hi0)
        addi    a1, a1, %pcrel_lo(.Lpcrel_hi1)
        call    fopen
        beqz    a0, .LBB0_4
        mv      s0, a0
.Lpcrel_hi3:
        auipc   a1, %pcrel_hi(.L.str.2)
        addi    a1, a1, %pcrel_lo(.Lpcrel_hi3)
        mv      a2, s1
        call    fprintf
        bltz    a0, .LBB0_5
        mv      a0, s0
        call    fclose
        bnez    a0, .LBB0_6
        ld      ra, 24(sp)
        ld      s0, 16(sp)
        ld      s1, 8(sp)
        addi    sp, sp, 32
        ret
.LBB0_4:
.Lpcrel_hi2:
        auipc   a0, %pcrel_hi(.L.str.1)
        addi    a0, a0, %pcrel_lo(.Lpcrel_hi2)
        ld      ra, 24(sp)
        ld      s0, 16(sp)
        ld      s1, 8(sp)
        addi    sp, sp, 32
        tail    perror
.LBB0_5:
.Lpcrel_hi4:
        auipc   a0, %pcrel_hi(.L.str.3)
        addi    a0, a0, %pcrel_lo(.Lpcrel_hi4)
        call    perror
        mv      a0, s0
        ld      ra, 24(sp)
        ld      s0, 16(sp)
        ld      s1, 8(sp)
        addi    sp, sp, 32
        tail    fclose
.LBB0_6:
.Lpcrel_hi5:
        auipc   a0, %pcrel_hi(.L.str.4)
        addi    a0, a0, %pcrel_lo(.Lpcrel_hi5)
        ld      ra, 24(sp)
        ld      s0, 16(sp)
        ld      s1, 8(sp)
        addi    sp, sp, 32
        tail    perror


.globl main
.type main, @function

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
.LBB2_1:
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
        bnez    a1, .LBB2_1
        call    call_migration

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
.Lpcrel_hi6:
        auipc   a0, %pcrel_hi(.L.str.5)
        addi    a0, a0, %pcrel_lo(.Lpcrel_hi6)
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

