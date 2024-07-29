#include <stdio.h>

int main() {
    if (rename("/tmp/oldname.txt", "/tmp/newname.txt") == -1)
        return 1;
    return 0;
}