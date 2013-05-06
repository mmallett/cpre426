#include <stdio.h>
#include <stdlib.h>

#include <mpi.h>

#define DIMENSIONS 2

int main(int argc, char ** argv){
		
	MPI_Init(&argc, &argv);

	int n,m;

	//get command line arguments
	if(argc != 3){ //extend later to include in, out files
			printf("USAGE: lifegrid m n\n");
			MPI_Finalize();
			exit(EXIT_FAILURE);
	}
	sscanf(argv[1], "%d", &m);
	sscanf(argv[2], "%d", &n);

	int world_size, world_rank;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	MPI_Comm grid_comm;
	
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
			
	int my_coords[2];
	MPI_Cart_get(
		grid_comm,
		DIMENSIONS,
		dims,
		periods,
		my_coords
	);

	//initialize a grid with some crap in it
	//over commit memory, provide a padding of one row/column extra
	//around the outside of the array
	int * grid = (int*) calloc((m+2) * (n+2) , sizeof(int));
	int i;
	int write_loc = n + 1; //skip to end of first row
	for(i=0; i < m*n; i++){
		if(i % n == 0){
			write_loc += 2;
		}
		grid[write_loc++] = i + world_rank * 100;
	}

	//make a 'working buffer'

	int * grid2 = (int*) calloc((m+2) * (n+2), sizeof(int));
	int * swap;
	//memcpy(grid2, grid, (m+2) * (n+2) * sizeof(int));
	
	MPI_Request vertical_reqs[4]; //send & recv up, down
	int vertical_req_index = 0;
	
	MPI_Request horizontal_reqs[4]; //send & recv left, right
	int horizontal_req_index = 0;
	
	#define TAG 0xC0FFEE
	
	int send_to_coords[2];
	int send_to_rank;
	
	int recv_from_coords[2];
	int recv_from_rank;
	
	//UP
	send_to_coords[0] = my_coords[0] - 1;
	send_to_coords[1] = my_coords[1];
	
	recv_from_coords[0] = my_coords[0] + 1;
	recv_from_coords[1] = my_coords[1];
	
	if(send_to_coords[0] >= 0 && send_to_coords[0] < dims[0]
		&& send_to_coords[1] >= 0 && send_to_coords[1] < dims[1]){
		MPI_Cart_rank(
			grid_comm,
			send_to_coords,
			&send_to_rank
		);
		MPI_Send_init(
			&grid[n+3],
			n,
			MPI_INT,
			send_to_rank,
			TAG,
			grid_comm,
			&vertical_reqs[vertical_req_index++]
		);
	}
	
	if(recv_from_coords[0] >= 0 && recv_from_coords[0] < dims[0]
		&& recv_from_coords[1] >= 0 && recv_from_coords[1] < dims[1]){
		MPI_Cart_rank(
			grid_comm,
			recv_from_coords,
			&recv_from_rank
		);
		MPI_Recv_init(
			&grid[((n+2)*(m+2)) - (n+1)],
			n,
			MPI_INT,
			recv_from_rank,
			TAG,
			grid_comm,
			&vertical_reqs[vertical_req_index++]
		);
	}
	
	//DOWN
	send_to_coords[0] = my_coords[0] + 1;
	send_to_coords[1] = my_coords[1];
	
	recv_from_coords[0] = my_coords[0] - 1;
	recv_from_coords[1] = my_coords[1];
	
	if(send_to_coords[0] >= 0 && send_to_coords[0] < dims[0]
		&& send_to_coords[1] >= 0 && send_to_coords[1] < dims[1]){
		MPI_Cart_rank(
			grid_comm,
			send_to_coords,
			&send_to_rank
		);
		MPI_Send_init(
			&grid[((n+2)*(m+2)) - (2 * n + 3)],//n+3],
			n,
			MPI_INT,
			send_to_rank,
			TAG,
			grid_comm,
			&vertical_reqs[vertical_req_index++]
		);
	}
	
	if(recv_from_coords[0] >= 0 && recv_from_coords[0] < dims[0]
		&& recv_from_coords[1] >= 0 && recv_from_coords[1] < dims[1]){
		MPI_Cart_rank(
			grid_comm,
			recv_from_coords,
			&recv_from_rank
		);
		MPI_Recv_init(
			&grid[1],
			n,
			MPI_INT,
			recv_from_rank,
			TAG,
			grid_comm,
			&vertical_reqs[vertical_req_index++]
		);
	}
	
	//LEFT
	MPI_Datatype column_t;
	MPI_Type_vector(
		m+2,
		1,
		n+2,
		MPI_INT,
		&column_t
	);
	MPI_Type_commit(&column_t);
	
	send_to_coords[0] = my_coords[0];
	send_to_coords[1] = my_coords[1] - 1;
	
	recv_from_coords[0] = my_coords[0];
	recv_from_coords[1] = my_coords[1] + 1;
	
	if(send_to_coords[0] >= 0 && send_to_coords[0] < dims[0]
		&& send_to_coords[1] >= 0 && send_to_coords[1] < dims[1]){
		MPI_Cart_rank(
			grid_comm,
			send_to_coords,
			&send_to_rank
		);
		MPI_Send_init(
			&grid[1],
			1,
			column_t,
			send_to_rank,
			TAG,
			grid_comm,
			&horizontal_reqs[horizontal_req_index++]
		);
	}
	
	int * recv_left_buffer = (int*) calloc(m+2, sizeof(int)); //initialize to 0
	if(recv_from_coords[0] >= 0 && recv_from_coords[0] < dims[0]
		&& recv_from_coords[1] >= 0 && recv_from_coords[1] < dims[1]){
		MPI_Cart_rank(
			grid_comm,
			recv_from_coords,
			&recv_from_rank
		);
		MPI_Recv_init(
			recv_left_buffer,
			m + 2,
			MPI_INT,
			recv_from_rank,
			TAG,
			grid_comm,
			&horizontal_reqs[horizontal_req_index++]
		);
	}
	
	//RIGHT
	send_to_coords[0] = my_coords[0];
	send_to_coords[1] = my_coords[1] + 1;
	
	recv_from_coords[0] = my_coords[0];
	recv_from_coords[1] = my_coords[1] - 1;
	
	if(send_to_coords[0] >= 0 && send_to_coords[0] < dims[0]
		&& send_to_coords[1] >= 0 && send_to_coords[1] < dims[1]){
		MPI_Cart_rank(
			grid_comm,
			send_to_coords,
			&send_to_rank
		);
		MPI_Send_init(
			&grid[n],
			1,
			column_t,
			send_to_rank,
			TAG,
			grid_comm,
			&horizontal_reqs[horizontal_req_index++]
		);
	}
	
	int * recv_right_buffer = (int*) calloc(m+2, sizeof(int)); //initialize to 0
	if(recv_from_coords[0] >= 0 && recv_from_coords[0] < dims[0]
		&& recv_from_coords[1] >= 0 && recv_from_coords[1] < dims[1]){
		MPI_Cart_rank(
			grid_comm,
			recv_from_coords,
			&recv_from_rank
		);
		MPI_Recv_init(
			recv_right_buffer,
			m + 2,
			MPI_INT,
			recv_from_rank,
			TAG,
			grid_comm,
			&horizontal_reqs[horizontal_req_index++]
		);
	}

	int zi;
	for(zi = 0; zi<4; zi++){
		MPI_Startall(vertical_req_index, vertical_reqs);
		//printf("[%d] vertical requests fired\n", world_rank);
		int zj, loc = n+1;
		for(zj = 0; zj < m * n; zj++){
			if(zj % n == 0){
				loc += 2;
			}
			grid2[loc] = (grid[loc] % 1000) + ((zi + 1) * 1000);
			loc++;
		}
		//printf("[%d] local processing done\n", world_rank);
		MPI_Waitall(vertical_req_index, vertical_reqs, MPI_STATUSES_IGNORE);
		//printf("[%d] vertical requests complete\n", world_rank);
		MPI_Startall(horizontal_req_index, horizontal_reqs);
		//printf("[%d] horizontal requests fired\n", world_rank);
		MPI_Waitall(horizontal_req_index, horizontal_reqs, MPI_STATUSES_IGNORE);
		//printf("[%d] horizontal requests complete\n", world_rank);*/
		for(zj = 0; zj <(m+2); zj++){
			grid[zj * (n+2)] = recv_right_buffer[zj];
			grid[(n+1) + zj * (n+2)] = recv_left_buffer[zj];
		}
		MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);
		MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);
		MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);
		MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);
		MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);
		if(world_rank == 0) printf("ROUND %d:\n", zi);
		
		for(zj=0; zj<world_size; zj++){
			if(world_rank == zj){
					int k;
					for(k=0; k < (m+2) * (n+2); k++){
							if(k%(n+2) == 0) printf("\n");
							printf("%4d ", grid[k]);
					}
					printf("\n\n");
			}
			MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);
			MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);
			MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);
			MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);
			MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);
			MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);
        }
		
		//swap = grid;
		//grid = grid2;
		//grid2 = swap;
		memcpy(grid, grid2, (m+2) * (n+2) * sizeof(int));
		
	}

	//the game.. is over
	MPI_Datatype final_grid_t;
	MPI_Type_vector(
		m,
		n,
		n+2,
		MPI_INT,
		&final_grid_t
	);
	MPI_Type_commit(&final_grid_t);

	if(world_rank == 0){
		int ** result_grid = (int**) malloc(world_size * sizeof(int*));
		MPI_Request result_reqs[world_size-1];
		int z;
		for(z=1; z<world_size; z++){
			result_grid[z] = (int*) malloc(m * n * sizeof(int));
			MPI_Irecv(
				result_grid[z],
				m * n,
				MPI_INT,
				z,
				TAG,
				grid_comm,
				&result_reqs[z-1]
			);		
		}
		result_grid[0] = (int*) malloc(m * n * sizeof(int));
		for(z=0; z<m; z++){
			memcpy(&result_grid[0][n * z], &grid[(n + 3) + z * (n + 2)], n * sizeof(int));
		}
		MPI_Waitall(world_size-1, result_reqs, MPI_STATUSES_IGNORE);

		printf("FINAL BOARD:\n\n");
		for(z=0; z<dims[0]; z++){
			int y;
			int row;
			for(row = 0; row < m; row++){
				for(y=0; y<dims[1]; y++){
					int x;
					for(x=0; x<n; x++){
						printf("%4d ",
							result_grid[z * dims[1] + y][row * n + x]);
					}
				}		
				printf("\n");
			}
		}		
	}
	else{
		MPI_Request result_req;
		MPI_Isend(
			&grid[n+3],
			1,
			final_grid_t,
			0,
			TAG,
			grid_comm,
			&result_req
		);
		MPI_Wait(&result_req, MPI_STATUSES_IGNORE);
	}

        MPI_Finalize();

        return 0;

}
