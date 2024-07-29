#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd = open("/tmp/read100.txt", O_WRONLY | O_CREAT, 0644);
    if (fd == -1)
        return 1;
    char data[101] = {0};
    for (int i = 0; i < 100; i++)
        data[i] = 'A';
    write(fd, data, 100);
    close(fd);
    return 0;
}