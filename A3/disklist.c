//doesnt support nested subdirectories............. yet
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>

struct __attribute__((__packed__)) superblock_t {
    uint8_t   fs_id[8];
    uint16_t block_size;
    uint32_t file_system_block_count;
    uint32_t fat_start_block;
    uint32_t fat_block_count;
    uint32_t root_dir_start_block;
    uint32_t root_dir_block_count;
};

// Function to print 'F' for file and 'D' for directory
void printFileOrDirectory(uint8_t stat) {
    if (stat & 0x01) {
        if (stat & 0x02) {
            printf("F ");  // File
        } else if (stat & 0x04) {
            printf("D ");  // Directory
        }
    }
}

// Function to print file size
void printFileSize(FILE* file, int starting_byte, int skip) {
    uint32_t fssize;
    fseek(file, starting_byte + skip + 9, SEEK_SET);
    fread(&fssize, sizeof(uint32_t), 1, file);
    fssize = ntohl(fssize);
    printf("%10d ", fssize);
}

// Function to print file name
void printFileName(FILE* file, int starting_byte, int skip) {
    int space = 0;
    uint8_t stat;
    fseek(file, starting_byte + skip + 27, SEEK_SET);
    for (int name_count = 27; name_count < 54; name_count++) {
        fread(&stat, sizeof(uint8_t), 1, file);
        if (space == 0) {
            printf("%30c", stat);
            space = 1;
        } else if (space == 1) {
            printf("%c", stat);
        }
    }
}

// Function to print file timestamp
void printFileTime(FILE* file, int starting_byte, int skip) {
    uint16_t fssize;
    fseek(file, starting_byte + skip + 20, SEEK_SET);
    fread(&fssize, sizeof(uint16_t), 1, file);
    fssize = ntohs(fssize);
    printf("\t%d/", fssize);

    uint8_t fileTime;
    for (int time_offset = 22; time_offset <= 26; time_offset++) {
        fseek(file, starting_byte + skip + time_offset, SEEK_SET);
        fread(&fileTime, sizeof(uint8_t), 1, file);
        printf("%.2d", fileTime);
        if (time_offset < 26) {
            printf(":");
        }
    }
    printf("\n");
}

// Function to print directory contents
void printDirectoryContents(FILE* file, int block_size, int root_block_count, int starting_byte) {
    int skip = 0;
    for (int i = 0; i < root_block_count * (block_size / 64); i++) {
        fseek(file, starting_byte + skip, SEEK_SET);
        uint8_t stat;
        fread(&stat, sizeof(uint8_t), 1, file);

        printFileOrDirectory(stat);
        
        if (stat & 0x01) {
            printFileSize(file, starting_byte, skip);
            printFileName(file, starting_byte, skip);
            printFileTime(file, starting_byte, skip);
        }

        skip = skip + 64;
    }
}

// Recursive function to traverse and print directory contents
void traverseAndPrint(FILE* file, int block_size, int starting_byte, int depth) {
    int skip = 0;
    while (1) {
        fseek(file, starting_byte + skip, SEEK_SET);
        uint8_t stat;
        fread(&stat, sizeof(uint8_t), 1, file);

        if (depth == 0) {
            printFileOrDirectory(stat);

            if (stat & 0x01) {
                printFileSize(file, starting_byte, skip);
                printFileName(file, starting_byte, skip);
                printFileTime(file, starting_byte, skip);
            }
        }

        if ((stat & 0x04) && (stat & 0x01)) { // If it's a directory
            int subdir_start_block;
            fseek(file, starting_byte + skip + 1, SEEK_SET);
            fread(&subdir_start_block, sizeof(int), 1, file);
            subdir_start_block = ntohl(subdir_start_block);

            if (subdir_start_block != 0) {
                traverseAndPrint(file, block_size, subdir_start_block * block_size, depth + 1);
            }
        }

        skip = skip + 64;

        if (skip >= block_size) {
            break;
        }
    }
}

// Function to compare directory names
int compareDirName(FILE* file, int starting_byte, int skip, const char* target_dir_name) {
    char dirname[28]; // Assuming the directory name is within 27 bytes, plus a null terminator
    fseek(file, starting_byte + skip + 27, SEEK_SET);
    fread(dirname, sizeof(char), 27, file);
    dirname[27] = '\0'; // Null terminate the string

    return strcmp(dirname, target_dir_name);
}

// Function to find the starting block of a directory by name
int findDirectoryStartBlock(FILE* file, int block_size, int root_start_block, int root_block_count, const char* target_dir_name) {
    int starting_byte = root_start_block * block_size;
    int skip = 0;

    while (1) {
        fseek(file, starting_byte + skip, SEEK_SET);
        uint8_t stat;
        fread(&stat, sizeof(uint8_t), 1, file);

        if (stat & 0x01) {
            if (compareDirName(file, starting_byte, skip, target_dir_name) == 0) {
                // Found the target directory
                int subdir_start_block;
                fseek(file, starting_byte + skip + 1, SEEK_SET);
                fread(&subdir_start_block, sizeof(int), 1, file);
                subdir_start_block = ntohl(subdir_start_block);
                return subdir_start_block;
            }
        }

        skip = skip + 64;

        if (skip >= block_size) {
            break;
        }
    }

    // Directory not found
    return -1;
}

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc < 2 || argc > 3) {
        printf("Usage: %s <file_system_image> [/optional_subdir]\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Open file
    int fd = open(argv[1], O_RDWR);
    if (fd == -1) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    FILE* file = fdopen(fd, "r+");
    if (file == NULL) {
        perror("Error converting file descriptor to FILE*");
        close(fd);
        return EXIT_FAILURE;
    }

    // Read superblock information
    struct superblock_t sb;
    fread(&sb, sizeof(struct superblock_t), 1, file);

    // Extract necessary block information
    int block_size = htons(sb.block_size);
    int root_block_count = htonl(sb.root_dir_block_count);
    int root_start_block = htonl(sb.root_dir_start_block);
    int starting_byte = root_start_block * block_size;

    // Check if a subdirectory is specified
    if (argc == 2 || (argc == 3 && strcmp(argv[2], "/") == 0)) {
        // Print contents of the root directory
        traverseAndPrint(file, block_size, starting_byte, 0);
    } else if (argc == 3) {
        // Check if the specified subdirectory exists
        int subdir_start_block = findDirectoryStartBlock(file, block_size, root_start_block, root_block_count, argv[2] + 1);

        if (subdir_start_block != -1) {
            // Traverse and print contents of the specified subdirectory
            traverseAndPrint(file, block_size, subdir_start_block * block_size, 0);
        } else {
            printf("The directory '%s' was not found.\n", argv[2] + 1);
        }
    }

    // Close file
    fclose(file);
    return EXIT_SUCCESS;
}