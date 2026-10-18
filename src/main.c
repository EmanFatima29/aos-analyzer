#include <stdio.h>
#include <string.h>

int run_monitor(void);
/*add this line */
int safe_kill(int pid, int signal_num);
int run_trace(const char *cmd);
int run_bench(void);
void run_scheduler_sim(void);
int run_netdemo(void);

static void print_help(void) {
    printf("AOS-Analyzer — Advanced OS Project\n");
    printf("Usage:\n");
    printf("  ./aos-analyzer monitor        # Module A: process monitor\n");
    printf("  ./aos-analyzer trace <cmd>    # Module B: syscall tracer\n");
    printf("  ./aos-analyzer bench          # Module C: threads + scheduling\n");
    printf("  ./aos-analyzer netdemo        # Module D: network + security\n");
    printf("  ./aos-analyzer help           # this message\n");
}

int main(int argc, char **argv) {
    if (argc < 2) { print_help(); return 1; }

    if (strcmp(argv[1], "monitor") == 0) return run_monitor();
    if (strcmp(argv[1], "trace") == 0) {
        if (argc < 3) { fprintf(stderr, "trace needs a command\n"); return 1; }
        return run_trace(argv[2]);
    }
         if (strcmp(argv[1], "bench") == 0) {
        run_bench();
        run_scheduler_sim();
        return 0;
    }
    if (strcmp(argv[1], "netdemo") == 0) return run_netdemo();

    /* ---- ADD THIS NEW BLOCK HERE ---- */
    if (strcmp(argv[1], "kill") == 0) {
        if (argc < 4) { 
            fprintf(stderr, "Usage: ./aos-analyzer kill <pid> <signal>\n"); 
            return 1; 
        }
        int pid = atoi(argv[2]);
        int sig = atoi(argv[3]);
        return safe_kill(pid, sig) == 0 ? 0 : 1;
    }
    /* --------------------------------- */

    if (strcmp(argv[1], "help") == 0)    { print_help(); return 0; }

    fprintf(stderr, "Unknown command: %s\n", argv[1]);
    print_help();
    return 1;
}
