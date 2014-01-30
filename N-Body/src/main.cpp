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
#include <time.h>



const unsigned int SIZE = 8;
const bool OUTPUT = true;


/*
int main(int argc, char *argv[]) {
/*
 *

	MPI_Init(&argc, &argv);
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	std::vector<int> vec;
	N_Body<int, int, int> n;

	if (rank == 0) {
		srand(time(NULL));
		for (unsigned int i = 0; i < SIZE; i++) {
//			vec.push_back(rand() % 100);
			vec.push_back(i+1);
		}
		n.setParticles(&vec);
	}

	n.compute(SIZE);



	if (rank == 0) {
		if (OUTPUT) {
			std::cout << "erg: ";
			for (unsigned int i = 0; i < n.getResult()->capacity(); i++) {
				std::cout << n.getResult()->at(i) << " ";
			}
			std::cout << std::endl;
		} else {
			std::cout << "done";
		}
	}
	/
	return 0;
}
*/
