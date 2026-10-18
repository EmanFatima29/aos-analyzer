#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    int pid, arrival, burst, priority;
} Job;

#define MAX_JOBS 32

/* --- FCFS (First-Come, First-Served) --- */
void sim_fcfs(Job *jobs, int n) {
    int t = 0;
    double wait_sum = 0, tat_sum = 0;
    printf("\n--- FCFS (First-Come, First-Served) ---\n");
    for (int i = 0; i < n; i++) {
        if (t < jobs[i].arrival) t = jobs[i].arrival;
        int wait = t - jobs[i].arrival;
        int tat  = wait + jobs[i].burst;
        printf("P%d: wait=%d tat=%d\n", jobs[i].pid, wait, tat);
        wait_sum += wait; tat_sum += tat;
        t += jobs[i].burst;
    }
    printf("Avg wait = %.2f, Avg turnaround = %.2f\n", wait_sum/n, tat_sum/n);
}

/* --- SJF (Shortest Job First - Non-preemptive) --- */
void sim_sjf(Job *jobs, int n) {
    int done[MAX_JOBS] = {0};
    int t = 0, completed = 0;
    double wait_sum = 0, tat_sum = 0;
    printf("\n--- SJF (Shortest Job First) ---\n");
    while (completed < n) {
        int best = -1;
        for (int i = 0; i < n; i++) {
            if (done[i]) continue;
            if (jobs[i].arrival > t) continue;
            if (best == -1 || jobs[i].burst < jobs[best].burst) best = i;
        }
        if (best == -1) { t++; continue; }
        int wait = t - jobs[best].arrival;
        int tat = wait + jobs[best].burst;
        printf("P%d: wait=%d tat=%d\n", jobs[best].pid, wait, tat);
        wait_sum += wait; tat_sum += tat;
        t += jobs[best].burst;
        done[best] = 1;
        completed++;
    }
    printf("Avg wait = %.2f, Avg turnaround = %.2f\n", wait_sum/n, tat_sum/n);
}

/* --- RR (Round Robin) --- */
void sim_rr(Job *jobs, int n, int quantum) {
    int rem[MAX_JOBS];
    int t = 0, completed = 0;
    double wait_sum = 0, tat_sum = 0;
    for (int i = 0; i < n; i++) rem[i] = jobs[i].burst;

    printf("\n--- Round Robin (quantum=%d) ---\n", quantum);
    while (completed < n) {
        int any = 0;
        for (int i = 0; i < n; i++) {
            if (rem[i] <= 0) continue;
            if (jobs[i].arrival > t) continue;
            any = 1;
            int run = rem[i] < quantum ? rem[i] : quantum;
            t += run;
            rem[i] -= run;
            if (rem[i] == 0) {
                int tat = t - jobs[i].arrival;
                int wait = tat - jobs[i].burst;
                printf("P%d: wait=%d tat=%d (finished at t=%d)\n", jobs[i].pid, wait, tat, t);
                wait_sum += wait; tat_sum += tat;
                completed++;
            }
        }
        if (!any) t++;
    }
    printf("Avg wait = %.2f, Avg turnaround = %.2f\n", wait_sum/n, tat_sum/n);
}

void run_scheduler_sim(void) {
    printf("\n=== Module C: Scheduling Algorithms Simulation ===\n");
    /* Sample workload: 4 processes */
    Job jobs[] = {
        {1, 0, 10, 2},
        {2, 1, 3,  1},
        {3, 2, 4,  3},
        {4, 3, 2,  4}
    };
    int n = 4;
    
    printf("Workload (PID, Arrival, Burst): (1,0,10) (2,1,3) (3,2,4) (4,3,2)\n");
    
    sim_fcfs(jobs, n);
    sim_sjf(jobs, n);
    sim_rr(jobs, n, 3); /* Quantum of 3 */
}
