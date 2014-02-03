#include "n-body.h"
#include <stdio.h>


int main(int argc, char *argv[]) {

	//calls also MPI_Init()
	nbody_init();

	int mpi_size, mpi_rank;
	//get my mpi number
	MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
	//get number of all mpi processes
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);

	//calculate, how many particles each node will get
	int strip_width = NUM_PARTICLES / mpi_size;

	//store my local particles
	particle locals[strip_width];

	//master (rank == 0) needs space for all particles, others not
	particle* particles;
	if (mpi_rank == 0) {
		particles = malloc(sizeof(particle) * NUM_PARTICLES);
		//read/create all particles
		nbody_create_particles(particles, NUM_PARTICLES);
	}
	//send a pack of particles to every mpi process
	//use mpi_cnt to determinate the number of bytes mpi needs to send, strip_width = number of particles to send to each process
	MPI_Scatter(particles, mpi_cnt(strip_width), nbody_mpi_datatype, locals, mpi_cnt(strip_width), nbody_mpi_datatype, 0, nbody_mpi_commring);

	//measure the time
	clock_t begin, end;
	if (mpi_rank == 0) {
		begin = clock();
	}

	//now every mpi_process calls nbody_compute with his own locals
	nbody_compute(locals);

	end = clock();
	//master collects the results from every node
	MPI_Gather(locals, mpi_cnt(strip_width), nbody_mpi_datatype, particles, mpi_cnt(strip_width), nbody_mpi_datatype, 0, nbody_mpi_commring);

	//measure time and print it
	if (mpi_rank == 0) {
		double time_spent = (double) (end - begin) / CLOCKS_PER_SEC;
		printf("particles: %d, chunksize: %d, duration: %f \n", NUM_PARTICLES, CHUNK_SIZE, time_spent);
		free(particles);
	}

	//calls also MPI_Finalize();
	nbody_finalize();
	return 0;

}
