// // найти адреса блоков данных (прямые и непрямые), извлечь их и вывести в stdout.
//
// #include <stddef.h>
// #include <stdint.h>
// #include <stdio.h>
//
// const size_t start = 0;
// const size_t superblock_start = 1024;
// size_t block_group_descriptor_table_start = -1;
//
// uint32_t big_endian(uint32_t n) {
//     union {
//         uint32_t u32;
//         uint8_t u8[4];
//     } be;
//
//     for (size_t i = 0; i < 4; i++) {
//         size_t shift = (4 - 1 - i) * 8;
//         be.u8[i] = (n >> shift) & 0xFFu;
//     }
//     return be.u32;
// }
//
// typedef struct superblock {
//     int32_t block_size;
//     int32_t blocks_per_group;
//     int32_t inodes_per_group;
//     int32_t inode_size;
// } superblock_t;
//
// typedef struct block_group_descriptor {
//     size_t inode_table_starting_block;
// } block_group_descriptor_t;
//
// typedef struct inode {
//     size_t size;
//     size_t direct_blocks[12];
//     size_t indirect1;
//     size_t indirect2;
//     size_t indirect3;
// } inode_t;
//
// superblock_t read_superblock(FILE *f) {
//     superblock_t sb;
//     fseek(f, superblock_start, start);
//     fseek(f, 24, 0);
//     fread(&sb.block_size, sizeof(sb.block_size), 1, f);
//     sb.block_size = 1024 << sb.block_size;
//     fseek(f, 32, 0);
//     fread(&sb.blocks_per_group, sizeof(sb.blocks_per_group), 1, f);
//     fseek(f, 40, 0);
//     fread(&sb.inodes_per_group, sizeof(sb.inodes_per_group), 1, f);
//
//     char tmp[2];
//     fseek(f, 62, 0);
//     fread(tmp, sizeof(char), 2, f);
//     if (tmp[0] == 0 && tmp[1] == 0) {
//         sb.inode_size = 128;
//     } else {
//         sb.inode_size = 256;
//     }
//
//     if (sb.block_size>1024) {
//         block_group_descriptor_table_start = 1;
//     }
//     else {
//         block_group_descriptor_table_start = 2;
//     }
//     return sb;
// }
//
// block_group_descriptor_t read_block_group_descriptor(FILE *f, superblock_t sb) {
//     block_group_descriptor_t bd;
//     fseek(f, block_group_descriptor_table_start*sb.block_size+8, SEEK_SET);
//     fread(&bd.inode_table_starting_block, sizeof(bd.inode_table_starting_block), 1, f);
//     return bd;
// }
//
// inode_t read_inode(FILE *f, int inode_number) {
//     inode_t data;
//     superblock_t sb = read_superblock(f);
//
//     int group_number = (inode_number - 1) / sb.blocks_per_group;
//     int index = (inode_number - 1) % sb.inodes_per_group;
//     int offset = index * sb.inode_size / sb.block_size;
//
//     block_group_descriptor_t bd = read_block_group_descriptor(f, sb);
//     fseek(f, (offset+bd.inode_table_starting_block)*sb.block_size + index*sb.inode_size, SEEK_SET);
//
// }
//
//
// // Пример: считать блоки из файловой системы ext2
// void print_inode_blocks(struct ext2_inode *inode, FILE *device, uint32_t block_size) {
//     printf("File size: %u bytes\n", inode->i_size);
//
//     // Вывод прямых блоков
//     for (int i = 0; i < EXT2_NDIR_BLOCKS; i++) {
//         if (inode->i_block[i])
//             printf("Direct block %d: %u\n", i, inode->i_block[i]);
//     }
//
//     // Вывод из одинарно косвенного блока
//     if (inode->i_block[EXT2_IND_BLOCK]) {
//         printf("Indirect block: %u\n", inode->i_block[EXT2_IND_BLOCK]);
//
//         uint32_t count = block_size / sizeof(uint32_t);
//         uint32_t blocks[count];
//         fseek(device, inode->i_block[EXT2_IND_BLOCK] * block_size, SEEK_SET);
//         fread(blocks, sizeof(uint32_t), count, device);
//
//         for (uint32_t i = 0; i < count; i++) {
//             if (blocks[i])
//                 printf("  Indirect -> %u\n", blocks[i]);
//         }
//     }
//
//     // Вывод из двойного косвенного блока
//     if (inode->i_block[EXT2_DIND_BLOCK]) {
//         printf("Double indirect block: %u\n", inode->i_block[EXT2_DIND_BLOCK]);
//
//         uint32_t count = block_size / sizeof(uint32_t);
//         uint32_t indirect_blocks[count];
//         fseek(device, inode->i_block[EXT2_DIND_BLOCK] * block_size, SEEK_SET);
//         fread(indirect_blocks, sizeof(uint32_t), count, device);
//
//         for (uint32_t i = 0; i < count; i++) {
//             if (indirect_blocks[i]) {
//                 printf("  Double indirect -> Indirect block: %u\n", indirect_blocks[i]);
//
//                 uint32_t blocks[count];
//                 fseek(device, indirect_blocks[i] * block_size, SEEK_SET);
//                 fread(blocks, sizeof(uint32_t), count, device);
//
//                 for (uint32_t j = 0; j < count; j++) {
//                     if (blocks[j])
//                         printf("    -> %u\n", blocks[j]);
//                 }
//             }
//         }
//     }
//
//     // Вывод из тройного косвенного блока
//     if (inode->i_block[EXT2_TIND_BLOCK]) {
//         printf("Triple indirect block: %u\n", inode->i_block[EXT2_TIND_BLOCK]);
//
//         uint32_t count = block_size / sizeof(uint32_t);
//         uint32_t dind_blocks[count];
//         fseek(device, inode->i_block[EXT2_TIND_BLOCK] * block_size, SEEK_SET);
//         fread(dind_blocks, sizeof(uint32_t), count, device);
//
//         for (uint32_t i = 0; i < count; i++) {
//             if (dind_blocks[i]) {
//                 printf("  Triple indirect -> Double indirect block: %u\n", dind_blocks[i]);
//
//                 uint32_t indirect_blocks[count];
//                 fseek(device, dind_blocks[i] * block_size, SEEK_SET);
//                 fread(indirect_blocks, sizeof(uint32_t), count, device);
//
//                 for (uint32_t j = 0; j < count; j++) {
//                     if (indirect_blocks[j]) {
//                         printf("    -> Indirect block: %u\n", indirect_blocks[j]);
//
//                         uint32_t blocks[count];
//                         fseek(device, indirect_blocks[j] * block_size, SEEK_SET);
//                         fread(blocks, sizeof(uint32_t), count, device);
//
//                         for (uint32_t k = 0; k < count; k++) {
//                             if (blocks[k])
//                                 printf("      -> %u\n", blocks[k]);
//                         }
//                     }
//                 }
//             }
//         }
//     }
// }
//
// int main(int argc, char *argv[]) {
//     if (argc != 2) {
//         printf("Usage: %s <file>\n", argv[0]);
//     }
//     char *filename = argv[1];
//     FILE *f = fopen(filename, "rb");
//     if (f == NULL) {
//         perror("fopen");
//     }
// }
