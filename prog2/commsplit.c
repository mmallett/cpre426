#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char ** argv)
{
	MPI_Init(&argc, &argv);

	int first_rank;

	MPI_Comm_rank(MPI_COMM_WORLD, &first_rank);

	MPI_Comm comms[4];

	MPI_Comm_dup(MPI_COMM_WORLD, &comms[0]);
	
	int i;
	for(i=0; i<3; i++)
	{

		MPI_Barrier(MPI_COMM_WORLD);

		int size;
		MPI_Comm_size(comms[i], &size);
		int rank;
		MPI_Comm_rank(comms[i], &rank);

		//printf("processor %d says: rank %d comm size %d\n", first_rank, rank, size);*/
		
		if(rank >= size/2)
		{
			int new_rank = rank - size/2;
			MPI_Comm_split(
				comms[i],
				2,
				new_rank,
				&comms[i+1]
			);
		}
		else
		{
			MPI_Comm_split(
				comms[i],
				1,
				rank,
				&comms[i+1]
			);
		}

		//int size;
                MPI_Comm_size(comms[i+1], &size);
                //int rank;
                MPI_Comm_rank(comms[i+1], &rank);

                printf("processor %d says: rank %d comm size %d\n", first_rank, rank, size);
	}

	/*
	for(i=0; i<2; i++)
	{
		MPI_Barrier(*(comms[0])); //sync at each iteration

		int rank, size;
		MPI_Comm_size(*(comms[i]), &size);
		MPI_Comm_rank(*(comms[i]), &rank);

		printf("processor %d says: rank %d comm size %d\n", first_rank, rank, size);
	}*/

	MPI_Finalize();
	return 0;
}



