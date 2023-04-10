#include <stdio.h>
#include <stdlib.h>
#include "queue.h"
#include <time.h>
#include <sys/time.h>
#include "functions.h"
#include "rcm.h"

#define SIZE 500         // number of rows and cols of sparse array
#define MODE 1           // MODE = 1: Read a sparse array from file, MODE = 2: Create a sparse array, with a specific sparsity
#define SPARSITY 0.6     // the percentage of zeros in sparse array


int* CuthillMckee(int* matrix) {
  int* degrees = degreesCalculation(matrix, SIZE);  // degrees points to a vector with the degree of each row

  queue* Q = queueInit();                            // Init the queue
  int* R = (int*) malloc(SIZE * sizeof(int));        // Allocate memory for the permutations array
  if(R == NULL) {
    printf("ERROR: malloc fail");
    exit(1);
  }
  int Rsize=0;                                       // The num of nodes in R

  int* notVisited = (int*) malloc(SIZE*sizeof(int)); // Allocate memory and init notVisited array
  if(notVisited == NULL) {
    printf("ERROR: malloc fail");
    exit(1);
  }

  // init notVisited array
  for (int i = 0; i < SIZE; i++) {
    *(notVisited+i) = 1;
  }

  while(Rsize != SIZE) {
    int minDegreeIndex = 0;  // The pos of min degree node in matrix
    int minDegree = SIZE+10; // A node can not have degree > SIZE

    // find the min degree
    for (int i = 0; i < SIZE; i++) {
      if(degrees[i] < minDegree && notVisited[i] == 1) {
        minDegreeIndex = i;
        minDegree = degrees[i];
      }
    }

    queueAdd(Q, minDegreeIndex);      // add in Q
    notVisited[minDegreeIndex] = 0;   // This node become visited

    while(!(Q->empty)) {
      int* currentIndex = (int*) malloc(sizeof(int));
      if(currentIndex == NULL) {
        printf("ERROR: malloc fail");
        exit(1);
      }
      queueDel(Q, currentIndex);
      int* neighbors = (int*) malloc(degrees[*currentIndex] * sizeof(int)); // array of neighbors
      if(neighbors == NULL) {
        printf("ERROR: malloc fail");
        exit(1);
      }
      int neighborsCounter=0;  // the num of neighbors

      // find all not visited neighbors
      for (int i = 0; i < SIZE; i++) {
        if(i != *currentIndex && *(matrix+(*currentIndex)*SIZE+i)==1 && notVisited[i]==1) {
          neighbors[neighborsCounter++] = i; // add the neighbor in the array
          notVisited[i] = 0; // and the node become visited
        }
      }

      // sort the neighbors by degree
      sortByDegree(neighbors, degrees, neighborsCounter); // sort the neighbors by degree

      // add the sorted neighbor in Q
      for (int i = 0; i < neighborsCounter; i++) {
        queueAdd(Q, neighbors[i]);
      }

      // add the current node in permutation array R
      R[Rsize++] = *currentIndex;
      free(currentIndex);
      free(neighbors);
    }
  }

  queueDelete(Q);
  free(notVisited);
  free(degrees);


  return R;
}

//  Reverse  CuthillMckee Result
int* ReverseCuthillMckee(int* matrix) {
  int* rcm = CuthillMckee(matrix);

  int n = SIZE;

  if(SIZE % 2 == 0)
    n -= 1;

  n = n / 2;

  for (int i = 0; i <= n; i++) {
    swap(&rcm[SIZE - 1 - i], &rcm[i]);
  }

  return rcm;
}





int main(int argc, char const *argv[]) {

  // Init the sparse array
  int *matrix = (int*) calloc(SIZE * SIZE, sizeof(int));
  if(matrix == NULL) {
    printf("ERROR: malloc fail");
    exit(1);
  }
  init_matrix(matrix, SIZE, MODE, SPARSITY);

  // printf("MATRIX:\n");
  // for (int i = 0; i < SIZE; i++) {
  //   for (int j = 0; j < SIZE; j++) {
  //     printf("%d ", *(matrix+i*SIZE+j));
  //   }
  //   printf("\n");
  // }
  // printf("\n");

  // int* degrees = degreesCalculation(matrix, SIZE);
  // printf("DEGREES:\n");
  // for (int i = 0; i < SIZE; i++) {
  //   printf("%d: %d, ", i, degrees[i]);
  // }
  // printf("\n\n");
  // free(degrees);


  struct timeval start, end;

  int* R = (int*) malloc(SIZE*sizeof(int));
  if(R == NULL) {
    printf("ERROR: malloc fail");
    exit(1);
  }
  // Run the algorithm and messure the execution time
  gettimeofday(&start, NULL);
  R = ReverseCuthillMckee(matrix);
  gettimeofday(&end, NULL);



  // for (int i = 0; i < SIZE; i++) {
  //   printf("%d, ", R[i]);
  // }

  // Calculate the time
  double time = ((double)((end.tv_sec*1e6 + end.tv_usec) - (start.tv_sec*1e6 + start.tv_usec)))*1e-6;
  printf(" >>> ExecutingTime: %lf sec\n", time);


  // Calculate the new array, after permutations
  int* new_matrix = reorder(matrix, R, SIZE);
  output_write(new_matrix, SIZE, SIZE, "output/output_matrix.txt");
  write_vector(R, SIZE, "output/v0_output.txt");

  free(matrix);
  free(R);

  return 0;
}
