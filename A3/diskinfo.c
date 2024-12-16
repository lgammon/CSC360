#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h> // for byte order conversion

// Structure to represent the File System Superblock
struct __attribute__((__packed__)) SuperBlock {
    uint8_t fs_identifier[9];
    uint16_t block_size;
    uint32_t fs_size_blocks;
    uint32_t fat_start;
    uint32_t fat_blocks;
    uint32_t root_dir_start;
    uint32_t root_dir_blocks;
};

// Function to read super block information
void readSuperBlock(FILE *file, struct SuperBlock *superblock) {
    fread(superblock->fs_identifier, sizeof(uint8_t), 8, file);
    superblock->fs_identifier[8] = '\0'; // Null-terminate the identifier
    fread(&superblock->block_size, sizeof(uint16_t), 1, file);
    fread(&superblock->fs_size_blocks, sizeof(uint32_t), 1, file);
    fread(&superblock->fat_start, sizeof(uint32_t), 1, file);
    fread(&superblock->fat_blocks, sizeof(uint32_t), 1, file);
    fread(&superblock->root_dir_start, sizeof(uint32_t), 1, file);
    fread(&superblock->root_dir_blocks, sizeof(uint32_t), 1, file);
}

// Function to convert values from big endian to little endian
void convertEndian(struct SuperBlock *superblock) {
    superblock->block_size = ntohs(superblock->block_size);
    superblock->fs_size_blocks = ntohl(superblock->fs_size_blocks);
    superblock->fat_start = ntohl(superblock->fat_start);
    superblock->fat_blocks = ntohl(superblock->fat_blocks);
    superblock->root_dir_start = ntohl(superblock->root_dir_start);
    superblock->root_dir_blocks = ntohl(superblock->root_dir_blocks);
}

// Function to display super block information
void displaySuperBlockInfo(struct SuperBlock *superblock) {
    printf("Super block information\n");
    printf("Block size: %u\n", superblock->block_size);
    printf("Block count: %u\n", superblock->fs_size_blocks);
    printf("FAT starts: %u\n", superblock->fat_start);
    printf("FAT blocks: %u\n", superblock->fat_blocks);
    printf("Root directory starts: %u\n", superblock->root_dir_start);
    printf("Root directory blocks: %u\n", superblock->root_dir_blocks);
}

// Function to read and display file system information
void displayFileSystemInfo(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        printf("Error opening file.\n");
        return;
    }

    struct SuperBlock superblock;

    readSuperBlock(file, &superblock);
    convertEndian(&superblock);
    displaySuperBlockInfo(&superblock);

    // Read and calculate FAT information
    fseek(file, superblock.fat_start * superblock.block_size, SEEK_SET);

    uint32_t allocatedBlocks = 0;
    uint32_t reservedBlocks = 0;
    uint32_t freeBlocks = 0;

    for (uint32_t i = 0; i < superblock.fat_blocks * (superblock.block_size / sizeof(uint32_t)); ++i) {
        uint32_t value;
        fread(&value, sizeof(uint32_t), 1, file);
        value = ntohl(value);
        if (value == 0x00000001) {
            reservedBlocks++;
        } else if (value == 0x00000000) {
            freeBlocks++;
        } else {
            allocatedBlocks++;
        }
    }

    // Display FAT information
    printf("\nFAT information:\n");
    printf("Free Blocks: %u\n", freeBlocks);
    printf("Reserved Blocks: %u\n", reservedBlocks);
    printf("Allocated Blocks: %u\n", allocatedBlocks);

    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <file_system_image>\n", argv[0]);
        return EXIT_FAILURE;
    }

    displayFileSystemInfo(argv[1]);

    return EXIT_SUCCESS;
}
