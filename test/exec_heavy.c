#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
int main(void) {
    for (int i = 0; i < 100; i++) {
        pid_t p = fork();
        if (p == 0) { execl("/bin/true", "true", NULL); _exit(1); }
        else if (p > 0) { waitpid(p, NULL, 0); }
    }
    return 0;
}
