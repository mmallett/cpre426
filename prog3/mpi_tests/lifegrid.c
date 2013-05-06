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

	//figure out processor grid dimensions
	//if power of 2, 2 x p/2
	//if perfect square root(p) x root(p)
	//TODO THIS

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
	MPI_Cart_coords(
		grid_comm,
		world_rank,
		2,
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

	for(i=0; i<world_size; i++){
		if(world_rank == i){
			int j;
			for(j=0; j < (m+2) * (n+2); j++){
				if(j%(n+2) == 0) printf("\n");
				printf("%3d ", grid[j]);
			}
			printf("\n\n");
		}
		MPI_Barrier(grid_comm);
	}

	//make a 'working buffer'

	int * grid2 = (int*) malloc((m+2) * (n+2) * sizeof(int));
	memcpy(grid2, grid, (m+2) * (n+2) * sizeof(int));

	//lets practice ^ v < >
	int src_coords[2];
	int dest_coords[2];

	// UP
	//who will yo usend too?
	dest_coords[0] = my_coords[0] - 1;
	dest_coords[1] = my_coords[1];
	int send_to_this_rank;

	//who will you receive from?
	src_coords[0] = my_coords[0] + 1;
	src_coords[1] = my_coords[1];
	int recv_from_this_rank;

	//printf("[%d] (%d,%d) (%d,%d) (%d,%d)\n",
	//	world_rank, my_coords[0], my_coords[1], dest_coords[0], dest_coords[1],
	//	src_coords[0], src_coords[1]);

	//check if in coordinate boundaries, only works for p = 4 right now
	if(dest_coords[0] >= 0 && dest_coords[1] >= 0
		&& dest_coords[0] < 2 && dest_coords[1] < 2){
		MPI_Cart_rank(
			grid_comm,
			dest_coords,
			&send_to_this_rank
		);	
	}
	else{
		send_to_this_rank = -1;
	}
	
	//check if inside boundaries, only works for p = 4 right now
	if(src_coords[0] >= 0 && src_coords[1] >= 0
		&& src_coords[0] < 2 && src_coords[1] < 2){
		MPI_Cart_rank(
			grid_comm,
			src_coords,
			&recv_from_this_rank
		);
	}
	else{
		recv_from_this_rank = -1;
	}

	MPI_Request send_req;
	if(send_to_this_rank != -1){
		MPI_Isend(
			&grid[n+3],
			n,
			MPI_INT,
			send_to_this_rank,
			3,
			grid_comm,
			&send_req
		);
	}

	MPI_Request recv_req;
	if(recv_from_this_rank != -1){
		MPI_Irecv(
			&grid2[((n+2)*(m+2)) - (n+1)],
			n,
			MPI_INT,
			recv_from_this_rank,
			3,
			grid_comm,
			&recv_req
		);
	}

	MPI_Status status;
	if(send_to_this_rank != -1) MPI_Wait(&send_req, &status);
	if(recv_from_this_rank != -1) MPI_Wait(&recv_req, &status);

	MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm); //ffs this thing wouldn't sync

	if(world_rank == 0) printf("TOP ROWS SENT UP\n\n");

	for(i=0; i<world_size; i++){
        	if(world_rank == i){
                	int j;
                	for(j=0; j < (m+2) * (n+2); j++){
                                if(j%(n+2) == 0) printf("\n");
                                printf("%3d ", grid2[j]);
                        }
                        printf("\n\n");
                }
                MPI_Barrier(grid_comm);
        }

	//feeling lazy so we'll just do RIGHT
	
	//derive the column data type
	//utlimately, LEFT and RIGHT should be done after UP and DOWN are complete
	//this lets us send complete columns

	MPI_Datatype column_t;
	MPI_Type_vector(
		m+2, //m + 2 total rows allocated
		1, 
		n+2, //n + 2 is length of rows AKA number of columns, skip this many elements
		// to get back to the target column
		MPI_INT,
		&column_t
	);
	MPI_Type_commit(&column_t);

	dest_coords[0] = my_coords[0];
	dest_coords[1] = my_coords[1] + 1;

	src_coords[0] = my_coords[0];
	src_coords[1] = my_coords[1] - 1;
	
	 //check if in coordinate boundaries, only works for p = 4 right now
        if(dest_coords[0] >= 0 && dest_coords[1] >= 0
                && dest_coords[0] < 2 && dest_coords[1] < 2){
                MPI_Cart_rank(
                        grid_comm,
                        dest_coords,
                        &send_to_this_rank
                );
        }
        else{
                send_to_this_rank = -1;
        }

        //check if inside boundaries, only works for p = 4 right now
        if(src_coords[0] >= 0 && src_coords[1] >= 0
                && src_coords[0] < 2 && src_coords[1] < 2){
                MPI_Cart_rank(
                        grid_comm,
                        src_coords,
                        &recv_from_this_rank
                );
        }
        else{
                recv_from_this_rank = -1;
        }
	//printf("[%d] (%d,%d) (%d,%d) (%d,%d)\n",
        //      world_rank, my_coords[0], my_coords[1], dest_coords[0], dest_coords[1],
        //      src_coords[0], src_coords[1]);


	 //MPI_Request send_req;
	//send a column to the right
        if(send_to_this_rank != -1){
                MPI_Isend(
                        &grid2[n],
                        1,
                        column_t,
                        send_to_this_rank,
                        3,
                        grid_comm,
                        &send_req
                );
        }

	int recv_buf[m+2];
        //MPI_Request recv_req;
        if(recv_from_this_rank != -1){
                MPI_Irecv(
                        recv_buf,
                        m + 2,
                        MPI_INT,
                        recv_from_this_rank,
                        3,
                        grid_comm,
                        &recv_req
                );
        }

	 if(send_to_this_rank != -1) MPI_Wait(&send_req, &status);
        if(recv_from_this_rank != -1){
		MPI_Wait(&recv_req, &status);
		//set the received data in the grid
                for(i=0; i<(m+2); i++){
                        grid2[i * (n+2)] = recv_buf[i];
                }
	}

	MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm);MPI_Barrier(grid_comm); //sync issues
	//.........................................................................................
	//..................................................................................




	//lulz

        if(world_rank == 0) printf("RIGHT ROWS SENT RIGHT\n\n");

        for(i=0; i<world_size; i++){
                if(world_rank == i){
                        int j;
                        for(j=0; j < (m+2) * (n+2); j++){
                                if(j%(n+2) == 0) printf("\n");
                                printf("%3d ", grid2[j]);
                        }
                        printf("\n\n");
                }
                MPI_Barrier(grid_comm);
        }

	//printf("\n\n\nWOOOOOOOOOWOWOWOWOOWOOOOOOOOOOOOOOOOOOO");

        MPI_Finalize();

        return 0;

}
