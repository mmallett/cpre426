#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpi.h>

/*
Input Parameters
Acquired either through command line args, or from input file
*/
int m, n, generations;
char * in_file, * out_file;

/*
Game of life definitions
*/
#define DIMENSIONS 2

int * current_generation_grid;
int * next_generation_grid;

void simulate_generation();

#define I_CONSTANT (n+2)
int evolve(int index);

/*
MPI runtime environment values
*/
#define TAG 0xCOFFEE

int world_size;
int world_rank;
MPI_Comm grid_comm;

int comm_grid_dims[DIMENSIONS];

int is_valid_coord(int * coords);

/*
Helper function definitions
*/
MPI_Request vertical_reqs[4]; //only 4 possible: send/recv ^, send/recv v
int vertical_req_index;

MPI_Request * horizontal_reqs[4]; //only 4 possible: send/recv <-, send/recv ->
int horizontal_req_index;

void create_up_communication();

void create_down_communication();

MPI_Datatype column_t;
void init_column_t();

int * recv_left_buffer;
void create_left_communication();

int * recv_right_buffer;
void create_right_communication();

void gather_results();

void output();
