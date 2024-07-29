#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd = open("/tmp/read510.txt", O_RDONLY);
    if (fd == -1)
        return 1;
    char buffer[6];
    lseek(fd, 4, SEEK_SET);
    read(fd, buffer, 6);
    close(fd);
    printf("%s\n", buffer);
    return 0;
}