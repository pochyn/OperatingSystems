#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"

unsigned char *disk;


int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: %s <image file name>\n", argv[0]);
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    printf("Inodes: %d\n", sb->s_inodes_count);
    printf("Blocks: %d\n", sb->s_blocks_count);
    
    struct ext2_group_desc *gd = (struct ext2_group_desc *)(disk + 1024 + EXT2_BLOCK_SIZE);
    printf("Block group:\n");
    printf("    block bitmap: %d\n", gd->bg_block_bitmap);
    printf("    inode bitmap: %d\n", gd->bg_inode_bitmap);
    printf("    inode table: %d\n", gd->bg_inode_table);
    printf("    free blocks: %d\n", gd->bg_free_blocks_count);
    printf("    free inodes: %d\n", gd->bg_free_inodes_count);
    printf("    used_dirs: %d\n", gd->bg_used_dirs_count);


    printf("Block bitmap: ");
    // from tut slides
    unsigned char *block_bits = (unsigned char *)(disk + EXT2_BLOCK_SIZE * gd->bg_block_bitmap);
    for(int byte = 0; byte < sb->s_blocks_count / 8; byte++) {
        for (int bit = 0; bit < 8; bit++){
            int in_use = (block_bits[byte] >> bit) & 0x1;
            printf("%d", in_use);
        }
        printf(" ");
    }
    printf("\n");
    printf("Inode bitmap: ");
    // from tut slides
    unsigned char *inode_bits = (unsigned char *)(disk + EXT2_BLOCK_SIZE * gd->bg_inode_bitmap);
    for(int byte = 0; byte < sb->s_inodes_count / 8; byte++) {
        for (int bit = 0; bit < 8; bit++){
            int in_use = (inode_bits[byte] >> bit) & 0x1;
            printf("%d", in_use);
        }
        printf(" ");
    }

    struct ext2_inode *inodes = (struct ext2_inode *)(disk + EXT2_BLOCK_SIZE * gd->bg_inode_table);
    printf("\n\nInodes:\n");
    int i = 0;
    while (i < sb->s_inodes_count) {
        //all space reserved before EXT2_GOOD_OLD_FIRST_INO unless its root
        if (i == (EXT2_ROOT_INO - 1) || i >= EXT2_GOOD_OLD_FIRST_INO){
            //if there is some data
            if (inodes[i].i_size > 0){
                //512b each
                int blocks = inodes[i].i_blocks / 2;

                //file case
                if (inodes[i].i_mode & EXT2_S_IFREG) {
                    printf("[%d] type: f size: %d links: %d blocks: %d\n", 
                        i + 1, inodes[i].i_size, inodes[i].i_links_count, inodes[i].i_blocks);
                    printf("[%d] Blocks:", i + 1);
                    int j = 0;
                    while (j < blocks) {
                        printf(" %d", inodes[i].i_block[j]);
                        j += 1;
                    }
                }

                //dir case
                if (inodes[i].i_mode & EXT2_S_IFDIR) {
                    printf("[%d] type: d size: %d links: %d blocks: %d\n", 
                        i + 1, inodes[i].i_size, inodes[i].i_links_count, inodes[i].i_blocks);
                    printf("[%d] Blocks:", i + 1);
                    int j = 0;
                    while (j < blocks) {
                        printf(" %d", inodes[i].i_block[j]);
                        j += 1;
                    }
                }
            }
        }
        i += 1;
    }


    printf("\n\nDirecory Blocks:\n");
    int j = 0;
    while (j < sb->s_inodes_count) {
        //all space reserved before EXT2_GOOD_OLD_FIRST_INO unless its root
        if (j == (EXT2_ROOT_INO - 1) || j >= EXT2_GOOD_OLD_FIRST_INO){
            //if there is some data
            if (inodes[j].i_size > 0){
                //need only direcories
                if (inodes[j].i_mode & EXT2_S_IFDIR) {
                    printf("    DIR BLOCK NUM: %d (for inode %d)\n", inodes[j].i_block[0], j + 1);
                    //512b each
                    int blocks = inodes[j].i_blocks / 2;
                    int k = 0;
                    while (k < blocks) {
                        int size = 0;
                        while (size < EXT2_BLOCK_SIZE) {
                            //dir entry
                            struct ext2_dir_entry *entry = (struct ext2_dir_entry *)(disk + EXT2_BLOCK_SIZE * inodes[j].i_block[0] + size);
                            printf("Inode: %d ", entry->inode);
                            printf("rec_len: %d ", entry->rec_len);
                            printf("name_len: %d ", entry->name_len);
                            if (inodes[j].i_mode & EXT2_S_IFREG) {
                                printf("type= f");
                            }
                            if (inodes[j].i_mode & EXT2_S_IFDIR) {
                                printf("type= d");
                            }
                            printf(" name=%.*s\n", entry->name_len, entry->name);
                            size += entry->rec_len;
                        }
                        k += 1;
                    }

                }
            }
        }
        j += 1;
    }


    return 0;
}
