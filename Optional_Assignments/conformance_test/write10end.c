#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd = open("/tmp/write10end.txt", O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd == -1)
        return 1;
    char data[11] = "1234567890";
    write(fd, data, 10);
    close(fd);
    return 0;
}