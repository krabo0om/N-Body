/*
 * n-body.h
 *
 *  Created on: 28.01.2014
 *      Author: yogi, paul
 */

#ifndef N_BODY_H_
#define N_BODY_H_

#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include "mpi.h"
#ifdef _OPENACC
#include <openacc.h>
#endif

//uncomment to calculate particle[x] and particle[y] with x=y
//otherwise particle[y] will temporarily replaced with EPSILON_P
//#define X_Y_CALC

//number of particles
#define NUM_PARTICLES	15000
//number of particles to send in one transmission, can be created than NUM_PARTICLES/MPI_NODE_COUNT
#define CHUNK_SIZE	15000

#define mpi_cnt(X) sizeof(particle) * X

typedef enum {
false, true}bool;

//define your particle struct here
//can't use pointers
typedef struct {
	float value;
	float x, y, z;
} force;

typedef struct {
	float x, y, z;
	float value;

	force f;
} particle;

//an particle which does not alternate the result in the computation, used in the part[x] * part[y] with x = y case
static const particle EPSILON_P = {.x = 0, .y = 0, .z = 0, .value = 0, .f = {.value = 0, .x = 0, .y = 0, .z = 0}};

MPI_Comm nbody_mpi_commring;
MPI_Datatype nbody_mpi_datatype;

//calls MPI_Init() and sets up the mpi comm
void nbody_init();
//calls MPI_Finalize()
void nbody_finalize();
particle calculate(particle p1, particle p2);
void nbody_create_particles(particle[], int size_of_array);
//compute and communicate, particles are just your locals
void nbody_compute(particle particles[]);

#endif /* N_BODY_H_ */
