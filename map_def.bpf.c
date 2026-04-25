#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include "common.h"

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 4096);
    __type(key, __u32);
    __type(value, struct uprobe_info);
} pid_uprobe_map SEC(".maps");

SEC("tracepoint/syscalls/sys_enter")
int dummy_prog(void *ctx) {
    return 0;
}

char _license[] SEC("license") = "GPL";