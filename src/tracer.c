#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define MAX_LINE 512

/* simple category classifier */
static const char* categorize(const char *name) {
    static const char *file_sys[]   = {"open","openat","read","write","close",
        "lseek","stat","fstat","lstat","statx","access","unlink","unlinkat",
        "rename","mkdir","rmdir","chmod","chown","dup","dup2","fcntl", NULL};
    static const char *proc_sys[]   = {"fork","vfork","execve","execvp","wait4",
        "waitpid","wait","clone","clone3","exit","exit_group","getpid","getppid", NULL};
    static const char *net_sys[]    = {"socket","bind","listen","accept","accept4",
        "connect","send","sendto","sendmsg","recv","recvfrom","recvmsg",
        "poll","epoll_wait","epoll_create","epoll_create1","epoll_ctl",
        "setsockopt","getsockopt","shutdown", NULL};
    static const char *mem_sys[]    = {"mmap","munmap","mprotect","brk","mremap",
        "madvise", NULL};
    static const char *sig_sys[]    = {"rt_sigaction","rt_sigprocmask","kill",
        "tgkill","sigaltstack","rt_sigreturn", NULL};

    for (int i = 0; file_sys[i]; i++) if (strcmp(name,file_sys[i])==0) return "file";
    for (int i = 0; proc_sys[i]; i++) if (strcmp(name,proc_sys[i])==0) return "process";
    for (int i = 0; net_sys[i];  i++) if (strcmp(name,net_sys[i]) ==0) return "network";
    for (int i = 0; mem_sys[i];  i++) if (strcmp(name,mem_sys[i]) ==0) return "memory";
    for (int i = 0; sig_sys[i];  i++) if (strcmp(name,sig_sys[i]) ==0) return "signal";
    return "other";
}

int run_trace(const char *cmd) {
    char logfile[256];
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    
    /* Ensure logs and reports directories exist */
    mkdir("logs", 0755);
    mkdir("reports", 0755);

    snprintf(logfile, sizeof(logfile),
             "logs/trace_%04d%02d%02d_%02d%02d%02d.log",
             tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
             tm->tm_hour, tm->tm_min, tm->tm_sec);

    pid_t p = fork();
    if (p == 0) {
        /* child: exec strace */
        execlp("strace", "strace",
               "-f", "-T", "-e", "trace=all",
               "-o", logfile, cmd, (char *)NULL);
        perror("execlp strace");
        _exit(1);
    }

    int status;
    waitpid(p, &status, 0);

    /* now parse logfile */
    FILE *f = fopen(logfile, "r");
    if (!f) { perror("open logfile"); return 1; }

    typedef struct { char name[64]; long count; double total_time; } Stat;
    Stat stats[512];
    int   nstats = 0;
    double total_time = 0.0;
    long   total_calls = 0;

    long cat_count[6] = {0};
    const char *cat_names[6] = {"file","process","network","memory","signal","other"};

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        /* Skip resume lines and signal lines */
        char *lt = strstr(line, "<...");
        if (lt && strstr(lt, "resumed>")) continue;
        if (line[0] == '-' || line[0] == '+') continue; 
         if (strncmp(line, "+++", 3) == 0) continue;
        /* format: HH:MM:SS.microseconds syscall(args) = ret <duration> */
        char *syscall_start = strchr(line, ' ');
        if (!syscall_start) continue;
        syscall_start++;
        char name[64];
        int i = 0;
        while (syscall_start[i] && syscall_start[i] != '(' && i < 63) {
            name[i] = syscall_start[i]; i++;
        }
        name[i] = '\0';
        if (i == 0) continue;

        /* find <duration> */
        double dur = 0.0;
        char *lt2 = strrchr(line, '<');
        if (lt2) {
            char *gt = strchr(lt2, '>');
            if (gt) { *gt = '\0'; dur = atof(lt2+1); }
        }

        total_calls++;
        total_time += dur;

        /* add or update stat */
        int found = -1;
        for (int k = 0; k < nstats; k++)
            if (strcmp(stats[k].name, name) == 0) { found = k; break; }
        if (found >= 0) {
            stats[found].count++;
            stats[found].total_time += dur;
        } else if (nstats < 512) {
            strncpy(stats[nstats].name, name, 63);
            stats[nstats].name[63] = '\0';
            stats[nstats].count = 1;
            stats[nstats].total_time = dur;
            nstats++;
        }

        /* category */
        const char *cat = categorize(name);
        for (int k = 0; k < 6; k++)
            if (strcmp(cat, cat_names[k]) == 0) { cat_count[k]++; break; }
    }
    fclose(f);

    /* Sort top-N by count */
    for (int i = 0; i < nstats; i++)
        for (int j = i+1; j < nstats; j++)
            if (stats[j].count > stats[i].count) {
                Stat t = stats[i]; stats[i] = stats[j]; stats[j] = t;
            }

    /* Print report */
    printf("\n=== Trace report for: %s ===\n", cmd);
    printf("Log file: %s\n", logfile);
    printf("Total syscalls: %ld\n", total_calls);
    printf("Total syscall time: %.4f sec\n\n", total_time);

    printf("%-20s %10s %12s\n", "SYSCALL", "COUNT", "TOTAL(ms)");
    printf("---------------------------------------------\n");
    int top = nstats < 15 ? nstats : 15;
    for (int i = 0; i < top; i++)
        printf("%-20s %10ld %12.3f\n",
               stats[i].name, stats[i].count, stats[i].total_time * 1000);

    printf("\nCategory breakdown:\n");
    for (int i = 0; i < 6; i++) {
        if (total_calls > 0)
            printf("  %-10s %8ld  (%5.1f%%)\n",
                   cat_names[i], cat_count[i],
                   100.0 * cat_count[i] / total_calls);
    }

    /* write markdown report */
    char repfile[256];
    snprintf(repfile, sizeof(repfile), "reports/trace_%ld.md", (long)t);
    FILE *rf = fopen(repfile, "w");
    if (rf) {
        fprintf(rf, "# Syscall Trace Report\n\n");
        fprintf(rf, "**Command:** `%s`\n\n", cmd);
        fprintf(rf, "**Total syscalls:** %ld\n\n", total_calls);
        fprintf(rf, "**Total syscall time:** %.4f s\n\n", total_time);

        fprintf(rf, "## Top 15 Syscalls\n\n");
        fprintf(rf, "| Syscall | Count | Total time (ms) |\n");
        fprintf(rf, "|---|---|---|\n");
        for (int i = 0; i < top; i++)
            fprintf(rf, "| %s | %ld | %.3f |\n",
                    stats[i].name, stats[i].count, stats[i].total_time * 1000);

        fprintf(rf, "\n## Category Breakdown\n\n");
        fprintf(rf, "| Category | Count | Percent |\n");
        fprintf(rf, "|---|---|---|\n");
        for (int i = 0; i < 6; i++)
            fprintf(rf, "| %s | %ld | %.1f%% |\n",
                    cat_names[i], cat_count[i],
                    total_calls > 0 ? 100.0 * cat_count[i] / total_calls : 0.0);
        fclose(rf);
        printf("\nMarkdown report written to %s\n", repfile);
    }

    return 0;
}
