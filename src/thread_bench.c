#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

/* We will do a small amount of math, then sleep.
   This simulates waiting for a file read or network request. */
void* io_sleep_work(void* arg) {
    long sum = 0;
    for (long i = 0; i < 10000000; i++) {
        sum += i;
    }
    /* Sleep for 1 second */
    sleep(1); 
    if (sum == 42) printf("Magic\n"); 
    return NULL;
}

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

int run_bench(void) {
    printf("=== Module C: Thread Creation Benchmark ===\n");
    printf("Workload: Math + 1 second sleep per thread.\n");
    printf("Your VM has %ld CPU cores.\n\n", sysconf(_SC_NPROCESSORS_ONLN));

    /* --- TEST 1: SINGLE THREAD --- */
    pthread_t t1;
    double start = get_time();
    pthread_create(&t1, NULL, io_sleep_work, (void*)1);
    pthread_join(t1, NULL);
    double time_1_thread = get_time() - start;
    
    printf("[1 Thread]  Time taken: %.4f seconds\n", time_1_thread);

    /* --- TEST 2: TWO THREADS --- */
    pthread_t t2, t3;
    start = get_time();
    pthread_create(&t2, NULL, io_sleep_work, (void*)2);
    pthread_create(&t3, NULL, io_sleep_work, (void*)3);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    double time_2_threads = get_time() - start;
    
    printf("[2 Threads] Time taken: %.4f seconds\n\n", time_2_threads);

    /* --- CALCULATE SPEEDUP --- */
    printf("Speedup: %.2fx\n", (time_1_thread / time_2_threads));
    printf("Why? The OS ran both threads simultaneously. While one was sleeping, the other ran.\n");

    return 0;
}
