#include "life.h"

int main(int argc, char ** argv){
		
	MPI_Init(&argc, &argv);

	//get command line arguments //CHANGE TO MATCH ./life in.file out.file eventually
	if(argc != 3){
			printf("USAGE: lifegrid m n\n");
			MPI_Finalize();
			exit(EXIT_FAILURE);
	}
	sscanf(argv[1], "%d", &m);
	sscanf(argv[2], "%d", &n);

	//in_file = (char*) malloc(strlen(argv[3]) * sizeof(char));
	//out_file = (char*) malloc(strlen(argv[4]) * sizeof(char));

	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	
	comm_grid_dims[0] = comm_grid_dims[1] = sqrt(world_size);

	int periods[] = {0,0};
	int reorder = 0;

	MPI_Cart_create(
			MPI_COMM_WORLD,
			DIMENSIONS,
			comm_grid_dims,
			periods,
			reorder,
			&grid_comm);
			
	int my_coords[2];
	MPI_Cart_get(
		grid_comm,
		DIMENSIONS,
		comm_grid_dims,
		periods,
		my_coords
	);

	//initialize a grid with some crap in it
	//over commit memory, provide a padding of one row/column extra
	//around the outside of the array
	current_generation_grid = (int*) calloc((m+2) * (n+2) , sizeof(int));
	int i;
	int write_loc = n + 1; //skip to end of first row
	for(i=0; i < m*n; i++){
		if(i % n == 0){
			write_loc += 2;
		}
		current_generation_grid[write_loc++] = i + world_rank * 100;
	}

	//REPLACE WITH FILE READ EVENTUALLY

	next_generation_grid = (int*) calloc((m+2) * (n+2), sizeof(int));
	
	create_up_communication();
	create_down_communication();
	init_column_t();
	create_left_communication();
	create_right_communication();
	
	int current_generation;
	for(current_generation = 0; current_generation<generations; current_generation++){
		simulate_generation();
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

void simulate_generation(){
	MPI_Startall(vertical_req_index, vertical_reqs);
	int i, loc = n+1;
	for(i = 0; i < m * n; i++){
		if(i % n == 0){
			loc += 2;
		}
		/*next_generation_grid[loc] = (current_generation_grid[loc] % 1000)
			+ ((current_generation + 1) * 1000);*/
		next_generation_grid[loc] = evolve(loc++);
		//eventually, need to exclude a 2 layer border
	}
	MPI_Waitall(vertical_req_index, vertical_reqs, MPI_STATUSES_IGNORE);

	MPI_Startall(horizontal_req_index, horizontal_reqs);
	MPI_Waitall(horizontal_req_index, horizontal_reqs, MPI_STATUSES_IGNORE);

	for(i = 0; i <(m+2); i++){
		current_generation_grid[i * (n+2)] = recv_right_buffer[i];
		current_generation_grid[(n+1) + i * (n+2)] = recv_left_buffer[i];
	}
	//now have neighbor dependencies, can calculate remaining cells
	MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);
	MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);
	MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);
	MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);
	MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);
	if(world_rank == 0) printf("ROUND %d:\n", zi);
	
	for(i=0; i<world_size; i++){
		if(world_rank == i){
				int j;
				for(j=0; j < (m+2) * (n+2); j++){
						if(j%(n+2) == 0) printf("\n");
						printf("%4d ", current_generation_grid[j]);
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
	
	memcpy(current_generation_grid, next_generation_grid, (m+2) * (n+2) * sizeof(int));
}

int evolve(int index){
	int alive_cells = 0;
	//no boundary checks needed, will always have the outer 'buffer' to prevent out of bounds
	//up
	alive_cells += current_generation_grid[index - I_CONSTANT];
	//down
	alive_cells += current_generation_grid[index + I_CONSTANT];
	//left
	alive_cells += current_generation_grid[index - 1];
	//right
	alive_cells += current_generation_grid[index + 1];
	//up left
	alive_cells += current_generation_grid[index - I_CONSTANT - 1];
	//up right
	alive_cells += current_generation_grid[index - I_CONSTANT + 1];
	//down left
	alive_cells += current_generation_grid[index + I_CONSTANT - 1];
	//down right
	alive_cells += current_generation_grid[index + I_CONSTANT + 1];

	return alive_cells == 3;	
}

int is_valid_coord(int * coords){
	return coords[0] >= 0 && coords[0] < comm_grid dims[0]
		&& coords[1] >= 0 && coords[1] < comm_grid_dims[1];
}
	
void create_up_communication(){

	int send_to_coords[2];
	int send_to_rank;
	
	int recv_from_coords[2];
	int recv_from_rank;

	send_to_coords[0] = my_coords[0] - 1;
	send_to_coords[1] = my_coords[1];
	
	recv_from_coords[0] = my_coords[0] + 1;
	recv_from_coords[1] = my_coords[1];
	
	if(is_valid_coord(send_to_coords)){
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
	
	if(is_valid_coord(recv_from_coords)){
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
}
	
void create_down_communication(){

	int send_to_coords[2];
	int send_to_rank;
	
	int recv_from_coords[2];
	int recv_from_rank;

	send_to_coords[0] = my_coords[0] + 1;
	send_to_coords[1] = my_coords[1];
	
	recv_from_coords[0] = my_coords[0] - 1;
	recv_from_coords[1] = my_coords[1];
	
	if(is_valid_coord(send_to_coords){
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
	
	if(is_valid_coord(recv_from_coords)){
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
}

void init_column_t(){
	MPI_Type_vector(
		m+2,
		1,
		n+2,
		MPI_INT,
		&column_t
	);
	MPI_Type_commit(&column_t);
}
	
void create_left_communication(){

	int send_to_coords[2];
	int send_to_rank;
	
	int recv_from_coords[2];
	int recv_from_rank;
	
	send_to_coords[0] = my_coords[0];
	send_to_coords[1] = my_coords[1] - 1;
	
	recv_from_coords[0] = my_coords[0];
	recv_from_coords[1] = my_coords[1] + 1;
	
	if(is_valid_coord(send_to_coords)){
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
	
	recv_left_buffer = (int*) calloc(m+2, sizeof(int)); //initialize to 0
	if(is_valid_coord(recv_from_coords)){
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
}
	
void create_right_communication(){

	int send_to_coords[2];
	int send_to_rank;
	
	int recv_from_coords[2];
	int recv_from_rank;

	send_to_coords[0] = my_coords[0];
	send_to_coords[1] = my_coords[1] + 1;
	
	recv_from_coords[0] = my_coords[0];
	recv_from_coords[1] = my_coords[1] - 1;
	
	if(is_valid_coord(send_to_coords)){
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
	
	recv_right_buffer = (int*) calloc(m+2, sizeof(int)); //initialize to 0
	if(is_valid_coord(recv_from_coords)){
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
}

gather_results(){
	
}

output(){

}
