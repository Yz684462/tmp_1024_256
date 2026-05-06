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

static uintptr_t patch_addrs[1024];
static int patch_addr_cnt = 0;

static __u64 get_main_exe_inode() {
    struct stat st;
    if (stat("/proc/self/exe", &st) == 0)
        return (__u64)st.st_ino;
    return 0;
}

int patch_code_map(uintptr_t addr) {
    if(addr != 0){
        patch_addrs[patch_addr_cnt++] = addr;
        return 0;
    }
    int map_fd = bpf_obj_get(MAP_PIN_PATH);
    if (map_fd < 0) {
        fprintf(stderr, "Failed to get BPF map fd: %s\n", strerror(errno));
        return -1;
    }

    __u32 pid = getpid();
    __u64 inode = get_main_exe_inode();

    struct uprobe_info data = {0};
    data.inode = inode;
    data.is_attached = false;
    data.offset_cnt = patch_addr_cnt;
    for(int i = 0; i < patch_addr_cnt; i++){
        data.offsets[i] = patch_addrs[i];
    }

    ret = bpf_map_update_elem(map_fd, &pid, &data, BPF_ANY);
    if (ret != 0) {
        fprintf(stderr, "Failed to update BPF map: %s\n", strerror(errno));
        close(map_fd);
        return -1;
    }

    printf("[PATCHER] Added offset to BPF map: pid=%d, inode=%lu, total_offsets=%d\n", pid, inode, data.offset_cnt);
    close(map_fd);
    return 0;
}