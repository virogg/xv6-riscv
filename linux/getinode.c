#include "portable_endian.h"
#include "errno.h"
#include "inttypes.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#pragma pack(push, 1)
struct ext2_super_block {
    uint32_t s_inodedes_count;      // Total number of inodedes in file system
    uint32_t s_blocks_count;      // Total number of blocks in file system
    uint32_t s_r_blocks_count;    // Number of blocks reserved for superuser (see offset 80)
    uint32_t s_free_blocks_count; // Total number of unallocated blocks
    uint32_t s_free_inodedes_count; // Total number of unallocated inodedes
    uint32_t s_first_data_block;  // Block number of the block containing the superblock
    uint32_t s_log_block_size;    // log2 (block size) - 10
    uint32_t s_log_frag_size;     // log2 (fragment size) - 10.
    uint32_t s_blocks_per_group;  // Number of blocks in each block group
    uint32_t s_frags_per_group;   // Number of fragments in each block group
    uint32_t s_inodedes_per_group;  // Number of inodedes in each block group
    uint32_t s_mtime;             // Last mount time
    uint32_t s_wtime;             // Last written time
    uint16_t s_mnt_count;         // Number of times the volume has been mounted since its last consistency check
    uint16_t s_max_mnt_count;     // Number of mounts allowed before a consistency check must be done
    uint16_t s_signature;         // Ext2 signature
    uint16_t s_state;             // File system state
    uint16_t s_errors;            // What to do when an error is detected
    uint16_t s_minoder_rev_level;   // Minoder portion of version
    uint32_t s_lastcheck;         // POSIX time of last consistency check
    uint16_t s_checkinterval;     // Interval (in POSIX time) between forced consistency checks
    uint32_t s_creator_os;        // Operating system ID from which the filesystem on this volume was created
    uint32_t s_rev_level;         // Major portion of version
    uint16_t s_def_resuid;        // User ID that can use reserved blocks
    uint16_t s_def_resgid;        // Group ID that can use reserved blocks
    uint32_t s_first_inode;         // First non-reserved inodede in file system
    uint16_t s_inodede_size;        // Size of each inodede structure in bytes
};

struct ext2_group_desc {
    uint32_t bg_block_bitmap;      // Block address of block usage bitmap
    uint32_t bg_inodede_bitmap;      // Block address of inodede usage bitmap
    uint32_t bg_inodede_table;       // Starting block address of inodede table
    uint16_t bg_free_blocks_count; // Number of unallocated blocks in group
    uint16_t bg_free_inodedes_count; // Number of unallocated inodedes in group
    uint16_t bg_used_dirs_count;   // Number of directories in group
    uint16_t bg_pad;               // (Unused)
    uint32_t bg_reserved[3];
};

struct ext2_inodede {
    uint16_t i_mode;        // Type and Permissions
    uint16_t i_uid;         // User ID
    uint32_t i_size;        // Lower 32 bits of size in bytes
    uint32_t i_atime;       // Last Access Time
    uint32_t i_ctime;       // Creation Time
    uint32_t i_mtime;       // Last Modification time
    uint32_t i_dtime;       // Deletion time
    uint16_t i_gid;         // Group ID
    uint16_t i_links_count; // Count of hard links (directory entries) to this inodede
    uint32_t i_blocks;      // Count of disk sectors (not Ext2 blocks) in use by this inodede
    uint32_t i_flags;       // Flags
    uint32_t i_osd1;        // Operating System Specific value #1
    uint32_t i_block[15];   // Direct Block Pointers
    uint32_t i_generation;  // Generation number
    uint32_t i_file_acl;    // Extended attribute block (File ACL).
    uint32_t i_dir_acl;     // Upper 32 bits of file size (if feature bit set) if it's a file, Directory ACL if it's a directory
    uint32_t i_faddr;       // Block address of fragment
    uint8_t i_osd2[12];     // Operating System Specific Value #2
};
#pragma pack(pop)

