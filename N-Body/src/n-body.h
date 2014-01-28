/*
 * n-body.h
 *
 *  Created on: 28.01.2014
 *      Author: yogi
 */

#ifndef N_BODY_H_
#define N_BODY_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
 * dieses struct definiert den Particle, ersetzen für andere particle art
 */
struct force {
	int value;
};

struct particle {
	int x, y, z;
	int value;
	struct force;
};

/*
 * dieses struct definiert die Kraft die alle Partikel auf einen Partikel ausüben, ersetzen für andere Kraft
 */



struct force calculate_force(struct force , struct force );
void create_particles(struct particle []);
void compute (struct particle []);


#endif /* N_BODY_H_ */
