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
#include <map>

static const unsigned int CHUNK_SIZE = 2;
static const unsigned int MPI_NEW_DATA_TAG = 111;

static const MPI_Datatype mpi_datatype = MPI_INT;
/**
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
		 * @param list_size size of the complete list of particles
		 * @return 0 if everything went right
		 */
		int compute(unsigned int list_size) {

			//particle_data = {1,2,3,4,5};
			//mpi master finden
//			int mpi_init_i;
//			char ** mpi_init_c;    //TODO besseren weg finden?
//			MPI_Init(&mpi_init_i, &mpi_init_c);

			int rank, proc;
			MPI_Comm_rank(MPI_COMM_WORLD, &rank);
			MPI_Comm_size(MPI_COMM_WORLD, &proc);

			//broadcast der particle
//			MPI_Bcast(&particle_data.front(), particle_data.size(), MPI_INT, 0, MPI_COMM_WORLD);
			//TODO eigenen datentyp erstellen: http://stackoverflow.com/questions/10419990/creating-an-mpi-datatype-for-a-structure-containing-pointers
			//TODO ist ein broadcast nötig, wenn alle mpi clients eh mit den anfangsdaten starten oder ist das aufm hpc anders?

			//TODO letzter Node sollte root sein, der hat meist weniger zu tun
			//alle ermitteln ihren streifen
			int strip_width = (int) ((list_size / proc) + 0.5);		//round up
			int number_of_transmissions = list_size / CHUNK_SIZE;		//TODO auf rundungen prüfen
			/*int offset = rank * strip_width;
			 if (offset + strip_width > list_size) {
			 strip_width = list_size - offset;    //truncate the strip_width for the last process if the strip would be to large
			 }*/

			//stores the chunk of data to compute with and to send away
			std::vector<P> buffer_out(CHUNK_SIZE);
			//stores the incoming chunk of data
			std::vector<P> buffer_in(CHUNK_SIZE);
			//stores your particles
			std::vector<P> locals(strip_width);
			//send the locals to the respective nodes
			MPI_Scatter(&particle_data.front(), strip_width, mpi_datatype, &locals.front(), strip_width, mpi_datatype, 0, MPI_COMM_WORLD);

			if (locals.size() > CHUNK_SIZE) {
				std::cerr << "chunks too small, aborting" << std::endl;
				return -1337;
			}
			buffer_out.assign(locals.begin(), locals.end());	//TODO locals.size() > CHUNK_SIZE

			//berechnung mittels openMP
			std::vector<ER> result(strip_width);

			//start sending first chunk of data
			MPI_Request send_status;
			MPI_Request recv_status;
			int num_send_rec = 0;
			bool receive = true;
			while (receive) {
				//TODO auf openAcc umstellen
				//communicate
				if (num_send_rec < number_of_transmissions - 1) {    //not the last run -> need to send
					MPI_Isend(&buffer_out.front(), CHUNK_SIZE, mpi_datatype, (rank + 1) % proc, MPI_NEW_DATA_TAG, MPI_COMM_WORLD,
							&send_status);
					MPI_Irecv(&buffer_in.front(), CHUNK_SIZE, mpi_datatype, rank == 0 ? proc - 1 : rank - 1, MPI_NEW_DATA_TAG,
					MPI_COMM_WORLD, &recv_status);
				}
#pragma omp parallel
				{

//TODO vllt doch umdrehen, innenere <-> äußere schleife, wegen innen nicht parallel
#pragma omp for
					for (int i = 0; i < locals.size(); i++) {    //streifen durchgehen
						P local = locals[i];
//						std::cout << rank << " " << omp_get_thread_num() << " " << i << " " << local << std::endl;
//						std::flush(cout);
						//stores intermediate results
						std::vector<IR> inter;
						//TODO folgende schleife wird nicht parallel ausgeführt
						for (int next = 0; next < buffer_out.size(); next++) {    //compute partial ER
							IR temp = Computation<P, IR, ER>::compute(local, buffer_out[next]);
							inter.push_back(temp);
						}
						ER temp = Computation<P, IR, ER>::aggregate(inter, result[i]);
						result[i] = temp;    //TODO BUG!! überschreibt immer und nimmt nicht das zwischenergebnis als grundlage
						inter.clear();
					}
				}    //end of pragma omp parallel

//				std::cout << "rank: " << rank << ", num_send_rec: " << num_send_rec << ", number_of_trans: " << number_of_transmissions
//						<< std::endl;

				if (++num_send_rec == number_of_transmissions) {	//done
					receive = false;
				} else {
					//wait for comm to finish
					MPI_Status temp;
					MPI_Wait(&send_status, &temp);
					MPI_Wait(&recv_status, &temp);
					//free buffer_in, store data for next computation phase
					buffer_out.assign(buffer_in.begin(), buffer_in.end());
				}
			}

			locals.clear();
			buffer_out.clear();
			buffer_in.clear();
			std::vector<ER> endresults;
			//ergebnisse zusammen führen
			if (rank == 0) {
				endresults.resize(list_size);
			}
			//TODO use mpi_gatherV
//			std::cout << "rank: " << rank << " for gather" << std::endl;
//			std::flush(cout);
			MPI_Gather(&result.front(), strip_width, MPI_INT, &endresults.front(), strip_width, MPI_INT, 0, MPI_COMM_WORLD);
//			std::cout << "rank: " << rank << " after gather" << std::endl;
			//fertig

			if (rank == 0) {
				this->result.assign(endresults.begin(), endresults.end());
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
