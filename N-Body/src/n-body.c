/*
 * n-body.c
 *
 *  Created on: 28.01.2014
 *      Author: yogi
 */


#include "n-body.h""
#define SIZE_LOCAL 20000
#define NUM_PARTICLES 20000
#define CHUNK_SIZE 20000


struct force calculate_force(struct force f1, struct force f2)
{
		f1.x =  2.01* f2.x + f2.y + f2.z;
		f1.y =  f2.x +  2.01*f2.y + f2.z;
		f1.z =  f2.x + f2.y +  2.01*f2.z;

return f1;
}


/**
 * Die Compute Funktion berechnet in einem Prozess jeweils komplett die auf eine Menge von zugewiesenen Partikeln (locals)
 * wirkenden Kräfte
 * @return
 */

void compute (struct particle particles []) {




	 	clock_t begin, end;
	 	double time_spent;

	struct particle locals [SIZE_LOCAL];			//locals sind die partikel zu denen ein Prozess die Kräfte die auf sie wirken berechnen soll
	struct particle buffer_in [CHUNK_SIZE];			//buffer_in sind die Partikel die per MPI ankommen
	struct particle buffer_out [CHUNK_SIZE];	//buffer_out sind die Partikel die per MPI gesendet werden, mit buffer_out werden die Teilkräfte für locals berechnet
	struct particle result [NUM_PARTICLES];		//In dieses Array werden die particle mit den fertigen kräften gespeichert


	begin = clock();
		 	/* here, do your time-consuming job */


	//hier MPI kram zum erhalten von buffer in, kopieren in buffer out, danach auch die size von buffer out festlegen, benutze jetzt dummy
	int buffer_out_size=20000;
	create_particles(buffer_out);



	//Hier werden jetzt die dummy daten erzeugt, dies kann dann von MPI ersetzt werden


	//Speicher reservieren für die struct arrays, nur notwendig wenn wir dynamische struct arrays brauchen
	/*
	locals = (struct particle *) malloc(SIZE_LOCAL * sizeof(struct particle));
	buffer_in = (struct particle *) malloc(CHUNK_SIZE * sizeof(struct particle));
	buffer_out = (struct particle *) malloc(CHUNK_SIZE * sizeof(struct particle));
	result = (struct particle *) malloc(NUM_PARTICLES * sizeof(struct particle));
	*/

	//gehe alle local partikel durch und berechne die auf sie wirkenden Kräfte
	int i= 0;
	int j =0;


	{

	#pragma acc data copy (particles[0: NUM_PARTICLES ], buffer_out [0:buffer_out_size])
	for( i =0; i<SIZE_LOCAL;i++)
	{

		struct particle* actual_particle = &particles[i];

		#pragma acc parallel loop
		for( j =0; j<buffer_out_size;j++)
		{

			actual_particle->f.x +=  buffer_out[j].f.x + buffer_out[j].f.y + buffer_out[j].f.z;
			actual_particle->f.y +=  buffer_out[j].f.x +  buffer_out[j].f.y + buffer_out[j].f.z;
			actual_particle->f.z +=  buffer_out[j].f.x + buffer_out[j].f.y +  buffer_out[j].f.z;



		}

	}
	}

				end = clock();
			 	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;

			 	printf("particles: %d Zeit: %f \n", NUM_PARTICLES, time_spent);


//zurückschreiben in ergebnisarray



}

/**
 * Erstellt eine Menge von Particles, kann nachher durch Dateieingabe ersetzt werden
 * @param particles
 */
void create_particles(struct particle particles [])
{
	int size_of_array =  NUM_PARTICLES;  //sizeof(*particles)/sizeof(struct particles) geht nicht bei structs irgendwie;
	int i =0;

	/*
	 * Array wird nach folgendem Schema initialisiert:
	 * 1. Element: val=1
	 * 2. Element: val=2
	 * 3. Element: val=3
	 */
	for (i=0; i < size_of_array; i++ )
	{
		struct particle temp;
		temp.value=i+1;
		particles[i]=temp;
	}




}
int main() {

	//array alle vorhandenen Partikel
	struct particle particles[NUM_PARTICLES];

	//ich versteh nicht warum dass jetzt call by reference ist obwohl es kein Pointer ist. vllt weil array immer ein pointer ist?
	create_particles(particles);

	//übergebe alle particle an compute function die nbody berechnet und erhalte result particle mit forces
	struct particle result[NUM_PARTICLES];
 	compute(particles);

 	//ergebnis ausgeben:
 	int i=0;
 	int b;
 	b= sizeof(struct particle);
 	printf ("structgröße %d",b );

 	printf("result:");

 	/*
 	for(i =0; i<NUM_PARTICLES;i++)
 	{
 		printf(" %d ", particles[i].value);
 	}
*/






return 1337;
}



