#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctype.h>

struct __attribute__((__packed__)) superblock_t {
    uint8_t fs_id[8];
    uint16_t block_size;
    uint32_t file_system_block_count;
    uint32_t fat_start_block;
    uint32_t fat_block_count;
    uint32_t root_dir_start_block;
    uint32_t root_dir_block_count;
};

void convertToUpperCase(char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        str[i] = toupper(str[i]);
    }
}

int findFile(FILE* file, const char* sourceFileName, struct superblock_t* sb, int blocksize) {
    int rootblockcount = htonl(sb->root_dir_block_count);
    int rootstartblock = htonl(sb->root_dir_start_block);
    int startingbyte = rootstartblock * blocksize;

    int skip = 0;
    char temp[28];
    int flag = 0;

    for (int i = 0; i < rootblockcount * (blocksize / 64); i++) {
        char stat;
        fseek(file, startingbyte + skip, SEEK_SET);
        fread(&stat, 1, 1, file);

        if (stat & 0x01) {
            fseek(file, startingbyte + skip + 27, SEEK_SET);
            fread(temp, 27, 1, file);
            temp[27] = '\0';

            // Convert filename to uppercase for comparison
            convertToUpperCase(temp);
            char upperFileName[100];
            strcpy(upperFileName, sourceFileName);
            convertToUpperCase(upperFileName);

            if (strcmp(temp, upperFileName) == 0) {
                flag = 1;
                break;
            }
        }

        skip += 64;
    }

    return flag ? EXIT_SUCCESS : EXIT_FAILURE;
}

int copyFileContents(FILE* file, const char* destinationFileName, struct superblock_t* sb, int blocksize, int skip) {
    FILE* filewrite = fopen(destinationFileName, "w");
    if (!filewrite) {
        perror("Error creating destination file");
        return EXIT_FAILURE;
    }

    int startingbyte = htonl(sb->root_dir_start_block) * blocksize;

    fseek(file, startingbyte + skip + 9, SEEK_SET);
    int fssize;
    fread(&fssize, 4, 1, file);
    fssize = ntohl(fssize);

    int startblock;
    fseek(file, startingbyte + skip + 1, SEEK_SET);
    fread(&startblock, 4, 1, file);
    startblock = htonl(startblock);

    int sizeleft = fssize;
    char blocksize_buffer[blocksize];

    while (sizeleft > 0) {
        int bytes_to_copy = (sizeleft > blocksize) ? blocksize : sizeleft;
        fseek(file, startblock * blocksize, SEEK_SET);
        fread(blocksize_buffer, bytes_to_copy, 1, file);
        fwrite(blocksize_buffer, bytes_to_copy, 1, filewrite);

        sizeleft -= bytes_to_copy;

        if (sizeleft > 0) {
            fseek(file, htonl(sb->fat_start_block) * blocksize + startblock * 4, SEEK_SET);
            fread(&startblock, 4, 1, file);
            startblock = ntohl(startblock);
        }
    }

    fclose(filewrite);
    return EXIT_SUCCESS;
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s <file_system_image> <source_file_path> <destination_file_name>\n", argv[0]);
        return EXIT_FAILURE;
    }

    FILE* file = fopen(argv[1], "r+");
    if (!file) {
        perror("Error opening file system image");
        return EXIT_FAILURE;
    }

    struct stat status_buffer;
    int status = fstat(fileno(file), &status_buffer);

    struct superblock_t sb;
    fread(&sb, sizeof(struct superblock_t), 1, file);

    int blocksize = htons(sb.block_size);

    int flag = findFile(file, argv[2], &sb, blocksize);

    if (!flag) {
        printf("File not found\n");
        fclose(file);
        return EXIT_FAILURE;
    }

    int skip = 64; 
    int result = copyFileContents(file, argv[3], &sb, blocksize, skip);

    if (result == EXIT_FAILURE) {
        printf("Error copying file\n");
    } else {
        printf("File copied successfully\n");
    }

    fclose(file);
    return result;
}
