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
#include "stdint.h"
#include <sys/types.h>
#include <omp.h>
#include "N_Body.cpp"

using namespace std;

const unsigned int SIZE = 8;

int main(int argc, char *argv[]) {

	//int numThreads, tid;
	//N_Body<int, int> n;
	/*MPI_Init(&argc, &argv);
	 int rank, proc;
	 MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	 MPI_Comm_size(MPI_COMM_WORLD, &proc);
	 printf("hallo ich bin %d von %d", rank, proc);
	 MPI_Finalize();
	 */

	vector<int> vec;
	N_Body<int, int, int> n;

	vec.push_back(1);
	vec.push_back(2);
	vec.push_back(3);
	vec.push_back(4);
//	vec.push_back(5);
//	vec.push_back(6);
//	vec.push_back(7);
//	vec.push_back(8);

	n.setParticles(&vec);
	n.compute();

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

