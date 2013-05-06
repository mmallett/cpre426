#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <mpi.h>

int n, m;

#define DIMENSIONS 2

MPI_Comm grid_comm;

int main(int argc, char ** argv){

	MPI_Init(&argc, &argv);

	if(argc != 3){
		printf("USAGE: ./grid m n\n");
		MPI_Finalize();
		exit(EXIT_FAILURE);
	}
	sscanf(argv[1], "%d", &m);
	sscanf(argv[2], "%d", &n);

	//printf("%d x %d\n", m, n);

	int world_size, world_rank;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	int dims[2];
	dims[0] = dims[1] = sqrt(world_size);

	int periods[] = {0,0};
	int reorder = 0;

	MPI_Cart_create(
		MPI_COMM_WORLD,
		DIMENSIONS, 
		dims,
		periods,
		reorder,
		&grid_comm);

	if(world_rank == 0){
		int coords[2];
		int i;
		for(i=0; i<world_size; i++){
			MPI_Cart_coords(
				grid_comm,
				i,
				DIMENSIONS,
				coords);
			printf("[%d] (%d,%d)\n", i, coords[0], coords[1]);
		}		
	}

	MPI_Finalize();

	return 0;
}
