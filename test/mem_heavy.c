#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
int main(void) {
    size_t sz = 1024 * 1024;
    for (int i = 0; i < 50; i++) {
        void *p = mmap(NULL, sz, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        if (p == MAP_FAILED) { perror("mmap"); return 1; }
        memset(p, i, sz);
        munmap(p, sz);
    }
    return 0;
}
