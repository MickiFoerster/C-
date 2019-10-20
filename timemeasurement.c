#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

void do_work(void) {
  int i;
  for (i = 0; i < INT32_MAX; ++i) {
  }
}

void print(double elapsedTime) {
  fprintf(stderr, "elapsedTime: %fms.\n", elapsedTime);
}

void variant_A(void) {
  struct timeval start, end;
  double elapsedTime;

  gettimeofday(&start, NULL);
  do_work();
  gettimeofday(&end, NULL);

  // compute and print the elapsed time in millisec
  elapsedTime = (end.tv_sec - start.tv_sec) * 1000.0;    // sec to ms
  elapsedTime += (end.tv_usec - start.tv_usec) / 1000.0; // us to ms
  print(elapsedTime);
}

void variant_B(void) {
  struct timespec start, end;
  double elapsedTime;

  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

  do_work();

  clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);

  elapsedTime = (double)(end.tv_sec - start.tv_sec) * 1.0e3 +   // sec -> ms
                (double)(end.tv_nsec - start.tv_nsec) * 1.0e-6; // nsec -> ms
  print(elapsedTime);
}

int main(void) {
  variant_A();
  variant_B();
  return 0;
}

