/*
 * your_code_here.c
 *
 *      Author: you
 */
#include "n-body.h"

//insert your calculation code here
inline particle calculate(particle p1, particle p2) {
	p1.x = 2.01 * p2.x + p2.y + p2.z;
	p1.y = p2.x + 2.01 * p2.y + p2.z;
	p1.z = p2.x + p2.y + 2.01 * p2.z;

	return p1;
}

//read your particles from disk or somewhere else
void nbody_create_particles(particle* particles, int size_of_array) {
	int i = 0;

	for (i = 0; i < size_of_array; i++) {
		particle temp;
		temp.value = 1 + i;
		particles[i] = temp;
	}

}
