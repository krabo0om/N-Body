/*
 * N_Body.h
 *
 *  Created on: 10.01.2014
 *      Author: ulkba_000
 */

#ifndef N_BODY_H_
#define N_BODY_H_

#include "mpi.h"
#include <omp.h>
#include "Computation.h"
#include <iostream>
#include <vector>
#include <map>	//TODO unordered map?//TODO structure of parameters/**
 * Main class for controlling the tasks. It will split the work and coordinate the nodes.
 * @tparam P the particle class
 * @tparam ER the end result class
 */
template<class P, class IR, class ER>
class N_Body {
	public:
		/**
		 * Assign the list of particles to compute with
		 * @param data the list of particles
		 */
		void setParticles(std::vector<P> *data) {
			particle_data = *data;
		}

		/**
		 * Starts the computation. The result can be accessed by #getResult().
		 * @return 0 if everything went right
		 */
		int compute() {

			//particle_data = {1,2,3,4,5};
			//mpi master finden
			int mpi_init_i;
			char ** mpi_init_c;    //TODO besseren weg finden?
			MPI_Init(&mpi_init_i, &mpi_init_c);

			int rank, proc;
			MPI_Comm_rank(MPI_COMM_WORLD, &rank);
			MPI_Comm_size(MPI_COMM_WORLD, &proc);

			//broadcast der particle
			MPI_Bcast(&particle_data.front(), particle_data.size(), MPI_INT, 0, MPI_COMM_WORLD);
			//TODO eigenen datentyp erstellen: http://stackoverflow.com/questions/10419990/creating-an-mpi-datatype-for-a-structure-containing-pointers
			//TODO ist ein broadcast nötig, wenn alle mpi clients eh mit den anfangsdaten starten oder ist das aufm hpc anders?

			//alle ermitteln ihren streifen
			int strip_width = (int) ((particle_data.size() / proc) + 0.5);    //round up
			int offset = rank * strip_width;
			if (offset + strip_width > particle_data.size()) {
				strip_width = particle_data.size() - offset;  	//truncate the strip_width for the last process if the strip would be to large
			}
			//berechnung mittels openMP
//			std::map<P, ER> result;
			std::vector<ER> result;
#pragma omp parallel
			{

#pragma omp for
				for (int row = 0; row < strip_width; row++) {    //streifen durchgehen
					P local = particle_data[row + offset];
					std::vector<IR> inter(particle_data.size());
					for (typename vector<P>::iterator it = particle_data.begin(); it != particle_data.end(); it++) {    //compute ER
						inter.push_back(Computation<P, IR, ER>::compute(local, *it));
						//TODO vllt gleich alles berechnen? performance einbuße aufgrund abhängiger schleife?
					}
					ER temp = Computation<P, IR, ER>::aggregate(inter);
					std::cout << "ich bin thread " << omp_get_thread_num() << " und berechnetet streifen " << row+offset << " mit erg " << temp << endl;
					#pragma omp critical
					{
						result.push_back(temp);
					}
				}
			}
			std::vector<ER> endresults;
			//ergebnisse zusammen führen
			if (rank == 0) {
				endresults.reserve(particle_data.size());
			}
			MPI_Gather(&result.front(), strip_width, MPI_INT, &endresults.front(), strip_width, MPI_INT, 0, MPI_COMM_WORLD);
			//fertig

			if (rank == 0) {
				this->result = endresults;
			}

			MPI_Finalize();
			return -1;
			//TODO imagine some error codes
		}

		/**
		 * Returns the result after a successful computation. If there is currently no result, \c NULL will be returned.
		 * @return \c NULL if there is no result, else the result of the latest computation
		 */
		vector<ER>* getResult() {
			return &result;
		}

	private:
		/**
		 * stores the particle data
		 */
		std::vector<P> particle_data;
		std::vector<ER> result;
};

#endif /* N_BODY_H_ */
