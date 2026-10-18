#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
int main(void) {
    char buf[64] = "hello aos-analyzer\n";
    for (int i = 0; i < 10000; i++) {
        int fd = open("/tmp/aos_test.txt", O_WRONLY|O_CREAT|O_TRUNC, 0644);
        write(fd, buf, strlen(buf));
        close(fd);
    }
    return 0;
}
