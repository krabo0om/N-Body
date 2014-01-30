/*
 * n-body.h
 *
 *  Created on: 28.01.2014
 *      Author: yogi, paul
 */

#ifndef N_BODY_H_
#define N_BODY_H_

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>
#include "mpi.h"

typedef enum {
false, true}bool;

/*
 * dieses struct definiert den Particle, ersetzen für andere particle art
 */
typedef struct {
	float value;
	float x, y, z;
} force;

typedef struct {
	float x, y, z;
	float value;

	force f;
} particle;

/*
 * dieses struct definiert die Kraft die alle Partikel auf einen Partikel ausüben, ersetzen für andere Kraft
 */

void setMpiDatatype();
force calculate_force(force f1, force f2);
void create_particles(particle[], int size_of_array);
void compute(particle* particles);

#endif /* N_BODY_H_ */
