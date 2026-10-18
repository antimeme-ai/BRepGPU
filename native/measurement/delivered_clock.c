// Delivered-clock probe for measurement context rows.
//
// papers/2026-10-17-heterogeneous-placement-measurement.md §7.9: delivered
// clock is measured, not read from scaling_cur_freq. One thread per listed
// logical CPU is pinned to it, and all threads run a dependent integer add
// chain together: a fixed warm-up, then the requested measurement window.
// One dependent add retires per cycle, so adds per second of the thread's own
// CPU time (CLOCK_THREAD_CPUTIME_ID) is the clock it ran at, and its CPU time
// over wall time is the share of the CPU it got while other work competed.
// Output: one "cpu<TAB>ghz<TAB>cpu_share" row per CPU. Exit 2 on bad
// arguments, 3 if a thread cannot be pinned.
#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if !defined(__x86_64__)
#error "the dependent add chain is written for x86-64"
#endif

enum { MAX_CPUS = 1024 };

static int cpus[MAX_CPUS];
static int count;
static double seconds;
static double ghz[MAX_CPUS];
static double share[MAX_CPUS];
static int pin_failed[MAX_CPUS];
static pthread_barrier_t start;

enum { WARMUP_MILLISECONDS = 20 };

static double clock_seconds(clockid_t id) {
  struct timespec t;
  clock_gettime(id, &t);
  return (double)t.tv_sec + 1e-9 * (double)t.tv_nsec;
}

static double now(void) { return clock_seconds(CLOCK_MONOTONIC); }

static void *chain(void *argument) {
  int index = (int)(intptr_t)argument;
  cpu_set_t set;
  CPU_ZERO(&set);
  CPU_SET(cpus[index], &set);
  pin_failed[index] = pthread_setaffinity_np(pthread_self(), sizeof set, &set) != 0;
  pthread_barrier_wait(&start);
  if (pin_failed[index]) return NULL;
  uint64_t value = 0, adds = 0;
  double begin = now(), elapsed, cpu_begin = 0;
  int warm = 1;
  do {
    for (int k = 0; k < 1000; ++k) {
      __asm__ volatile(
          "add $1, %0\n\tadd $1, %0\n\tadd $1, %0\n\tadd $1, %0\n\t"
          "add $1, %0\n\tadd $1, %0\n\tadd $1, %0\n\tadd $1, %0\n\t"
          "add $1, %0\n\tadd $1, %0\n\tadd $1, %0\n\tadd $1, %0\n\t"
          "add $1, %0\n\tadd $1, %0\n\tadd $1, %0\n\tadd $1, %0\n\t"
          : "+r"(value));
    }
    adds += 16000;
    elapsed = now() - begin;
    if (warm && elapsed >= WARMUP_MILLISECONDS / 1000.0) {
      warm = 0;
      value = adds = 0;
      begin = now();
      cpu_begin = clock_seconds(CLOCK_THREAD_CPUTIME_ID);
      elapsed = 0;
    }
  } while (warm || elapsed < seconds);
  double cpu = clock_seconds(CLOCK_THREAD_CPUTIME_ID) - cpu_begin;
  ghz[index] = cpu > 0 ? (double)adds / cpu / 1e9 : 0;
  share[index] = cpu / elapsed;
  return value == adds && cpu > 0 ? NULL : (void *)1;
}

static int parse_cpus(const char *text) {
  const char *p = text;
  while (*p) {
    char *end;
    errno = 0;
    long cpu = strtol(p, &end, 10);
    if (end == p || errno || cpu < 0 || cpu >= CPU_SETSIZE || count == MAX_CPUS) return 0;
    for (int i = 0; i < count; ++i)
      if (cpus[i] == cpu) return 0;
    cpus[count++] = (int)cpu;
    if (*end == ',') ++end;
    else if (*end) return 0;
    p = end;
  }
  return count > 0;
}

int main(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "usage: brepgpu-delivered-clock MILLISECONDS CPU[,CPU...]\n");
    return 2;
  }
  char *end;
  errno = 0;
  long milliseconds = strtol(argv[1], &end, 10);
  if (*end || errno || milliseconds < 1 || milliseconds > 10000 || !parse_cpus(argv[2])) {
    fprintf(stderr, "brepgpu-delivered-clock: MILLISECONDS 1..10000 and distinct CPU ids required\n");
    return 2;
  }
  seconds = (double)milliseconds / 1000.0;
  pthread_t threads[MAX_CPUS];
  if (pthread_barrier_init(&start, NULL, (unsigned)count)) return 3;
  for (int i = 0; i < count; ++i)
    if (pthread_create(&threads[i], NULL, chain, (void *)(intptr_t)i)) {
      fprintf(stderr, "brepgpu-delivered-clock: thread creation failed\n");
      return 3;
    }
  int failed = 0;
  for (int i = 0; i < count; ++i) {
    void *status;
    pthread_join(threads[i], &status);
    failed |= pin_failed[i] || status != NULL;
  }
  if (failed) {
    fprintf(stderr, "brepgpu-delivered-clock: a thread could not be pinned or its chain miscounted\n");
    return 3;
  }
  printf("cpu\tghz\tcpu_share\n");
  for (int i = 0; i < count; ++i) printf("%d\t%.4f\t%.4f\n", cpus[i], ghz[i], share[i]);
  return 0;
}
