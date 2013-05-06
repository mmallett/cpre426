#include <stdio.h>
#include <stdlib.h>

#include <mpi.h>

int main(int argc, char ** argv){

	MPI_Init(&argc, &argv);

	int rank, size;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	int array[16];
	int i;
	for(i=0; i<16; i++){
		array[i] = i + (100 * rank);
	}

	MPI_Datatype column_t;
	MPI_Type_vector(
		4,
		1,
		4,
		MPI_INT,
		&column_t
	);
	MPI_Type_commit(&column_t);

	/*
	for(i=0; i<size; i++){
		if(rank == i){
			int j;
			for(j=0; j<16; j++){
				if(j%4 == 0) printf("\n");
				printf("%d ", array[j]);
			}
			printf("\n");
		}
	}*/

	for(i=size-1; i>0; i--){
		int buff[4];
		if(rank == 0){
			MPI_Status status;
			MPI_Recv(
				buff,
				4,
				MPI_INT,
				i,
				3,
				MPI_COMM_WORLD,
				&status
			);
			printf("%d %d %d %d\n", buff[0], buff[1], buff[2], buff[3]);
		}
		else if(rank == i){		
			MPI_Send(
				array,
				1,
				column_t,
				0,
				3,
				MPI_COMM_WORLD);
		}
	}

	MPI_Finalize();
	
	return 0;
}
