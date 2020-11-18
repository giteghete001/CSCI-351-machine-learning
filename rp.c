/*
Copyright (c) 2016-2020 Jeremy Iverson & Godgift O. Iteghete

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

/* assert */
#include <assert.h>

/* fabs */
#include <math.h>

/* MPI API */
#include <mpi.h>

/* OMP API */
#include <omp.h>

/* printf, fopen, fclose, fscanf, scanf */
#include <stdio.h>

/* EXIT_SUCCESS, malloc, calloc, free, qsort */
#include <stdlib.h>

#define MPI_SIZE_T MPI_UNSIGNED_LONG

struct distance_metric {
  size_t viewer_id;
  double distance;
};

static int
cmp(void const *ap, void const *bp)
{
  struct distance_metric const a = *(struct distance_metric*)ap;
  struct distance_metric const b = *(struct distance_metric*)bp;

  return a.distance < b.distance ? -1 : 1;
}

int
main(int argc, char * argv[])
{
  int ret, p, rank;
  size_t n, m, k;
  double * rating;

  /* Initialize MPI environment. */
  ret = MPI_Init(&argc, &argv);
  assert(MPI_SUCCESS == ret);

  /* Get size of world communicator. */
  ret = MPI_Comm_size(MPI_COMM_WORLD, &p);
  assert(ret == MPI_SUCCESS);

  /* Get my rank. */
  ret = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  assert(ret == MPI_SUCCESS);

  /* Validate command line arguments. */
  assert(2 == argc);

  /* Read input --- only if your rank 0. */
  if (0 == rank) {
    /* ... */
    char const * const fn = argv[1];

    /* Validate input. */
    assert(fn);

    /* Open file. */
    FILE * const fp = fopen(fn, "r");
    assert(fp);

    /* Read file. */
    fscanf(fp, "%zu %zu", &n, &m);

    /* Allocate memory. */
    rating = malloc(n * m * sizeof(*rating));

    /* Check for success. */
    assert(rating);

    for (size_t i = 0; i < n; i++) {
      for (size_t j = 0; j < m; j++) {
        fscanf(fp, "%lf", &rating[i * m + j]);
      }
    }

    /* Close file. */
    ret = fclose(fp);
    assert(!ret);
  }

  /* Send number of viewers and movies to rest of processes. */
  /*if (0 == rank) {
    for (int r = 1; r < p; r++) {
      ret = MPI_Send(&n, 1, MPI_SIZE_T, r, 0, MPI_COMM_WORLD);
      assert(MPI_SUCCESS == ret);
      ret = MPI_Send(&m, 1, MPI_SIZE_T, r, 0, MPI_COMM_WORLD);
      assert(MPI_SUCCESS == ret);
    }
  } else {
      ret = MPI_Recv(&n, 1, MPI_SIZE_T, 0, 0, MPI_COMM_WORLD,
        MPI_STATUS_IGNORE);
      assert(MPI_SUCCESS == ret);
      ret = MPI_Recv(&m, 1, MPI_SIZE_T, 0, 0, MPI_COMM_WORLD,
        MPI_STATUS_IGNORE);
      assert(MPI_SUCCESS == ret);
  }*/

ret = MPI_Bcast(&n, 1, MPI_SIZE_T, 0, MPI_COMM_WORLD);
assert(MPI_SUCCESS == ret);
ret = MPI_Bcast(&m, 1, MPI_SIZE_T, 0, MPI_COMM_WORLD);
assert(MPI_SUCCESS == ret);

  /* Compute base number of viewers. */
  size_t const base = 1 + ((n - 1) / p); // ceil(n / p)

  /* Compute local number of viewers. */
  size_t const ln = (rank + 1) * base > n ? n - rank * base : base;

  if (0 != rank){
  /* Allocate memory. */
    rating = malloc(ln * m * sizeof(*rating));

    /* Check for success. */
    assert(rating);
}

  /* Send viewer data to rest of processes. */

int * sendarray;
int * displs; 
  if (0 == rank) {
	sendarray = malloc(p * sizeof(*sendarray));
	assert(sendarray);
	displs = malloc(p * sizeof(*displs));
	assert(displs);

    for (int r = 0; r < p; r++) {
      size_t const rn = (r + 1) * base > n ? n - r * base : base;
      sendarray[r] = rn * m;
      displs[r] = r * base * m;
    }
  } 

ret = MPI_Scatterv(rating, sendarray, displs, MPI_DOUBLE, rating, ln * m, MPI_DOUBLE, 0, MPI_COMM_WORLD);
assert(MPI_SUCCESS == ret);

if (0 == rank)
{
free(sendarray);
free(displs);
}

  /* Allocate more memory. */
  double * const urating = malloc((m - 1) * sizeof(*urating));

  /* Check for success. */
  assert(urating);

  /* Get user input and send it to rest of processes. */
  if (0 == rank) {
    for (size_t j = 0; j < m - 1; j++) {
      printf("Enter your rating for movie %zu: ", j + 1);
      fflush(stdout);
      scanf("%lf", &urating[j]);
    }
}
    /*for (int r = 1; r < p; r++) {
      ret = MPI_Send(urating, m - 1, MPI_DOUBLE, r, 0, MPI_COMM_WORLD);
      assert(MPI_SUCCESS == ret);
    }
  } else {
    ret = MPI_Recv(urating, m - 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD,
      MPI_STATUS_IGNORE);
    assert(MPI_SUCCESS == ret);
  }*/


ret = MPI_Bcast(urating, m-1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
assert(MPI_SUCCESS == ret);

double const ts = omp_get_wtime();

  /* Allocate more memory. */
  double * distance;
 
  if (0 == rank) {
    distance = calloc(n, sizeof(*distance));
  } else {
    distance = calloc(ln, sizeof(*distance));
  }

  /* Check for success. */
  assert(distance);

double ts1 = omp_get_wtime();

  /* Compute distances. */
  for (size_t i = 0; i < ln; i++) {
    for (size_t j = 0; j < m - 1; j++) {
      distance[i] += fabs(urating[j] - rating[i * m + j]);
    }
  }
double te1 = omp_get_wtime();
double local_elapsed = te1 - ts1;
double global_elapsed;


//double ts2 = omp_get_wtime();
//if (0 == rank){
ret = MPI_Reduce(&local_elapsed, &global_elapsed , 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
assert(MPI_SUCCESS == ret);
//}
//double te2 = omp_get_wtime();
//double redT = te2 - te2;


  int *recievearray;
  int *displs1; 
  if (0 == rank) {
	recievearray = malloc(p * sizeof(*recievearray));
	assert(recievearray);
	displs1 = malloc(p * sizeof(*displs1));
	assert(displs1);

    for (int r = 0; r < p; r++) {
      size_t const rn = (r + 1) * base > n ? n - r * base : base;
      recievearray[r] = rn;
      displs1[r] = r * base ;
    }
  } 
      
 
ret = MPI_Gatherv(distance, ln, MPI_DOUBLE, distance, recievearray, displs1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
assert(MPI_SUCCESS == ret);

if (0 == rank)
{
free(recievearray);
free(displs1);
}


  if (0 == rank) {
    struct distance_metric * distance2 = malloc(n * sizeof(*distance2));
    assert(distance2);

    for (size_t i = 0; i < n; i++) {
      distance2[i].viewer_id = i;
      distance2[i].distance = distance[i];
    }

    /* Sort distances. */
    qsort(distance2, n, sizeof(*distance2), cmp);

    /* Get user input. */
    printf("Enter the number of similar viewers to report: ");
    fflush(stdout);
    scanf("%zu", &k);

    /* Output k viewers who are least different from the user. */
    printf("Viewer ID   Movie five   Distance\n");
    printf("---------------------------------\n");

    for (size_t i = 0; i < k; i++) {
      printf("%9zu   %10.1lf   %8.1lf\n", distance2[i].viewer_id + 1,
        rating[distance2[i].viewer_id * m + 4], distance2[i].distance);
    }

    printf("---------------------------------\n");

    /* Compute the average to make the prediction. */
    double sum = 0.0;
    for (size_t i = 0; i < k; i++) {
      sum += rating[distance2[i].viewer_id * m + 4];
    }

    double const te = omp_get_wtime();

    /* Output elapsed time */
    printf("It took %fs to compute the prediction.\n", te-ts);

if (0 == rank){
    /* Output compute distance time */
    printf("It took %fs to compute the distances.\n", global_elapsed);
    }

    /***** RUN THIS PROGRAM MULTIPLE TIMES FOR EACH NUMBER OF PROCESSORS AND GET THE LOWEST TIME FOR EACH PROCESSOR THEN CREATE A GRAPH******/


    /* Output reduction time*
    printf("The total reduction time was %fs.\n", redT); */

    /* Output prediction. */
    printf("The predicted rating for movie five is %.1lf.\n", sum / k);

    free(distance2);
  }
    
  /* Deallocate memory. */
  free(rating);
  free(urating);
  free(distance);

  ret = MPI_Finalize();
  assert(MPI_SUCCESS == ret);

  return EXIT_SUCCESS;
}

/*
Thread Count 1: 
1.)0.000746s
2.)0.000642s
3.)0.000622s 
4.)0.000625s 
5.)0.000638s 
Minimum Run-Time: 0.000622s

Thread Count 2:
1.)0.000530s
2.)0.000563s
3.)0.000599s
4.)0.000485s
5.)0.000483s
Minimum Run-Time: 0.000483s

Thread Count 4: 
1.)0.000623s
2.)0.000497s 
3.)0.000609s 
4.)0.000624s
5.)0.000498s
Minimum Run-Time: 0.000498s

Thread Count 8:
1.)0.000590s
2.)0.000537s
3.)0.000560s
4.)0.000350s
5.)0.000575s
Minimum Run-Time: 0.000350s

Thread Count 16:
1.)0.000611s
2.)0.000530s 
3.)0.001026s
4.)0.000636s
5.)0.000539s 
Minimum Run-Time: 0.000530s 

*/

