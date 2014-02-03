/*
 * n-body.c
 *
 *  Created on: 28.01.2014
 *      Author: yogi, paul
 */

#include "n-body.h"

#define NUM_PARTICLES	15
#define CHUNK_SIZE	2
#define mpi_cnt(X) sizeof(particle) * X
//uncomment to calculate particle[x] and particle[y] with x=y
//otherwise particle[y] will temporarily replaced with EPSILON_P
//#define X_Y_CALC

MPI_Datatype mpi_datatype = MPI_INT;
MPI_Comm mpi_commring;

static const unsigned int MPI_NEW_DATA_TAG = 111;

//static const particle EPSILON_P = {0, 0, 0, 0, {0, 0, 0, 0}};		//short version
static const particle EPSILON_P = { .x = 0, .y = 0, .z = 0, .value = 0, .f = { .value = 0, .x = 0, .y = 0, .z = 0 } };

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

	int var_cs = CHUNK_SIZE;		//workaround for calculations in the define statement
	int current_buffer_wi = CHUNK_SIZE;
	int last_buffer_wi = current_buffer_wi;

	int mpi_rank, mpi_size;
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

	int left, right;
	MPI_Cart_shift(mpi_commring, 0, 1, &left, &right);

	particle* backup_locals;		//the locals are overwritten in the progress, if not all can be sent all together, we need a backup

	int strip_width = (NUM_PARTICLES) / mpi_size;
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

	//particle locals[strip_width];			//locals sind die partikel zu denen ein Prozess die Kräfte die auf sie wirken berechnen soll
	particle buffer_in[current_buffer_wi];			//buffer_in sind die Partikel die per MPI ankommen
	particle buffer_out[current_buffer_wi];    //buffer_out sind die Partikel die per MPI gesendet werden, mit buffer_out werden die Teilkräfte für locals berechnet
	//particle result[NUM_PARTICLES];		//In dieses Array werden die particle mit den fertigen kräften gespeichert

	memcpy(buffer_out, particles, sizeof(particle) * (current_buffer_wi));

	int transmissions;
	MPI_Request send_status;
	MPI_Request recv_status;
	//TODO fix: copy anstatt copyin, dafür copyin beim buffer_out
#pragma acc data copy (particles[0: strip_width ])
	for (transmissions = 0; transmissions < number_of_transmissions; ++transmissions) {
		if (number_of_transmissions - 2 > transmissions) {
			int temp = current_buffer_wi;
			if (odd_locals == true && number_of_transmissions - mpi_size - 1 == transmissions) {    //last round of transmissions, properly smaller chunks
				temp = last_buffer_wi;
			}
			MPI_Isend(buffer_out, mpi_cnt(temp), mpi_datatype, right, MPI_NEW_DATA_TAG, mpi_commring, &send_status);
			MPI_Irecv(buffer_in, mpi_cnt(temp), mpi_datatype, left, MPI_NEW_DATA_TAG, mpi_commring, &recv_status);
		}

		int offset_locals = transmissions / mpi_size;
		int x_offset = (offset_locals > 0) ? 1 : 0;		//offset factor, your locals are last in one transmission cycle
		//pos of incoming particles
		int x = ((mpi_rank + transmissions + x_offset) * strip_width) % NUM_PARTICLES + (var_cs * offset_locals);    // + j
		//your locals
		int y = mpi_rank * var_cs;		// + i
		//gehe alle local partikel durch und berechne die auf sie wirkenden Kräfte
#pragma acc data copyin (buffer_out [0:current_buffer_wi], x, y)
		for (int i = 0; i < strip_width; i++) {
#ifndef X_Y_CALC
			if (x >= y + i && x + current_buffer_wi <= y + i) {
				buffer_out[x % current_buffer_wi] = EPSILON_P;
			}
#endif

			//berechne die kraft die auf partikel i wirkt
#pragma acc parallel loop
			for (int j = 0; j < current_buffer_wi; j++) {
/*#ifdef CHECKER
				//calc if x>y and x+y odd or x<y and x+y even
				int x_temp = x + j;
				int y_temp = y + i;
				if (x_temp == y_temp || (x_temp > y_temp && (x_temp + y_temp) % 2 == 0) || (x_temp < y_temp && (x_temp + y_temp) % 2 == 1)) {
					continue;
				}
#endif*/
				particles[i].f.x += buffer_out[j].f.x + buffer_out[j].f.y + buffer_out[j].f.z;
				particles[i].f.y += buffer_out[j].f.x + buffer_out[j].f.y + buffer_out[j].f.z;
				particles[i].f.z += buffer_out[j].f.x + buffer_out[j].f.y + buffer_out[j].f.z;
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

/**
 * Erstellt eine Menge von Particles, kann nachher durch Dateieingabe ersetzt werden
 * @param particles
 */
void create_particles(particle* particles, int size_of_array) {
	int i = 0;

	for (i = 0; i < size_of_array; i++) {
		particle temp;
		temp.value = 1 + i;
		particles[i] = temp;
	}

}
int main(int argc, char *argv[]) {

	MPI_Init(&argc, &argv);
	int mpi_rank, mpi_size;
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
	/*
	 if (mpi_size < 2) {
	 printf("Too few mpi processes (%d)", mpi_size);
	 MPI_Finalize();
	 return -1;
	 }
	 */
	int periodic = 1;
	MPI_Cart_create( MPI_COMM_WORLD, 1, &mpi_size, &periodic, 1, &mpi_commring);

	setMpiDatatype();

	int strip_width = NUM_PARTICLES / mpi_size;
	particle locals[strip_width];
	particle* particles;
	if (mpi_rank == 0) {
		//array aller vorhandener Partikel
		particles = malloc(sizeof(particle) * NUM_PARTICLES);
		create_particles(particles, NUM_PARTICLES);
	}
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
		printf("particles: %d, chunksize: %d, Zeit: %f \n", NUM_PARTICLES, CHUNK_SIZE, time_spent);
		free(particles);
	}

	MPI_Finalize();
	return 0;
}

