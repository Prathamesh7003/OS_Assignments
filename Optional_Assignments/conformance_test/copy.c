#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd1 = open("/tmp/original.txt", O_RDONLY);
    int fd2 = open("/tmp/copy.txt", O_WRONLY | O_CREAT, 0644);
    if (fd1 == -1 || fd2 == -1)
        return 1;
    char buffer[1024];
    int bytes;
    while ((bytes = read(fd1, buffer, 1024)) > 0)
        write(fd2, buffer, bytes);
    close(fd1);
    close(fd2);
    return 0;
}