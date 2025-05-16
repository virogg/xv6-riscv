#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <byteswap.h>

#define EXT2_SUPERBLOCK_OFFSET 1024
#define EXT2_SIGNATURE 0xEF53
#define EXT2_DIRECT_BLOCKS 12
#define EXT2_SINGLY_INDIRECT_BLOCK 12
#define EXT2_DOUBLY_INDIRECT_BLOCK 13
#define EXT2_TRIPLY_INDIRECT_BLOCK 14
#define EXT2_ZEROS_BUF_SIZE 8192

// Check endianess
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define le16_to_cpu(x) (x)
#define le32_to_cpu(x) (x)
#define cpu_to_le16(x) (x)
#define cpu_to_le32(x) (x)
#else
#define le16_to_cpu(x) bswap_16(x)
#define le32_to_cpu(x) bswap_32(x)
#define cpu_to_le16(x) bswap_16(x)
#define cpu_to_le32(x) bswap_32(x)
#endif

#pragma pack(push, 1)
struct ext2_superblock {
    uint32_t s_inodes_count;         // Total number of inodes in file system
    uint32_t s_blocks_count;         // Total number of blocks in file system
    uint32_t s_r_blocks_count;       // Number of blocks reserved for superuser
    uint32_t s_free_blocks_count;    // Total number of unallocated blocks
    uint32_t s_free_inodes_count;    // Total number of unallocated inodes
    uint32_t s_first_data_block;     // Block number of the block containing the superblock (also the starting block number, NOT always zero.)
    uint32_t s_log_block_size;       // The number to shift 1,024 to the left by to obtain the block size
    uint32_t s_log_frag_size;        // The number to shift 1,024 to the left by to obtain the fragment size
    uint32_t s_blocks_per_group;     // Number of blocks in each block group
    uint32_t s_frags_per_group;      // Number of fragments in each block group
    uint32_t s_inodes_per_group;     // Number of inodes in each block group
    uint32_t s_mtime;                // Last mount time
    uint32_t s_wtime;                // Last written time
    uint16_t s_mnt_count;            // Number of times the volume has been mounted since its last consistency check
    uint16_t s_max_mnt_count;        // Number of mounts allowed before a consistency check must be done
    uint16_t s_magic;                // Ext2 signature (0xef53)
    uint16_t s_state;                // File system state
    uint16_t s_errors;               // What to do when an error is detected
    uint16_t s_minor_rev_level;      // Minor portion of version
    uint32_t s_lastcheck;            // Time of last consistency check
    uint32_t s_checkinterval;        // Interval between forced consistency checks
    uint32_t s_creator_os;           // Operating system ID from which the filesystem on this volume was created
    uint32_t s_rev_level;            // Major portion of version
    uint16_t s_def_resuid;           // User ID that can use reserved blocks
    uint16_t s_def_resgid;           // Group ID that can use reserved blocks

    uint32_t s_first_ino;            // First non-reserved inode in file system
    uint16_t s_inode_size;           // Size of each inode structure in bytes
    uint16_t s_block_group_nr;       // Block group that this superblock is part of
    uint32_t s_feature_compat;       // Optional features present
    uint32_t s_feature_incompat;     // Required features present
    uint32_t s_feature_ro_compat;    // Features that if not supported, the volume must be mounted read-only
    uint8_t s_uuid[16];              // File system ID
    char s_volume_name[16];          // Volume name
    char s_last_mounted[64];         // Path volume was last mounted to
    uint32_t s_algorithm_usage_bitmap; // Compression algorithms used

    uint8_t s_prealloc_blocks;      // Number of blocks to preallocate for files
    uint8_t s_prealloc_dir_blocks;  // Number of blocks to preallocate for directories
    uint16_t s_padding1;            // (Unused)

    uint8_t s_journal_uuid[16];     // Journal ID
    uint32_t s_journal_inum;        // Journal inode
    uint32_t s_journal_dev;         // 	Journal device
    uint32_t s_last_orphan;         // Head of orphan inode list
    uint32_t s_hash_seed[4];
    uint8_t s_def_hash_version;
    uint8_t s_reserved_char_pad;
    uint16_t s_reserved_word_pad;
    uint32_t s_default_mount_opts;
    uint32_t s_first_meta_bg;
    uint32_t s_reserved[190];
};

