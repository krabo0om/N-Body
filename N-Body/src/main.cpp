/*
 ============================================================================
 Name        : project.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello OpenMP World in C
 ============================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "Computation.h"
#include "stdint.h"
#include <sys/types.h>
#include <omp.h>
#include "N_Body.h"

/**
 * Hello OpenMP World prints the number of threads and the current thread id
 */
int main(int argc, char *argv[]) {

	int numThreads, tid;
	int array[1] = {1};
	//N_Body<int, int> n;
	/*MPI_Init(&argc, &argv);
	int rank, proc;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &proc);
	printf("hallo ich bin %d von %d", rank, proc);
	MPI_Finalize();
*/

	/* This creates a team of threads; each thread has own copy of variables  */
//#pragma omp parallel private(numThreads, tid)
//	{
//		tid = omp_get_thread_num();
//		printf("Hello World from thread number %d\n", tid);
//
//		/* The following is executed by the master thread only (tid=0) */
//		if (tid == 0) {
//			numThreads = omp_get_num_threads();
//			printf("Number of threads is %d\n", numThreads);
//		}
//	}
	return 0;
}

