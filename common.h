#ifndef COMMON_H
#define COMMON_H

#define MAX_UPROBE_OFFSETS 8

struct uprobe_info {
    __u64 inode;
    __u64 offsets[MAX_UPROBE_OFFSETS];
    int  offset_cnt;
    bool is_attached;
};

#define MAP_PIN_PATH "/sys/fs/bpf/pid_uprobe_map"

#endif