.section .data
.align 4
.globl simulated_cpu_state
.type simulated_cpu_state, @object
.size simulated_cpu_state, 4192
simulated_cpu_state:
    .space 4192
    # 或者需要初始值：
    # .byte 0x00, 0x01, 0x02
    # .fill 4189, 1, 0