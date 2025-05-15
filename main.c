#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "ext2_types.h"

struct ext2_super_block superblock;
struct ext2_group_desc *group_descriptor_table;
struct ext2_inode inode;


#define SUPERBLOCK_OFFSET 1024
#define SUPERBLOCK_SIZE   1024

int read_superblock(FILE *fp) {
    if (fseek(fp, SUPERBLOCK_OFFSET, SEEK_SET) != 0) {
        perror("fseek");
        return -1;
    }

    if (fread(&superblock, sizeof(struct ext2_super_block), 1, fp) != 1) {
        perror("fread superblock");
        return -1;
    }

    return 0;
}


int read_group_descriptors(FILE *fp) {
    const uint32_t gd_table_offset = (EXT2_BLOCK_SIZE(superblock) == SUPERBLOCK_SIZE)
                                         ? SUPERBLOCK_SIZE + SUPERBLOCK_OFFSET
                                         : EXT2_BLOCK_SIZE(superblock);

    if (fseek(fp, gd_table_offset, SEEK_SET) != 0) {
        perror("fseek to group descriptor table");
        return -1;
    }

    uint32_t num_groups = (EXT2_SUPER_BLOCKS_COUNT(superblock) + EXT2_SUPER_BLOCKS_PER_GROUP(superblock) - 1) /
                          EXT2_SUPER_BLOCKS_PER_GROUP(superblock);
    group_descriptor_table = malloc(num_groups * sizeof(struct ext2_group_desc));
    if (fread(group_descriptor_table, sizeof(struct ext2_group_desc), num_groups, fp) != 1) {
        perror("fread group descriptor table");
        return -1;
    }

    return 0;
}

// Чтение inode по его номеру
int read_inode_by_number(FILE *fp, uint32_t inode_num) {
    if (inode_num == 0 || inode_num > EXT2_SUPER_INODES_COUNT(superblock)) {
        fprintf(stderr, "Invalid inode number: %u\n", inode_num);
        return -1;
    }

    uint32_t inodes_per_group = EXT2_SUPER_INODES_PER_GROUP(superblock);
    const uint32_t group = (inode_num - 1) / inodes_per_group;
    const uint32_t index_in_group = (inode_num - 1) % inodes_per_group;

    uint32_t inode_table_block = EXT2_GROUP_INODE_TABLE(group_descriptor_table[group]);

    // Размер inode
    const uint32_t inode_size = EXT2_SUPER_INODE_SIZE(superblock);

    // Смещение до нужного inode (в байтах)
    long int inode_offset = (inode_table_block * EXT2_BLOCK_SIZE(superblock)) + (index_in_group * inode_size);

    // Чтение inode
    if (fseek(fp, inode_offset, SEEK_SET) != 0) {
        perror("fseek to inode");
        return -1;
    }

    if (fread(&inode, sizeof(struct ext2_inode), 1, fp) != 1) {
        perror("fread inode");
        return -1;
    }

    return 0;
}

int print_inode_blocks(FILE *device, uint32_t block_size) {
    printf("File size: %u bytes\n", inode.i_size);

    // Вывод прямых блоков
    for (int i = 0; i < EXT2_NDIR_BLOCKS; i++) {
        if (inode.i_block[i])
            printf("Direct block %d: %u\n", i, inode.i_block[i]);
    }
    return 0;
    // Вывод из одинарно косвенного блока
    if (inode.i_block[EXT2_IND_BLOCK]) {
        printf("Indirect block: %u\n", inode.i_block[EXT2_IND_BLOCK]);

        uint32_t count = block_size / sizeof(uint32_t);
        uint32_t blocks[count];
        fseek(device, inode.i_block[EXT2_IND_BLOCK] * block_size, SEEK_SET);
        fread(blocks, sizeof(uint32_t), count, device);

        for (uint32_t i = 0; i < count; i++) {
            if (blocks[i])
                printf("  Indirect -> %u\n", blocks[i]);
        }
    }

    // Вывод из двойного косвенного блока
    if (inode.i_block[EXT2_DIND_BLOCK]) {
        printf("Double indirect block: %u\n", inode.i_block[EXT2_DIND_BLOCK]);

        uint32_t count = block_size / sizeof(uint32_t);
        uint32_t indirect_blocks[count];
        fseek(device, inode.i_block[EXT2_DIND_BLOCK] * block_size, SEEK_SET);
        fread(indirect_blocks, sizeof(uint32_t), count, device);

        for (uint32_t i = 0; i < count; i++) {
            if (indirect_blocks[i]) {
                printf("  Double indirect -> Indirect block: %u\n", indirect_blocks[i]);

                uint32_t blocks[count];
                fseek(device, indirect_blocks[i] * block_size, SEEK_SET);
                fread(blocks, sizeof(uint32_t), count, device);

                for (uint32_t j = 0; j < count; j++) {
                    if (blocks[j])
                        printf("    -> %u\n", blocks[j]);
                }
            }
        }
    }

    // Вывод из тройного косвенного блока
    if (inode.i_block[EXT2_TIND_BLOCK]) {
        printf("Triple indirect block: %u\n", inode.i_block[EXT2_TIND_BLOCK]);

        uint32_t count = block_size / sizeof(uint32_t);
        uint32_t dind_blocks[count];
        fseek(device, inode.i_block[EXT2_TIND_BLOCK] * block_size, SEEK_SET);
        fread(dind_blocks, sizeof(uint32_t), count, device);

        for (uint32_t i = 0; i < count; i++) {
            if (dind_blocks[i]) {
                printf("  Triple indirect -> Double indirect block: %u\n", dind_blocks[i]);

                uint32_t indirect_blocks[count];
                fseek(device, dind_blocks[i] * block_size, SEEK_SET);
                fread(indirect_blocks, sizeof(uint32_t), count, device);

                for (uint32_t j = 0; j < count; j++) {
                    if (indirect_blocks[j]) {
                        printf("    -> Indirect block: %u\n", indirect_blocks[j]);

                        uint32_t blocks[count];
                        fseek(device, indirect_blocks[j] * block_size, SEEK_SET);
                        fread(blocks, sizeof(uint32_t), count, device);

                        for (uint32_t k = 0; k < count; k++) {
                            if (blocks[k])
                                printf("      -> %u\n", blocks[k]);
                        }
                    }
                }
            }
        }
    }
}


int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: <file> <inode_number>\n");
        exit(-1);
    }
    char *filename = argv[1];
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("fopen");
    }

    if (read_superblock(fp) == -1) {
        return -1;
    }
    if (read_group_descriptors(fp) == -1) {
        return -1;
    }
    if (read_inode_by_number(fp, strtol(argv[2], 0, 10)) == -1) {
        return -1;
    }
    if (print_inode_blocks(0, 0) == -1) {
        return -1;
    }
    return 0;
}
