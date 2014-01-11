/*
 * N_Body.h
 *
 *  Created on: 10.01.2014
 *      Author: ulkba_000
 */

#ifndef N_BODY_H_
#define N_BODY_H_

//TODO structure of parameters

/**
 * Main class for controlling the tasks. It will split the work and coordinate the nodes.
 * @tparam P the particle class
 * @tparam ER the end result class
 */
template <class P, class ER>
class N_Body {
public:
	/**
	 * @param data	an array of particles to compute with
	 * @param size	the size of the array
	 */
	N_Body(P data[], int size);

	/**
	 * Starts the computation. The result can be accessed by #getResult().
	 * @return 0 if everything went right
	 */
	int compute();

	/**
	 * Returns the result after a successful computation. If there is currently no result, \c NULL will be returned.
	 * @return \c NULL if there is no result, else the result of the latest computation
	 */
	ER getResult();

private:
	/**
	 * stores the particle data
	 */
	P particle_data[];
	/**
	 * stores the size of the particle list
	 */
	int data_size;
};

#endif /* N_BODY_H_ */
