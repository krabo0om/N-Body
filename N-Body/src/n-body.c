/*
 * n-body.c
 *      Author: yogi, paul
 */

#include "n-body.h"

static const unsigned int MPI_NEW_DATA_TAG = 111;

void nbody_compute(particle particles[]) {

	int var_cs = CHUNK_SIZE;		//workaround for calculations in the define statement
	int current_buffer_wi = CHUNK_SIZE;		//the current width of the buffer, can be smaller in the last transmission
	int last_buffer_wi = current_buffer_wi;

	int mpi_rank, mpi_size;
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

	int left, right;
	MPI_Cart_shift(nbody_mpi_commring, 0, 1, &left, &right);

	particle* backup_locals;		//the locals are overwritten in the progress, if not all can be sent all together, we need a backup

	int strip_width = (NUM_PARTICLES) / mpi_size;	//number of particles the process calculates, they are called locals
	bool odd_locals = false;	//true = last chunk of locals is smaller than chunk size, implicates more locals than chunk size
	if (strip_width > var_cs) {
		odd_locals = true;
		last_buffer_wi = strip_width % var_cs;
		if (last_buffer_wi == 0) {				//strip_width * k = chunk_size, but we assumed strip_width * k + last_buf_wi = chunk_size
			last_buffer_wi = current_buffer_wi;
		}
		long size = sizeof(particle) * (strip_width - var_cs);
		backup_locals = malloc(size);
		memcpy(backup_locals, &particles[var_cs], size);    //backup to locals which will be sent later
	}

	if (strip_width < var_cs) {				//don't have enough locals to fill up the buffers
		current_buffer_wi = strip_width;	//decrease buffer size
	}

	int number_of_transmissions = strip_width / current_buffer_wi;    //for locals on one node
	if (strip_width % current_buffer_wi != 0) {    //if s_w > c_b_w then / truncates the remainder, so need to add a transmission
		number_of_transmissions++;
	}
	number_of_transmissions *= mpi_size;		//for all nodes

	particle buffer_in[current_buffer_wi];			//buffer_in contains the particles from the left node
	particle buffer_out[current_buffer_wi];    //buffer_out contains the particles which will be sent to the right node

	memcpy(buffer_out, particles, sizeof(particle) * (current_buffer_wi));

#ifdef _OPENACC
	int num_dev = acc_get_num_devices(acc_device_nvidia);
	int id = mpi_rank % num_dev;
	acc_set_device_num(id, acc_device_nvidia );		//select one of the gpu's
#endif

	int transmissions;
	MPI_Request send_status;
	MPI_Request recv_status;
#pragma acc data copy (particles[0: strip_width ])
	for (transmissions = 0; transmissions < number_of_transmissions; ++transmissions) {
		if (number_of_transmissions - 2 > transmissions) {
			int temp = current_buffer_wi;
			if (odd_locals == true && number_of_transmissions - mpi_size - 1 == transmissions) {    //last round of transmissions, properly smaller chunks
				temp = last_buffer_wi;
			}
			MPI_Isend(buffer_out, mpi_cnt(temp), nbody_mpi_datatype, right, MPI_NEW_DATA_TAG, nbody_mpi_commring, &send_status);
			MPI_Irecv(buffer_in, mpi_cnt(temp), nbody_mpi_datatype, left, MPI_NEW_DATA_TAG, nbody_mpi_commring, &recv_status);
		}

		int offset_locals = transmissions / mpi_size;
		int x_offset = (offset_locals > 0) ? 1 : 0;		//offset factor, the locals are the last ones in one transmission complete cycle
		//pos of incoming particles
		int x = ((mpi_rank + transmissions + x_offset) * strip_width) % NUM_PARTICLES + (var_cs * offset_locals);    // + j
		//your locals
		int y = mpi_rank * var_cs;		// + i
		//calculate stuff on the locals
#pragma acc data copyin (buffer_out [0:current_buffer_wi], x, y)
		for (int i = 0; i < strip_width; i++) {
#ifndef X_Y_CALC
			if (x >= y + i && x + current_buffer_wi <= y + i) {
				buffer_out[x % current_buffer_wi] = EPSILON_P;
			}
#endif

#pragma omp parallel for
#pragma acc parallel loop
			for (int j = 0; j < current_buffer_wi; j++) {
				particles[i] = calculate(buffer_out[j], particles[i]);
			}

		}
		if (number_of_transmissions - 2 > transmissions) {
			if (odd_locals == true && number_of_transmissions - mpi_size - 1 == transmissions) {
				current_buffer_wi = last_buffer_wi;
			}

			//wait for comm to finish
			MPI_Status temp;
			MPI_Wait(&send_status, &temp);
			MPI_Wait(&recv_status, &temp);
			//butter_out = buffer_in
			if (odd_locals == true && (transmissions + 1) % mpi_size == 0) {
				//if we have to send some more locals and the current sending loop is complete
				int offset = transmissions / mpi_size * CHUNK_SIZE;    //no trans+1 because the first offset is 0, otherwise ((trans+1)/m_s)-1
				memcpy(buffer_out, &backup_locals[offset], sizeof(particle) * current_buffer_wi);
			} else {
				memcpy(buffer_out, buffer_in, sizeof(particle) * current_buffer_wi);
			}
		}
	}
}

void nbody_init(){
	int ti; char** tc;
	MPI_Init(&ti, &tc);

	int mpi_rank, mpi_size;
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
	int periodic = 1;
	MPI_Cart_create( MPI_COMM_WORLD, 1, &mpi_size, &periodic, 1, &nbody_mpi_commring);

	nbody_mpi_datatype = MPI_BYTE;
}

void nbody_finalize(){
	MPI_Finalize();
}
