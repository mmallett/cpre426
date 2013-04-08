#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define BIG 0x3FFFFFFF

int main(int argc, char ** argv)
{	
	MPI_Init(&argc, &argv);
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	int * data;
	int * send_data;
	int data_size;

	if(world_rank == 0)
	{	
		//open the file
		FILE * fp;
		fp = fopen("/home/mmallett/cpre426/prog2/in.txt", "r");

		if(fp == NULL)
		{
			perror("Could not open file");
			exit(EXIT_FAILURE);
		}
		
		int n;
		fscanf(fp, "%d\n", &n);
		printf("%d elements\n", n);	

		int i;
		data_size = n/world_size + (n%world_size > 0); //super nooblol
		int buff_size = world_size * data_size;
		send_data = (int*) malloc(buff_size *sizeof(int));

		//fill buffer with data from file		
		for(i=0; i<n; i++)
		{
			fscanf(fp, "%d\n", &send_data[i]);
			//printf("%d\n", send_data[i]);
		}
		//fill remainder of buffer, gives each processor same buffer size
		for(i=n; i<buff_size; i++)
		{
			send_data[i] = BIG;
		}
			
	}

	//broadcast size of incound data buffer
	MPI_Bcast
	(
		&data_size,
		1,
		MPI_INT,
		0,
		MPI_COMM_WORLD
	);

	data = (int*) malloc(data_size * sizeof(int));

	//scatter data among processors
	MPI_Scatter
	(
		send_data,
		data_size,
		MPI_INT,
		data,
		data_size,
		MPI_INT,
		0,
		MPI_COMM_WORLD
	);
	
	int i;
	for(i=0; i<data_size; i++)
	{
		printf("%d has %d\n", world_rank, data[i]);
	}	
	
	MPI_Finalize();
	return 0;
}
