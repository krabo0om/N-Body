/*
 * Computation.h
 *
 *  Created on: 10.01.2014
 *      Author: ulkba_000
 */

#ifndef COMPUTATION_H_
#define COMPUTATION_H_

#include <vector>

using namespace std;

template <class P, class IR, class ER>
class Computation {
public:
		static IR compute(P obj1, P obj2){
			return obj1 * obj2;
		}
		static ER aggregate(vector<IR> interim_results){
			int erg = 0;
			for (vector<int>::iterator it = interim_results.begin(); it != interim_results.end(); it++){
				erg += *it;
			}
			return erg;
		}
};

#endif /* COMPUTATION_H_ */
