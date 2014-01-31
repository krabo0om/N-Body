/*
 * n-body.c
 *
 *  Created on: 28.01.2014
 *      Author: yogi, paul
 */

#include "n-body.h"

//#define SIZE_LOCAL 20000
#define NUM_PARTICLES 20000
#define CHUNK_SIZE 5000

#define mpi_cnt(X) sizeof(particle) * X

MPI_Datatype mpi_datatype = MPI_INT;
MPI_Comm mpi_commring;

static const unsigned int MPI_NEW_DATA_TAG = 111;

force calculate_force(force f1, force f2) {
	f1.x = 2.01 * f2.x + f2.y + f2.z;
	f1.y = f2.x + 2.01 * f2.y + f2.z;
	f1.z = f2.x + f2.y + 2.01 * f2.z;

	return f1;
}

void setMpiDatatype() {
	//force
	int blockl[1] = { 4 };
	MPI_Datatype types[1] = { MPI_FLOAT };
	MPI_Aint offsets[1];
	offsets[0] = 0;
	MPI_Datatype temp_force;

	MPI_Type_create_struct(1, blockl, offsets, types, &temp_force);
	MPI_Type_commit(&temp_force);

	//particle
//	int blockl_p[5] = { 1, 1, 1, 1 ,1};
//	MPI_Datatype types_p[5] = { MPI_FLOAT, MPI_FLOAT, MPI_FLOAT, MPI_FLOAT, temp_force};
//	MPI_Aint offsets_p[5];
//	offsets[0] = offsetof(particle, x);
//	offsets[1] = offsetof(particle, y);
//	offsets[2] = offsetof(particle, z);
//	offsets[3] = offsetof(particle, value);
//	offsets[4] = offsetof(particle, f);

	int blockl_p[2] = { 4, 1 };
	MPI_Datatype types_p[2] = { MPI_FLOAT, temp_force };
	MPI_Aint offsets_p[2], extend, lb;
	MPI_Type_get_extent(temp_force, &lb, &extend);
	offsets_p[0] = 0;
	offsets_p[1] = 4 * extend;

	MPI_Type_create_struct(2, blockl_p, offsets_p, types_p, &mpi_datatype);
	MPI_Type_commit(&mpi_datatype);

	mpi_datatype = MPI_BYTE;
}

/**
 * Die Compute Funktion berechnet in einem Prozess jeweils komplett die auf eine Menge von zugewiesenen Partikeln (locals)
 * wirkenden Kräfte
 * @return
 */

void compute(particle particles[]) {

	double time_spent;

	int mpi_rank, mpi_size;
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

	int left, right;
	MPI_Cart_shift(mpi_commring, 0, 1, &left, &right);

	int strip_width = NUM_PARTICLES / mpi_size;
	int number_of_transmissions = NUM_PARTICLES / CHUNK_SIZE;
	bool more_locals = false;    //there are more locals than chunk_size
	if (strip_width > CHUNK_SIZE) {
		more_locals = true;
//		buffer_width = strip_width;
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
	memcpy(buffer_out, particles, sizeof(particle) * CHUNK_SIZE);

	int transmissions;
	MPI_Request send_status;
	MPI_Request recv_status;
	for (transmissions = 0; transmissions < number_of_transmissions; ++transmissions) {
		if (number_of_transmissions - 2 > transmissions) {
			MPI_Isend(buffer_out, mpi_cnt(CHUNK_SIZE), mpi_datatype, right, MPI_NEW_DATA_TAG, mpi_commring, &send_status);
			MPI_Irecv(buffer_in, mpi_cnt(CHUNK_SIZE), mpi_datatype, left, MPI_NEW_DATA_TAG, mpi_commring, &recv_status);
		}

		//gehe alle local partikel durch und berechne die auf sie wirkenden Kräfte
		int i = 0;
		int j = 0;
		particle* actual_particle;
#pragma acc data copy (particles[0: strip_width ], buffer_out [0:CHUNK_SIZE], actual_particle[0:1])
		for (i = 0; i < strip_width; i++) {
			actual_particle = &particles[i];
			//berechne die kraft die auf partikel i wirkt
#pragma acc parallel loop
			for (j = 0; j < CHUNK_SIZE; j++) {
//				actual_particle->f.x += buffer_out[j].f.x + buffer_out[j].f.y + buffer_out[j].f.z;
//				actual_particle->f.y += buffer_out[j].f.x + buffer_out[j].f.y + buffer_out[j].f.z;
//				actual_particle->f.z += buffer_out[j].f.x + buffer_out[j].f.y + buffer_out[j].f.z;
				actual_particle->f.z = 17;
			}
		}

		if (number_of_transmissions - 2 > transmissions) {
			//wait for comm to finish
			MPI_Status temp;
			MPI_Wait(&send_status, &temp);
			MPI_Wait(&recv_status, &temp);
			//free buffer_in, store data for next computation phase
			//if we have to send some more locals and the current sending loop is complete
			if (more_locals == true && (transmissions + 1) % mpi_size == 0) {
				int offset = (transmissions + 1) / mpi_size * CHUNK_SIZE;
				memcpy(buffer_out, &particles[offset], sizeof(particle) * CHUNK_SIZE);
			} else {
				memcpy(buffer_out, buffer_in, sizeof(particle) * CHUNK_SIZE);
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

	printf("test\n");
	fflush(stdout);

	if (mpi_size < 2) {
		printf("Too few mpi processes (%d)", mpi_size);
		MPI_Finalize();
		return -1;
	}

	int periodic = 1;
	MPI_Cart_create( MPI_COMM_WORLD, 1, &mpi_size, &periodic, 1, &mpi_commring);

	setMpiDatatype();

//	mpi_size = 4;
	int strip_width = NUM_PARTICLES / mpi_size;
	particle locals[strip_width];
	particle* particles;
	if (mpi_rank == 0) {
		//array alle vorhandenen Partikel
		particles = malloc(sizeof(particle) * NUM_PARTICLES);
		create_particles(particles, NUM_PARTICLES);
//		locals[0].f.value=1337;
//		MPI_Send(locals, strip_width * sizeof(particle), mpi_datatype, 1, 0, mpi_commring);
//		printf("gesendet\n");
	}
//	if (mpi_rank == 1){
//		MPI_Status temp;
//		MPI_Recv(locals, strip_width * sizeof(particle), mpi_datatype, 0, 0, mpi_commring, &temp);
//		printf("empfangen\n");
//		printf("%d: particle.value = %f\n", mpi_rank, locals[0].f.value);
//	}
//
//
	MPI_Scatter(particles, mpi_cnt(strip_width), mpi_datatype, locals, mpi_cnt(strip_width), mpi_datatype, 0, mpi_commring);

	//übergebe alle particle an compute function die nbody berechnet und erhalte result particle mit forces
	clock_t begin, end;
	if (mpi_rank == 0) {
		begin = clock();
	}

	compute(locals);

	end = clock();
	MPI_Gather(locals, mpi_cnt(strip_width), mpi_datatype, particles, mpi_cnt(strip_width), mpi_datatype, 0, mpi_commring);

	if (mpi_rank == 0) {
		double time_spent = (double) (end - begin) / CLOCKS_PER_SEC;
		printf("particles: %d Zeit: %f \n", NUM_PARTICLES, time_spent);
		free(particles);
	}

//	MPI_Type_free(&mpi_datatype);
	MPI_Finalize();

	/*
	 for(i =0; i<NUM_PARTICLES;i++)
	 {
	 printf(" %d ", particles[i].value);
	 }
	 */

	return 0;
}