struct ext2_group_desc {
    uint32_t bg_block_bitmap;        // Block address of block usage bitmap
    uint32_t bg_inode_bitmap;        // Block address of inode usage bitmap
    uint32_t bg_inode_table;         // Starting block address of inode table
    uint16_t bg_free_blocks_count;   // Number of unallocated blocks in group
    uint16_t bg_free_inodes_count;   // Number of unallocated inodes in group
    uint16_t bg_used_dirs_count;     // Number of directories in group
    uint16_t bg_pad;                 // (Unused)
    uint32_t bg_reserved[3];         // (Unused)
};

struct ext2_inode {
    uint16_t i_mode;                 // Type and Permissions
    uint16_t i_uid;                  // User ID
    uint32_t i_size;                 // Lower 32 bits of size in bytes
    uint32_t i_atime;                // Last Access Time
    uint32_t i_ctime;                // Creation Time
    uint32_t i_mtime;                // Last Modification time
    uint32_t i_dtime;                // Deletion time
    uint16_t i_gid;                  // Group ID
    uint16_t i_links_count;          // Count of hard links to this inode
    uint32_t i_blocks;               // Count of disk sectors (not Ext2 blocks) in use by this inode
    uint32_t i_flags;                // Flags
    uint32_t i_osd1;                 // Operating System Specific value #1
    uint32_t i_block[15];            // Direct Block Pointers
    uint32_t i_generation;           // Generation number
    uint32_t i_file_acl;             // Extended attribute block (File ACL)
    uint32_t i_dir_acl;              // Upper 32 bits of file size (if feature bit set) if it's a file, Directory ACL if it's a directory
    uint32_t i_faddr;                // Block address of fragment
    uint8_t i_osd2[12];              // Operating System Specific Value #2
};
#pragma pack(pop)

FILE *fs_file;
uint32_t block_size;
struct ext2_superblock superblock;
struct ext2_group_desc *group_desc_table;

void* read_block(uint32_t block_num, void *buffer);
void read_inode(uint32_t inode_num, struct ext2_inode *inode_buf); // Изменена сигнатура
void output_zeros(uint64_t size);
uint64_t write_block_data(uint32_t block_num, uint64_t remaining_size);
void process_indirect_blocks(uint32_t block_num, int level, uint64_t *remaining_size);
uint64_t process_direct_blocks(struct ext2_inode *inode, uint64_t file_size, uint32_t blocks_to_process);
uint64_t process_singly_indirect(uint32_t singly_block_num, uint64_t file_size, uint64_t bytes_already_written, uint64_t singly_max_bytes, uint32_t entries_per_block);
uint64_t process_doubly_indirect(uint32_t doubly_block_num, uint64_t file_size, uint64_t bytes_already_written, uint64_t doubly_max_bytes, uint32_t entries_per_block, uint64_t remaining_blocks);
void extract_inode_data(uint32_t inode_num);

void* read_block(uint32_t block_num, void *buffer) {
    if (!buffer) {
        buffer = malloc(block_size);
        if (!buffer) {
            perror("Memory buffer allocation error");
            exit(EXIT_FAILURE);
        }
    }

    if (fseek(fs_file, block_num * block_size, SEEK_SET) != 0) {
        perror("Error seeking to block");
        free(buffer);
        exit(EXIT_FAILURE);
    }

    if (fread(buffer, block_size, 1, fs_file) != 1) {
        if (feof(fs_file)) {
            fprintf(stderr, "End of file reached while reading block %u\n", block_num);
        } else {
            perror("Error reading block");
        }
        free(buffer);
        exit(EXIT_FAILURE);
    }

    return buffer;
}

void read_inode(uint32_t inode_num, struct ext2_inode *inode_buf) {
    if (!inode_buf) {
        fprintf(stderr, "Error: inode_buf cannot be NULL\n");
        exit(EXIT_FAILURE);
    }

    if (inode_num == 0 || inode_num > le32_to_cpu(superblock.s_inodes_count)) {
        fprintf(stderr, "Wrong inode: %u\n", inode_num);
        exit(EXIT_FAILURE);
    }

    uint32_t inodes_per_group = le32_to_cpu(superblock.s_inodes_per_group);
    uint32_t group = (inode_num - 1) / inodes_per_group;
    uint32_t index = (inode_num - 1) % inodes_per_group;
    uint32_t inode_size = le16_to_cpu(superblock.s_inode_size);

    if (inode_size == 0) {
        inode_size = 128;
    }

    uint32_t groups_count = (le32_to_cpu(superblock.s_blocks_count) -
                             le32_to_cpu(superblock.s_first_data_block) +
                             le32_to_cpu(superblock.s_blocks_per_group) - 1) /
                            le32_to_cpu(superblock.s_blocks_per_group);

    if (group >= groups_count) {
        fprintf(stderr, "Inode number out of range: %u\n", inode_num);
        exit(EXIT_FAILURE);
    }

    uint32_t inode_table_block = le32_to_cpu(group_desc_table[group].bg_inode_table);
    uint32_t offset = index * inode_size;
    uint32_t inode_block = inode_table_block + (offset / block_size);
    uint32_t inode_offset = offset % block_size;

    void *block_buffer = malloc(block_size);
    if (!block_buffer) {
        perror("Memory block buffer allocation error");
        exit(EXIT_FAILURE);
    }

    read_block(inode_block, block_buffer);

    memcpy(inode_buf, (char *) block_buffer + inode_offset, sizeof(struct ext2_inode));
    free(block_buffer);
}

