#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
int main(void) {
    for (int i = 0; i < 100; i++) {
        pid_t p = fork();
        if (p == 0) { _exit(0); }
        else if (p > 0) { waitpid(p, NULL, 0); }
    }
    return 0;
}
