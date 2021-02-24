#include <math.h>
#include <stdio.h>
#include <stdlib.h>

// int values[] = { 88, 56, 100, 2, 25 };
//
// int cmpfunc (const void * a, const void * b) {
//   return ( *(int*)a - *(int*)b );
//}
//
// int main () {
//   int n;
//
//   printf("Before sorting the list is: \n");
//   for( n = 0 ; n < 5; n++ ) {
//      printf("%d ", values[n]);
//   }
//   printf("\n");
//   // void qsort(void *base, size_t nitems, size_t size,
//   //           int (*compar)(const void *, const void *))
//   qsort(values, 5, sizeof(int), cmpfunc);
//
//   printf("\nAfter sorting the list is: \n");
//   for( n = 0 ; n < 5; n++ ) {
//      printf("%d ", values[n]);
//   }
//   printf("\n");
//
//   return(0);
//}

#define N 5
int X[N] = {2, -1, -4, -3, 3};
int Y[N] = {2, -2, 4, 1, -3};
int I[N] = {0, 1, 2, 3, 4};

double dist(int x, int y) { return sqrt(x * x + y * y); }

int cmpfunc(const void *a, const void *b) {
  int x1 = X[*(int *)a];
  int y1 = Y[*(int *)a];
  int x2 = X[*(int *)b];
  int y2 = Y[*(int *)b];
  double d1 = dist(x1, y1);
  double d2 = dist(x2, y2);
  if (d1 == d2)
    return 0;
  else if (d1 < d2)
    return -1;
  else
    return 1;
}

int main() {
    int i;
    for (i = 0; i < N; ++i) {
        printf("%d. dist: %lf\n", i, dist(X[i], Y[i]));
    }

    qsort(I, N, sizeof(I[0]), cmpfunc);
    for (i = 0; i < N; ++i) {
        printf("I[%d]: %d\n", i, I[i]);
    }
    printf("sorted:\n");
    for (i = 0; i < N; ++i) {
        printf("%d. dist: %lf\n", i, dist(X[I[i]], Y[I[i]]));
    }
}
