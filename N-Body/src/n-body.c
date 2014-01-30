/*
 * n-body.c
 *
 *  Created on: 28.01.2014
 *      Author: yogi
 */

#include "n-body.h"

//#define SIZE_LOCAL 20000
#define NUM_PARTICLES 20000
#define CHUNK_SIZE 5000

MPI_Datatype mpi_datatype = MPI_INT;
MPI_Comm mpi_commring;

static const unsigned int MPI_NEW_DATA_TAG = 111;

struct force calculate_force(struct force f1, struct force f2) {

	return f1;
}

void setMpiDatatype() {
	int blockl[4] = { 1, 1, 1, 1 };
	MPI_Datatype types[4] = { MPI_INT, MPI_INT, MPI_INT, MPI_INT };
	MPI_Aint offsets[4];
	offsets[0] = offsetof(particle, x);
	offsets[1] = offsetof(particle, y);
	offsets[2] = offsetof(particle, z);
	offsets[3] = offsetof(particle, value);

	MPI_Type_create_struct(4, blockl, offsets, types, &mpi_datatype);
	MPI_Type_commit(&mpi_datatype);
}

/**
 * Die Compute Funktion berechnet in einem Prozess jeweils komplett die auf eine Menge von zugewiesenen Partikeln (locals)
 * wirkenden Kräfte
 * @return
 */

void compute(particle* particles) {

	double time_spent;

	int mpi_rank, mpi_size;
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

	int left, right;
	MPI_Cart_shift(mpi_commring, 0, 1, &left, &right);

	int strip_width = NUM_PARTICLES / mpi_size;
	int number_of_transmissions = mpi_size;
	int buffer_width = CHUNK_SIZE;    //the first part of data to copy from the locals list to the output buffer
	bool more_locals = false;    //there are more locals than chunk_size
	if (strip_width > CHUNK_SIZE) {
		more_locals = true;
		buffer_width = strip_width;
	}

	//particle locals[strip_width];			//locals sind die partikel zu denen ein Prozess die Kräfte die auf sie wirken berechnen soll
	particle buffer_in[CHUNK_SIZE];			//buffer_in sind die Partikel die per MPI ankommen
	particle buffer_out[CHUNK_SIZE];    //buffer_out sind die Partikel die per MPI gesendet werden, mit buffer_out werden die Teilkräfte für locals berechnet
	//particle result[NUM_PARTICLES];		//In dieses Array werden die particle mit den fertigen kräften gespeichert

	//Speicher reservieren für die struct arrays, nur notwendig wenn wir dynamische struct arrays brauchen
	/*
	 locals = (struct particle *) malloc(SIZE_LOCAL * sizeof(struct particle));
	 buffer_in = (struct particle *) malloc(CHUNK_SIZE * sizeof(struct particle));
	 buffer_out = (struct particle *) malloc(CHUNK_SIZE * sizeof(struct particle));
	 result = (struct particle *) malloc(NUM_PARTICLES * sizeof(struct particle));
	 */
	memcpy(buffer_out, particles, sizeof(particle) * buffer_width);

	int transmissions;
	MPI_Request send_status;
	MPI_Request recv_status;
	for (transmissions = 0; transmissions < number_of_transmissions + 1; ++transmissions) {

		if (number_of_transmissions - 2 > transmissions) {
			MPI_Isend(buffer_out, CHUNK_SIZE, mpi_datatype, right, MPI_NEW_DATA_TAG, mpi_commring, &send_status);
			MPI_Irecv(buffer_in, CHUNK_SIZE, mpi_datatype, left, MPI_NEW_DATA_TAG, mpi_commring, &recv_status);
		}

		//gehe alle local partikel durch und berechne die auf sie wirkenden Kräfte
		int i = 0;
		int j = 0;

		for (i = 0; i < strip_width; i++) {

			//berechne die kraft die auf partikel i wirkt
#pragma acc data copyin (particles[0: strip_width ], buffer_out [0:CHUNK_SIZE])
#pragma acc parallel loop
			for (j = 0; j < CHUNK_SIZE; j++) {
				particles[i].value += buffer_out[j].value;
			}
		}

		if (number_of_transmissions - 2 > transmissions) {
			//wait for comm to finish
			MPI_Status temp;
			MPI_Wait(&send_status, &temp);
			MPI_Wait(&recv_status, &temp);
		}

		if (number_of_transmissions - 1 > transmissions) {
			//free buffer_in, store data for next computation phase
			//if we have to send some more locals and the current sending loop is complete
			if (more_locals == true && (transmissions + 1) % mpi_size == 0) {
				int offset = (transmissions + 1) / mpi_size * CHUNK_SIZE;
				memcpy(buffer_out, &particles[offset], sizeof(particle) * buffer_width);
			} else {
				memcpy(buffer_out, buffer_in, sizeof(particle) * buffer_width);
			}
		}
	}

//zurückschreiben in ergebnisarray

}

/**
 * Erstellt eine Menge von Particles, kann nachher durch Dateieingabe ersetzt werden
 * @param particles
 */
void create_particles(particle* particles, int size_of_array) {
	int i = 0;

	/*
	 * Array wird nach folgendem Schema initialisiert:
	 * 1. Element: val=1
	 * 2. Element: val=2
	 * 3. Element: val=3
	 */
	for (i = 0; i < size_of_array; i++) {
		particle temp;
		temp.value = i + 1;
		particles[i] = temp;
	}

}
int main(int argc, char *argv[]) {

	MPI_Init(&argc, &argv);
	int mpi_rank, mpi_size;
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

	if (mpi_size < 2){
		printf("Too few mpi processes (%d)", mpi_size);
		MPI_Finalize();
		return -1;
	}

	int periodic = 1;
	MPI_Cart_create( MPI_COMM_WORLD, 1, &mpi_size, &periodic, 1, &mpi_commring);

	setMpiDatatype();

	int strip_width = NUM_PARTICLES / mpi_size;
	particle locals[strip_width];
	particle* particles;
	if (mpi_rank == 0) {
		//array alle vorhandenen Partikel
		particles = malloc(sizeof(particle) * NUM_PARTICLES);
		create_particles(particles, mpi_size);
	}
	MPI_Scatter(particles, mpi_size, mpi_datatype, locals, mpi_size, mpi_datatype, 0, mpi_commring);

	//übergebe alle particle an compute function die nbody berechnet und erhalte result particle mit forces
	clock_t begin, end;
	if (mpi_rank == 0) {
		begin = clock();
	}

	compute(locals);

	MPI_Gather(locals, strip_width, mpi_datatype, particles, strip_width, mpi_datatype, 0, mpi_commring);

	if (mpi_rank == 0) {
		end = clock();
		double time_spent = (double) (end - begin) / CLOCKS_PER_SEC;
		printf("particles: %d Zeit: %f \n", NUM_PARTICLES, time_spent);
		free(particles);
	}

	//ergebnis ausgeben:
	int i = 0;

	MPI_Type_free(mpi_datatype);
	MPI_Finalize();

	/*
	 for(i =0; i<NUM_PARTICLES;i++)
	 {
	 printf(" %d ", particles[i].value);
	 }
	 */

	return 1337;
}

