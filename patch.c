// patch.c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <errno.h>

#include "common.h"
#include "patch.h"

static __u64 get_main_exe_inode() {
    struct stat st;
    if (stat("/proc/self/exe", &st) == 0)
        return (__u64)st.st_ino;
    return 0;
}

int patch_code_map(uintptr_t addr) {
    int map_fd = bpf_obj_get(MAP_PIN_PATH);
    if (map_fd < 0) {
        fprintf(stderr, "Failed to get BPF map fd: %s\n", strerror(errno));
        return -1;
    }

    __u32 pid = getpid();
    __u64 inode = get_main_exe_inode();

    struct uprobe_info data = {0};
    
    // First try to read existing entry
    int ret = bpf_map_lookup_elem(map_fd, &pid, &data);
    if (ret == 0) {
        // Entry exists, check if we can add more offsets
        if (data.offset_cnt >= MAX_UPROBE_OFFSETS) {
            fprintf(stderr, "Maximum offsets (%d) already reached for pid %d\n", MAX_UPROBE_OFFSETS, pid);
            close(map_fd);
            return -1;
        }
        // Add new offset to existing entry
        data.offsets[data.offset_cnt] = addr;
        data.offset_cnt++;
    } else {
        // No existing entry, create new one
        data.inode = inode;
        data.offsets[0] = addr;
        data.offset_cnt = 1;
        data.is_attached = false;
    }

    ret = bpf_map_update_elem(map_fd, &pid, &data, BPF_ANY);
    if (ret != 0) {
        fprintf(stderr, "Failed to update BPF map: %s\n", strerror(errno));
        close(map_fd);
        return -1;
    }

    printf("[PATCHER] Added offset 0x%lx to BPF map: pid=%d, inode=%lu, total_offsets=%d\n", 
           addr, pid, inode, data.offset_cnt);
    close(map_fd);
    return 0;
}