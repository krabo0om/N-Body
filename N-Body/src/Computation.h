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

//TODO IR und ER zusammen fassen

template<class P, class IR, class ER>
class Computation {
	public:
		static IR compute(P obj1, P obj2) {
			return obj1 + obj2;
		}

		/**
		 * Aggregates two intermediate results to one end result
		 * @param obj1 an intermediate result
		 * @param obj2 an intermediate result
		 * @return the aggregation of the given intermediates
		 */
		static ER aggregate(IR obj1, IR obj2) {
			//change your code here
			return obj1 + obj2;
		}

		/**
		 * Aggregates some intermediate results with an given end result
		 * @param objs some intermediate results
		 * @param obj2 an end result
		 * @return an updated end result
		 */
		static ER aggregate(vector<IR> objs, ER obj2) {
			ER erg = obj2;
			for (typename vector<IR>::iterator it = objs.begin(); it != objs.end(); it++) {
				erg = aggregate(erg, *it);
			}
			return erg;
		}

		/**
		 * Aggregates at least two intermediate results to an end result
		 * @param interim_results at least two intermediate results
		 * @return the aggregation of the given intermediates or \c 0, if there were not enough intermediates
		 */
		static ER aggregate(vector<IR> interim_results) {
			if (interim_results.size() < 2) return 0;
			typename vector<ER>::iterator it = interim_results.begin();
			ER temp = *it; it++;
			ER erg = aggregate(temp, *it);
			for (; it != interim_results.end(); it++) {
				erg += *it;
			}
			return erg;
		}
};

#endif /* COMPUTATION_H_ */