void output_zeros(uint64_t size) {
    static char zeros[EXT2_ZEROS_BUF_SIZE] = {0};

    uint64_t remaining = size;
    while (remaining > 0) {
        uint64_t chunk = (remaining > sizeof(zeros)) ? sizeof(zeros) : remaining;
        if (fwrite(zeros, 1, chunk, stdout) != chunk) {
            perror("fwrite");
            exit(EXIT_FAILURE);
        }
        remaining -= chunk;
    }
}

uint64_t write_block_data(uint32_t block_num, uint64_t remaining_size) {
    uint64_t bytes_to_write = (remaining_size < block_size) ? remaining_size : block_size;

    if (block_num == 0) {
        // hole
        output_zeros(bytes_to_write);
    } else {
        void *buffer = malloc(block_size);
        if (!buffer) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        read_block(block_num, buffer);
        if (fwrite(buffer, 1, bytes_to_write, stdout) != bytes_to_write) {
            perror("fwrite");
            exit(EXIT_FAILURE);
        }

        free(buffer);
    }

    return bytes_to_write;
}

void process_indirect_blocks(uint32_t block_num, int level, uint64_t *remaining_size) {
    if (*remaining_size == 0) return;

    // hole
    if (block_num == 0) {
        uint32_t entries = block_size / sizeof(uint32_t);
        uint64_t blocks_in_level = 1;

        for (int i = 1; i < level; i++) {
            blocks_in_level *= entries;
        }

        for (uint64_t i = 0; i < blocks_in_level && *remaining_size > 0; i++) {
            uint64_t bytes_to_write = (*remaining_size > block_size) ? block_size : *remaining_size;
            output_zeros(bytes_to_write);
            *remaining_size -= bytes_to_write;
        }

        return;
    }

    uint32_t entries = block_size / sizeof(uint32_t);
    uint32_t *tbl = malloc(block_size);
    if (!tbl) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }
    read_block(block_num, tbl);

    for (uint32_t i = 0; i < entries && *remaining_size > 0; i++) {
        uint32_t entry = le32_to_cpu(tbl[i]);
        if (level > 1) {
            process_indirect_blocks(entry, level - 1, remaining_size);
        } else {
            *remaining_size -= write_block_data(entry, *remaining_size);
        }
    }
    free(tbl);
}

uint64_t process_direct_blocks(struct ext2_inode *inode, uint64_t file_size, uint32_t blocks_to_process) {
    uint64_t bytes_written = 0;

    for (uint32_t i = 0; i < blocks_to_process; i++) {
        uint32_t block_num = le32_to_cpu(inode->i_block[i]);
        uint64_t remaining = file_size - bytes_written;
        if (remaining == 0) break;

        bytes_written += write_block_data(block_num, remaining);
    }

    return bytes_written;
}

uint64_t process_singly_indirect(uint32_t singly_block_num, uint64_t file_size, uint64_t bytes_already_written,
                                 uint64_t singly_max_bytes, uint32_t entries_per_block) {
    uint64_t bytes_written = 0;

    if (singly_block_num == 0) {
        uint64_t hole_size = (file_size < singly_max_bytes) ?
                             (file_size - bytes_already_written) :
                             (singly_max_bytes - bytes_already_written);

        output_zeros(hole_size);
        bytes_written = hole_size;
    } else {
        uint32_t *block_pointers = malloc(block_size);
        if (!block_pointers) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        read_block(singly_block_num, block_pointers);

        uint64_t remaining_file_size = file_size - bytes_already_written;
        uint32_t blocks_needed = (remaining_file_size + block_size - 1) / block_size;
        uint32_t blocks_to_process = (blocks_needed < entries_per_block) ? blocks_needed : entries_per_block;

        for (uint32_t i = 0; i < blocks_to_process; i++) {
            uint32_t block_num = le32_to_cpu(block_pointers[i]);
            uint64_t remaining = file_size - bytes_already_written - bytes_written;
            if (remaining == 0) break;

            bytes_written += write_block_data(block_num, remaining);
        }

        free(block_pointers);
    }

    return bytes_written;
}

