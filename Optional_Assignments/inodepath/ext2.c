#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#define EXT2_SUPERBLOCK_OFFSET 1024
#define EXT2_BLOCK_SIZE 1024
#define EXT2_INODE_SIZE 128
#define EXT2_NAME_LEN 255

struct ext2_inode {             //layout of an inode
    uint16_t i_mode;
    uint16_t i_uid;
    uint32_t i_size;
    uint32_t i_atime;
    uint32_t i_ctime;
    uint32_t i_mtime;
    uint32_t i_dtime;
    uint16_t i_gid;
    uint16_t i_links_count;
    uint32_t i_blocks;
    uint32_t i_flags;
    uint32_t i_osd1;
    uint32_t i_block[15];
    uint32_t i_generation;
    uint32_t i_file_acl;
    uint32_t i_dir_acl;
    uint32_t i_faddr;
    uint8_t i_osd2[12];
};

struct ext2_dir_entry_2 {       //directory entry in ext2 filesystem
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[EXT2_NAME_LEN];
};

void read_inode(int fd, uint32_t inode_num, struct ext2_inode *inode) {
    uint32_t block_group = (inode_num - 1) / 8192;
    uint32_t inode_table_block = 2048 + (block_group * EXT2_BLOCK_SIZE);
    uint32_t inode_offset = ((inode_num - 1) % 8192) * EXT2_INODE_SIZE;

    lseek(fd, inode_table_block + inode_offset, SEEK_SET);
    read(fd, inode, sizeof(struct ext2_inode));
}

void print_inode(char *device_name, char *path) {
    int fd = open(device_name, O_RDONLY);
    if (fd == -1) {
        perror("Error opening device");
        exit(EXIT_FAILURE);
    }

    struct stat file_stat;
    if (stat(path, &file_stat) == -1) {
        perror("Error getting file status");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (!S_ISREG(file_stat.st_mode) && !S_ISDIR(file_stat.st_mode)) {
        printf("Not a regular file or directory\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("%u\n", (uint32_t)file_stat.st_ino);
    close(fd);
}

void print_contents(char *device_name, char *path) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    printf("File size: %ld bytes\n", file_size);

    char *buffer = malloc(file_size + 1);
    if (buffer == NULL) {
        perror("Error allocating memory");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fread(buffer, 1, file_size, file);
    buffer[file_size] = '\0'; // Null-terminate the buffer

    printf("File contents:\n%s\n", buffer);

    free(buffer);
    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s device-file-name path-on-partition inode/data\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *device_name = argv[1];
    char *path = argv[2];
    char *operation = argv[3];

    if (strcmp(operation, "inode") == 0) {
        print_inode(device_name, path);
    } else if (strcmp(operation, "data") == 0) {
        print_contents(device_name, path);
    } else {
        printf("Invalid operation. Use 'inode' or 'data'\n");
        exit(EXIT_FAILURE);
    }

    return 0;
}

