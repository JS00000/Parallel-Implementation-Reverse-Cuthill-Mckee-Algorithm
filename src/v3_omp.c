#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include <time.h>
#include <sys/time.h>
#include "functions.h"
#include <omp.h>
#include "rcm.h"

#define SIZE 500         // number of rows and cols of sparse array
#define MODE 1           // MODE = 1: Read a sparse array from file, MODE = 2: Create a sparse array, with a specific sparsity
#define SPARSITY 0.6     // the percentage of zeros in sparse array
#define MAX_THREADS 4    // The max num of threads


int* CuthillMckee(int* matrix) {
  int* degrees = degreesCalculationParallel(matrix, SIZE, MAX_THREADS);

  queue* Q = queueInit();
  int* R = (int*) malloc(SIZE * sizeof(int));
  if(R == NULL) {
    printf("ERROR: malloc fail");
    exit(1);
  }
  int Rsize=0;
  int* notVisited = (int*) malloc(SIZE*sizeof(int));
  if(notVisited == NULL) {
    printf("ERROR: malloc fail");
    exit(1);
  }
  omp_lock_t writelock;
  omp_init_lock(&writelock);
  omp_set_dynamic(0);
  omp_set_nested(1);

  for (int i = 0; i < SIZE; i++) {
    *(notVisited+i) = 1;
  }


  while(Rsize < SIZE) {
    int minDegreeIndex = 0;

    minDegreeIndex = findMinIdxParallel(degrees, notVisited, SIZE, MAX_THREADS);
    // minDegreeIndex = findMinIdx(degrees, notVisited, SIZE);

    queueAdd(Q, minDegreeIndex);
    notVisited[minDegreeIndex] = 0;

    int queue_size = 1;
    int threads_using=queue_size;

    while(!(Q->empty)) {
      int *neighbors[threads_using];
      int neighborsCounter[threads_using];
      int currentIndex[threads_using];
      int tid;

      #pragma omp parallel num_threads(threads_using) private(tid) shared(neighbors, neighborsCounter, currentIndex, notVisited, degrees, Q, queue_size, R)
      {
        tid = omp_get_thread_num();

        omp_set_lock(&writelock);
          queueDel(Q, &currentIndex[tid]);
          queue_size--;
          R[Rsize] = currentIndex[tid];
          Rsize = Rsize + 1;
        omp_unset_lock(&writelock);

        neighbors[tid] = (int*) malloc(degrees[currentIndex[tid]] * sizeof(int));
        neighborsCounter[tid]=0;

        if(threads_using > MAX_THREADS/2) {
          for (int i = 0; i < SIZE; i++) {
            if(i != currentIndex[tid] && *(matrix+(currentIndex[tid])*SIZE+i)==1 && notVisited[i]==1) {
              neighbors[tid][neighborsCounter[tid]] = i;
              neighborsCounter[tid]++;
            }
          }
        }
        else {
          int i;
          int threads_num = ((MAX_THREADS-threads_using)/threads_using)+1;
          int CHUNKSIZE = SIZE / threads_num;
          #pragma omp parallel num_threads(threads_num) private(i) shared(notVisited, neighbors, tid)
          {
            #pragma omp for schedule(dynamic, CHUNKSIZE)
              for ( i = 0; i < SIZE; i++) {
                if(i != currentIndex[tid] && *(matrix+(currentIndex[tid])*SIZE+i)==1 && notVisited[i]==1) {
                  omp_set_lock(&writelock);
                    neighbors[tid][neighborsCounter[tid]] = i;
                    neighborsCounter[tid]++;
                  omp_unset_lock(&writelock);
                }
              }
          }
        }

        sortByDegree(neighbors[tid], degrees, neighborsCounter[tid]);
      }

      AddtoQueue(neighbors, Q, neighborsCounter, R, &Rsize ,currentIndex, notVisited, threads_using, &queue_size);

      threads_using = queue_size >= MAX_THREADS ? MAX_THREADS : queue_size;
    }
  }

  queueDelete(Q);
  free(notVisited);
  free(degrees);


  return R;
}

int* ReverseCuthillMckee(int* matrix) {
  int* rcm = CuthillMckee(matrix);

  int n = SIZE;

  if(SIZE % 2 == 0)
    n -= 1;

  n = n / 2;

  int i;
  int CHUNKSIZE = n / MAX_THREADS;
  #pragma omp parallel num_threads(MAX_THREADS) private(i)
  {
    #pragma omp for schedule(dynamic, CHUNKSIZE)
    for (i = 0; i <= n; i++) {
      swap(&rcm[SIZE - 1 - i], &rcm[i]);
    }
  }


  return rcm;
}


int main(int argc, char const *argv[]) {

  int *matrix = (int*) calloc(SIZE * SIZE, sizeof(int));

  init_matrix(matrix, SIZE, MODE, SPARSITY);

  // printf("MATRIX:\n");
  // for (int i = 0; i < SIZE; i++) {
  //   for (int j = 0; j < SIZE; j++) {
  //     printf("%d ", *(matrix+i*SIZE+j));
  //   }
  //   printf("\n");
  // }
  // printf("\n");
  //
  // int* degrees = degreesCalculation(matrix, SIZE);
  // printf("DEGREES:\n");
  // for (int i = 0; i < SIZE; i++) {
  //   printf("%d: %d, ", i, degrees[i]);
  // }
  // printf("\n\n");
  // free(degrees);


  struct timeval start, end;

  int* R = (int*) malloc(SIZE*sizeof(int));


  gettimeofday(&start, NULL);
  R = ReverseCuthillMckee(matrix);
  gettimeofday(&end, NULL);



  // for (int i = 0; i < SIZE; i++) {
  //   printf("%d, ", R[i]);
  // }


  double time = ((double)((end.tv_sec*1e6 + end.tv_usec) - (start.tv_sec*1e6 + start.tv_usec)))*1e-6;
  printf(" >>> ExecutingTime: %lf sec\n", time);


  write_vector(R, SIZE, "output/v3_output.txt");

  free(matrix);
  free(R);

  return 0;
}