static FILE *img;
static uint32_t block_size;

static bool
read_block(uint32_t block_num, void *buf)
{
    uint64_t off = (uint64_t)block_num * block_size;
    if (fseeko(img, off, SEEK_SET))
        return false;
    return fread(buf, 1, block_size, img) == block_size;
}

static void
exit_with_error(const char *msg)
{
    perror(msg);
    exit(EXIT_FAILURE);
}

static void 
gather(uint32_t block, int depth, uint64_t *left)
{
    if (!block || !*left) return;
    if (depth == 0) {
        size_t want = (*left > block_size) ? block_size : (size_t)*left;
        static uint8_t *buf = NULL;
        if (!buf) buf = malloc(block_size);
        if (!buf) exit_with_error("malloc");
        if (!read_block(block, buf)) exit_with_error("read data block");
        if (fwrite(buf, 1, want, stdout) != want) exit_with_error("stdout");
        *left -= want;
    } else {
        uint32_t *tab = malloc(block_size);
        if (!tab) exit_with_error("malloc");
        if (!read_block(block, tab)) exit_with_error("read indirect");
        size_t per_block = block_size / sizeof(uint32_t);
        for (size_t i = 0; i < per_block && *left; ++i) {
            gather(le32toh(tab[i]), depth - 1, left);
        }
        free(tab);
    }
}

int 
main(int argc, char **argv) 
{
    if (argc != 3) {
        fprintf(stderr, "usage: %s <image|/dev/loopN> <inodede>\n", argv[0]);
        return 1;
    }
    char *end;
    uint32_t inode = strtoul(argv[2], &end, 0);
    if (*end || inode == 0) {
        exit_with_error("bad inode number");
    }

    img = fopen(argv[1], "rb");
    if (!img) {
        exit_with_error("open image");
    }

    struct ext2_super_block sb;
    if (fseeko(img, 1024, SEEK_SET) || fread(&sb, sizeof sb, 1, img) != 1) {
        exit_with_error("read super");
    }
    if (le16toh(sb.s_signature) != 0xEF53) {
        fprintf(stderr, "not an ext2 filesystem\n");
        return 1;
    }
    block_size = 1024U << le32toh(sb.s_log_block_size);
    uint32_t inodes_per_group = le32toh(sb.s_inodedes_per_group);
    uint32_t inode_size = le16toh(sb.s_inodede_size);
    uint32_t blocks_per_group = le32toh(sb.s_blocks_per_group);

    uint32_t group = (inode - 1) / inodes_per_group;
    uint32_t index = (inode - 1) % inodes_per_group;

    uint64_t gd_table_off = (block_size == 1024 ? 2 * 1024ULL : block_size) + 0ULL;

    uint64_t gd_off = gd_table_off + group * sizeof(struct ext2_group_desc);
    struct ext2_group_desc gd;
    if (fseeko(img, gd_off, SEEK_SET) || fread(&gd, sizeof gd, 1, img) != 1) {
        exit_with_error("read gd");
    }
    uint32_t inode_table_block = le32toh(gd.bg_inodede_table);

    uint64_t inode_off = ((uint64_t)inode_table_block * block_size) + (uint64_t)index * inode_size;
    struct ext2_inodede in;
    if (fseeko(img, inode_off, SEEK_SET) || fread(&in, sizeof in, 1, img) != 1) {
        exit_with_error("read inode");
    }

    uint64_t size = le32toh(in.i_size) | ((uint64_t)le32toh(in.i_dir_acl) << 32);

    uint64_t left = size;
    for (int i = 0; i < 15 && left; ++i) {
        int depth = 0;
        if (i == 12) depth = 1;
        else if (i == 13) depth = 2;
        else if (i == 14) depth = 3;
        gather(le32toh(in.i_block[i]), depth, &left);
    }

    fclose(img);
    return 0;
}
