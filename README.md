# AOS-Analyzer: Advanced Operating Systems Project
**Author:** Eman
**Environment:** Ubuntu 20.04 (VMware)

## Overview
AOS-Analyzer is a Linux process, scheduling, and system call intelligence toolkit written in C. Instead of relying on simulations, this tool directly inspects the Linux kernel via the `/proc` filesystem and `strace` to demonstrate core OS concepts in a real environment.

## Modules Built:
1. **Module A (Process Monitor):** Reads `/proc/[pid]/stat` to calculate live CPU and memory usage. Maps kernel process states (R, S, D, Z) to human-readable labels. Includes a "Safe Kill" feature that checks User IDs before sending signals.
2. **Module B (Syscall Tracer):** Executes programs under `strace`, parses the log files, and categorizes syscalls into File, Process, Network, Memory, and Signals. Proves that `fork()` uses the `clone` syscall.
3. **Module C (Threads & Scheduling):** Demonstrates real `pthread` creation with an I/O-bound workload, proving concurrent execution. Includes a pure algorithm simulator for FCFS, SJF, and Round Robin scheduling.
4. **Module D (Network & Security):** Creates a local TCP client/server, traces the network syscalls (`socket`, `bind`, `connect`, `send`), and outputs a security analysis on distributed system vulnerabilities (plaintext data, no authentication).

## How to Build and Run
```bash
make
./aos-analyzer monitor        # Run Process Monitor
./aos-analyzer trace ./test/file_heavy  # Run Syscall Tracer
./aos-analyzer bench          # Run Thread & Scheduler Bench
./aos-analyzer netdemo        # Run Network & Security Demo