uint64_t process_doubly_indirect(uint32_t doubly_block_num, uint64_t file_size, uint64_t bytes_already_written,
                                 uint64_t doubly_max_bytes, uint32_t entries_per_block, uint64_t remaining_blocks) {
    uint64_t bytes_written = 0;

    if (doubly_block_num == 0) {
        uint64_t hole_size = (file_size < doubly_max_bytes) ?
                             (file_size - bytes_already_written) :
                             (doubly_max_bytes - bytes_already_written);

        output_zeros(hole_size);
        bytes_written = hole_size;
    } else {
        uint32_t *indirect_pointers = malloc(block_size);
        if (!indirect_pointers) {
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        read_block(doubly_block_num, indirect_pointers);

        uint32_t indirect_blocks_needed = (remaining_blocks + entries_per_block - 1) / entries_per_block;

        for (uint32_t i = 0; i < indirect_blocks_needed; i++) {
            uint32_t indirect_block_num = le32_to_cpu(indirect_pointers[i]);
            uint64_t current_bytes_written = bytes_already_written + bytes_written;

            if (current_bytes_written >= file_size) break;

            if (indirect_block_num == 0) {
                uint64_t block_indexes_in_this_indirect = (i == indirect_blocks_needed - 1) ?
                                                          (remaining_blocks - (indirect_blocks_needed - 1) * entries_per_block) :
                                                          entries_per_block;

                uint64_t hole_size = block_indexes_in_this_indirect * block_size;
                if (current_bytes_written + hole_size > file_size) {
                    hole_size = file_size - current_bytes_written;
                }

                output_zeros(hole_size);
                bytes_written += hole_size;
            } else {
                uint32_t *block_pointers = malloc(block_size);
                if (!block_pointers) {
                    perror("malloc");
                    exit(EXIT_FAILURE);
                }

                read_block(indirect_block_num, block_pointers);

                uint32_t data_blocks_in_this_indirect;
                if (i == indirect_blocks_needed - 1) {
                    data_blocks_in_this_indirect = remaining_blocks - (indirect_blocks_needed - 1) * entries_per_block;
                } else {
                    data_blocks_in_this_indirect = entries_per_block;
                }

                for (uint32_t j = 0; j < data_blocks_in_this_indirect; j++) {
                    uint32_t block_num = le32_to_cpu(block_pointers[j]);
                    uint64_t current_bytes = bytes_already_written + bytes_written;
                    uint64_t remaining = file_size - current_bytes;

                    if (remaining == 0) break;

                    bytes_written += write_block_data(block_num, remaining);
                }

                free(block_pointers);
            }
        }

        free(indirect_pointers);
    }

    return bytes_written;
}

void extract_inode_data(uint32_t inode_num) {
    struct ext2_inode inode;
    read_inode(inode_num, &inode);

    uint64_t file_size = (uint64_t)le32_to_cpu(inode.i_size);
    if (S_ISREG(le16_to_cpu(inode.i_mode)) && le32_to_cpu(superblock.s_rev_level) > 0) {
        file_size |= (uint64_t)le32_to_cpu(inode.i_dir_acl) << 32;
    }

    if (file_size == 0) {
        return;
    }

    uint32_t entries_per_block = block_size / sizeof(uint32_t);

    uint32_t direct_blocks = EXT2_DIRECT_BLOCKS;
    uint64_t direct_max_bytes = (uint64_t)direct_blocks * block_size;

    uint32_t singly_indirect_blocks = entries_per_block;
    uint64_t singly_max_bytes = direct_max_bytes + (uint64_t)singly_indirect_blocks * block_size;

    uint64_t doubly_indirect_blocks = (uint64_t)entries_per_block * entries_per_block;
    uint64_t doubly_max_bytes = singly_max_bytes + doubly_indirect_blocks * block_size;

    uint64_t blocks_needed = (file_size + block_size - 1) / block_size;
    uint64_t bytes_written = 0;

    uint32_t direct_blocks_to_process = (blocks_needed < direct_blocks) ? blocks_needed : direct_blocks;
    bytes_written += process_direct_blocks(&inode, file_size, direct_blocks_to_process);

    if (bytes_written < file_size && blocks_needed > direct_blocks) {
        uint32_t singly_block_num = le32_to_cpu(inode.i_block[EXT2_SINGLY_INDIRECT_BLOCK]);
        bytes_written += process_singly_indirect(singly_block_num, file_size, bytes_written,
                                                 singly_max_bytes, entries_per_block);
    }

    if (bytes_written < file_size && blocks_needed > direct_blocks + singly_indirect_blocks) {
        uint32_t doubly_block_num = le32_to_cpu(inode.i_block[EXT2_DOUBLY_INDIRECT_BLOCK]);
        uint64_t remaining_blocks = blocks_needed - direct_blocks - singly_indirect_blocks;

        bytes_written += process_doubly_indirect(doubly_block_num, file_size, bytes_written,
                                                 doubly_max_bytes, entries_per_block, remaining_blocks);
    }

    if (bytes_written < file_size) {
        uint32_t triply_block_num = le32_to_cpu(inode.i_block[EXT2_TRIPLY_INDIRECT_BLOCK]);

        if (triply_block_num == 0) {
            uint64_t hole_size = file_size - bytes_written;
            output_zeros(hole_size);
            bytes_written += hole_size;
        } else {
            uint64_t remaining_size = file_size - bytes_written;
            process_indirect_blocks(triply_block_num, 3, &remaining_size);
            bytes_written = file_size - remaining_size;
        }
    }

    if (bytes_written < file_size) {
        fprintf(stderr, "Warning: %llu bytes of inode %u not read (file_size = %llu)\n",
                file_size - bytes_written, inode_num, file_size);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <image_file> <inode_number>\n", argv[0]);
        return EXIT_FAILURE;
    }

    unsigned long inode_num = strtoul(argv[2], NULL, 10);
    if (inode_num == 0 || errno == ERANGE) {
        fprintf(stderr, "Invalid inode number: %s\n", argv[2]);
        return EXIT_FAILURE;
    }

    fs_file = fopen(argv[1], "rb");
    if (!fs_file) {
        perror("Error opening image file");
        return EXIT_FAILURE;
    }

    if (fseek(fs_file, EXT2_SUPERBLOCK_OFFSET, SEEK_SET) != 0) {
        perror("Error seeking to superblock");
        fclose(fs_file);
        return EXIT_FAILURE;
    }

    if (fread(&superblock, sizeof(superblock), 1, fs_file) != 1) {
        perror("Error reading superblock");
        fclose(fs_file);
        return EXIT_FAILURE;
    }

    if (le16_to_cpu(superblock.s_magic) != EXT2_SIGNATURE) {
        fprintf(stderr, "Invalid superblock signature: 0x%X\n", le16_to_cpu(superblock.s_magic));
        fclose(fs_file);
        return EXIT_FAILURE;
    }

    block_size = 1024 << le32_to_cpu(superblock.s_log_block_size);

    uint32_t groups_count = (le32_to_cpu(superblock.s_blocks_count) -
                             le32_to_cpu(superblock.s_first_data_block) +
                             le32_to_cpu(superblock.s_blocks_per_group) - 1) /
                            le32_to_cpu(superblock.s_blocks_per_group);

    uint32_t gdt_block = le32_to_cpu(superblock.s_first_data_block) + 1;

    group_desc_table = malloc(groups_count * sizeof(struct ext2_group_desc));
    if (!group_desc_table) {
        perror("Memory allocation error for group descriptor table");
        fclose(fs_file);
        return EXIT_FAILURE;
    }

    if (fseek(fs_file, gdt_block * block_size, SEEK_SET) != 0) {
        perror("Error seeking to group descriptor table");
        free(group_desc_table);
        fclose(fs_file);
        return EXIT_FAILURE;
    }

    if (fread(group_desc_table, sizeof(struct ext2_group_desc), groups_count, fs_file) != groups_count) {
        perror("Error reading group descriptor table");
        free(group_desc_table);
        fclose(fs_file);
        return EXIT_FAILURE;
    }

    extract_inode_data((uint32_t) inode_num);

    free(group_desc_table);
    fclose(fs_file);

    return EXIT_SUCCESS;
}