#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>

#define MAX_PROCS 1024

typedef struct {
    int    pid;
    char   comm[256];
    char   state;
    long   utime, stime;
    long   rss_pages;
    int    nice;
    long   total_ticks;
    double cpu_percent;
    long   rss_kb;
} ProcInfo;

static ProcInfo prev[MAX_PROCS];
static int      prev_count = 0;

static int read_one_proc(int pid, ProcInfo *out) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    FILE *f = fopen(path, "r");
    if (!f) return -1;

    char buf[2048];
    if (!fgets(buf, sizeof(buf), f)) { fclose(f); return -1; }
    fclose(f);

    char *lp = strchr(buf, '(');
    char *rp = strrchr(buf, ')');
    if (!lp || !rp || rp < lp) return -1;

    size_t comm_len = rp - lp - 1;
    if (comm_len >= sizeof(out->comm)) comm_len = sizeof(out->comm) - 1;
    strncpy(out->comm, lp + 1, comm_len);
    out->comm[comm_len] = '\0';

    char *rest = rp + 2;
    out->state = rest[0];

    long utime=0, stime=0, rss=0;
    int  nice_v=0;

    int field = 3;
    char *tok = strtok(rest, " ");
    while (tok) {
        field++;
        tok = strtok(NULL, " ");
        if (!tok) break;

        if (field == 14) utime = atol(tok);
        else if (field == 15) stime = atol(tok);
        else if (field == 19) nice_v = atoi(tok);
        else if (field == 24) { rss = atol(tok); break; }
    }

    out->pid = pid;
    out->utime = utime;
    out->stime = stime;
    out->nice = nice_v;
    out->rss_pages = rss;
    out->total_ticks = utime + stime;
    out->rss_kb = rss * (sysconf(_SC_PAGESIZE) / 1024);
    return 0;
}

static int list_pids(int *pids, int max) {
    DIR *d = opendir("/proc");
    if (!d) return 0;
    struct dirent *e;
    int n = 0;
    while ((e = readdir(d)) && n < max) {
        if (isdigit((unsigned char)e->d_name[0]))
            pids[n++] = atoi(e->d_name);
    }
    closedir(d);
    return n;
}

static ProcInfo* find_prev(int pid) {
    for (int i = 0; i < prev_count; i++)
        if (prev[i].pid == pid) return &prev[i];
    return NULL;
}

static const char* state_label(char s) {
    switch (s) {
        case 'R': return "Running";
        case 'S': return "Sleeping";
        case 'D': return "Disk-wait";
        case 'Z': return "Zombie";
        case 'T': return "Stopped";
        default:  return "Other";
    }
}

int run_monitor(void) {
    long ticks_per_sec = sysconf(_SC_CLK_TCK);
    int  cores = (int)sysconf(_SC_NPROCESSORS_ONLN);

    /* --- FIRST SAMPLE --- */
    int pids[MAX_PROCS];
    int n = list_pids(pids, MAX_PROCS);
    prev_count = 0;

    for (int i = 0; i < n; i++) {
        ProcInfo p;
        if (read_one_proc(pids[i], &p) == 0) {
            prev[prev_count++] = p;
        }
    }

    /* Wait 1 second to measure CPU usage */
    printf("Sampling CPU over 1 second...\n");
    sleep(1); 

    /* --- SECOND SAMPLE --- */
    int n2 = list_pids(pids, MAX_PROCS);
    ProcInfo now[MAX_PROCS];
    int m2 = 0;

    for (int i = 0; i < n2; i++) {
        ProcInfo p;
        if (read_one_proc(pids[i], &p) == 0) {
            ProcInfo *pv = find_prev(p.pid);
            if (pv) {
                long dt = p.total_ticks - pv->total_ticks;
                p.cpu_percent = (100.0 * dt) / (double)(ticks_per_sec * cores);
                if (p.cpu_percent < 0) p.cpu_percent = 0;
            } else {
                p.cpu_percent = 0.0;
            }
            now[m2++] = p;
        }
    }

    /* Sort by CPU% descending */
    for (int i = 0; i < m2; i++)
        for (int j = i+1; j < m2; j++)
            if (now[j].cpu_percent > now[i].cpu_percent) {
                ProcInfo t = now[i]; now[i] = now[j]; now[j] = t;
            }

    /* Print the final table */
    printf("\n%-7s %-20s %-10s %6s %8s %6s\n",
           "PID", "COMMAND", "STATE", "CPU%", "RSS(MB)", "NICE");
    printf("---------------------------------------------------------------\n");
           
    int shown = m2 < 15 ? m2 : 15;
    for (int i = 0; i < shown; i++) {
        printf("%-7d %-20.20s %-10s %6.2f %8.1f %6d\n",
               now[i].pid, now[i].comm, state_label(now[i].state),
               now[i].cpu_percent, now[i].rss_kb / 1024.0, now[i].nice);
    }

    return 0;
}
/* Safe kill — only allows killing processes owned by you */
int safe_kill(int pid, int signal_num) {
    if (pid <= 1) {
        fprintf(stderr, "Refusing to kill pid %d (system critical)\n", pid);
        return -1;
    }

    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *f = fopen(path, "r");
    if (!f) { 
        perror("open /proc/pid/status"); 
        return -1; 
    }

    int  target_uid = -1;
    int  current_uid = getuid();
    char line[256];
    
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "Uid:", 4) == 0) {
            sscanf(line + 4, "%d", &target_uid);
            break;
        }
    }
    fclose(f);

    if (target_uid != current_uid && current_uid != 0) {
        fprintf(stderr, "Refusing: pid %d owned by uid %d, you are uid %d\n",
                pid, target_uid, current_uid);
        return -1;
    }

    if (kill(pid, signal_num) == 0) {
        printf("Success: Sent signal %d to pid %d\n", signal_num, pid);
        return 0;
    }
    perror("kill");
    return -1;
}
