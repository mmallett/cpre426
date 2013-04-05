/********************************************************
Matt Mallett, Deven Starn
CprE 426 Programming Assignment 1

nqueens.c
Finds all solutions to the n-queens problem utilizing mpi

Sources
http://www.lam-mpi.org/tutorials/one-step/ezstart.php
	Skeleton master slave code for MPI
http://rosettacode.org/wiki/N-queens_problem#C
	Serial n-queens solution
********************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include <mpi.h>


//problem size definitions
static const int N = 8;
static const int K = 3;

//define tags for mpi communication
static const int WORKTAG = 1;
static const int DIETAG = 2;

//define methods for different processors
void master();
void slave();

//an outbuffer used by slaves
char * out;

//////// QUEUE DEFINITIONS

typedef struct node
{
	int data[K];
	struct node * next;
} node_t;

typedef struct queue
{
	int size;
	node_t * head;
	node_t * tail;
} queue_t;

queue_t * init_queue();

void enqueue(queue_t * q, int * data);

int is_empty(queue_t * q);

int * pop(queue_t *);

queue_t * jobs;

//////// END QUEUE
 
/*
invoked by the master to solve find all solutions
of the first k columns
solutions are put onto the jobs queue
*/
void solve_master(int col, int *hist)
{
	if (col == K) {
		enqueue(jobs, hist); //found a k length solution, throw it on the queue
		return;
	}
 
#	define attack(i, j) (hist[j] == i || abs(hist[j] - i) == col - j)
	for (int i = 0, j = 0; i < N; i++) { //for each row
		for (j = 0; j < col && !attack(i, j); j++); //if this piece is safe, move to the next column
		if (j < col) continue;
 
		hist[col] = i; //this row is safe for the current column
		solve_master(col + 1, hist);
	}
}

/*
invoked by slaves to complete a solution of length k
valid solutions are formatted, then put into an output buffer
*/
void solve_slave(int col, int *hist)
{
	if (col == N) { //found valid solution, output it
		char * tmp = (char*) calloc(strlen(out) + N + 4, sizeof(char));
		strcpy(tmp, out);
		free(out);
		out = tmp;
		for(int i=0; i<N; i++)
		{
			sprintf(out, "%s%d", out, hist[i]+1);
		}
		sprintf(out, "%s\n", out);
		return;
	}
 
#	define attack(i, j) (hist[j] == i || abs(hist[j] - i) == col - j)
	for (int i = 0, j = 0; i < N; i++) { //for each row
		for (j = 0; j < col && !attack(i, j); j++); //if this piece is safe, move to next column
		if (j < col) continue;
 
		hist[col] = i; // this row is safe for the current column
		solve_slave(col + 1, hist);
	}
}
 
int main(int argc, char **argv)
{	
	//mpi logistics
	MPI_Init(&argc, &argv);
	
	int rank;
	
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	double start, end;
	
	MPI_Barrier(MPI_COMM_WORLD);
	start = MPI_Wtime();
	
	//take on master or slave role based on ranks
	if(rank == 0)
	{
		master();
	}
	else
	{
		slave();
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	end = MPI_Wtime();
	
	//master prints runtime
	if(rank == 0)
	{
		printf("RUNTIME %lf ms\n", (end - start) * 1000);
	}
	
	//done!
	MPI_Finalize();
}

void master()
{
	
	jobs = init_queue();
	MPI_Status status;

	int hist[N];
	
	solve_master(0, hist);
	
	int processors, rank;
	
	MPI_Comm_size(MPI_COMM_WORLD, &processors);
	
	int * work;
	
	// send everyone some work!
	// keep track of how many nodes received work
	int deployed = 0;
	for(rank = 1; rank < processors; rank++)
	{
		if(!is_empty(jobs))
		{
			work = pop(jobs);
			
			MPI_Send(
				work,
				K,
				MPI_INT,
				rank,
				WORKTAG,
				MPI_COMM_WORLD);
			deployed++;
		}
	}
	
	// now listen for completions and dispatch work until done!
	
	int done[K]; //garbage buffer used to match signature
	
	while(!is_empty(jobs))
	{
		//receive completion
		
		MPI_Recv(
			done,           /* message buffer */
			1,       		/* one data item */
			MPI_INT,           /* of type double real */
			MPI_ANY_SOURCE,    /* receive from any sender */
			MPI_ANY_TAG,       /* any type of message */
			MPI_COMM_WORLD,    /* default communicator */
			&status);          /* info about the received message */
			
		// send it more work
		
		work = pop(jobs);
		
		MPI_Send(
			work,
			K,
			MPI_INT,
			status.MPI_SOURCE,
			WORKTAG,
			MPI_COMM_WORLD);
			
	}
	
	// work is done, receive outstanding jobs
	// receive outstanding jobs only from the number of machines that received work
	for(rank = 1; rank <= deployed; rank++)
	{	
		MPI_Recv(
			done,
			1,
			MPI_INT,
			MPI_ANY_SOURCE,
			MPI_ANY_TAG,
			MPI_COMM_WORLD,
			&status);
	}
	
	// Set the slaves free!
	
	for(rank=1; rank<processors; rank++)
	{
		MPI_Send(
			done,
			K,
			MPI_INT,
			rank,
			DIETAG,
			MPI_COMM_WORLD);
	}
			
}

void slave()
{

	int hist[N];
	
	int work[N];
	
	MPI_Status status;
	
	int myrank;
	
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	
	out = (char*) calloc(N + 4, sizeof(char));
	
	while(1)
	{
		// get a message from the master
		MPI_Recv(
			&work,
			K,
			MPI_INT,
			0,
			MPI_ANY_TAG,
			MPI_COMM_WORLD,
			&status);
			
		// check if everything is done
		if(status.MPI_TAG == DIETAG)
		{
			printf("%s", out);
			return;
		}
		
		// run the job received from master
		memcpy(hist, work, K * sizeof(int));
		
		solve_slave(K, hist);
		
		int something = 1;
		
		// check in completed job to master
		MPI_Send(
			&something,
			1,
			MPI_INT,
			0,
			0,
			MPI_COMM_WORLD);
	}

}

//////// QUEUE IMPLEMENTATION

/*
creates a new, empty queue and retuns a pointer to this quue
*/
queue_t * init_queue()
{
	queue_t * ret = (queue_t *) malloc(sizeof(queue_t));
	ret -> size = 0;
	ret -> head = NULL;
	ret -> tail = NULL;
	return ret;
}

/*
add the data vector to the back of the queue
*/
void enqueue(queue_t * q, int * data)
{
	node_t * new_node = (node_t *) malloc(sizeof(node_t));
	memcpy(new_node -> data, data, K * sizeof(int));
	new_node -> next = NULL;
	if(is_empty(q))
	{
		q -> head = new_node;
		q -> tail = new_node;
	}
	else
	{
		q -> tail -> next = new_node;
		q -> tail = new_node;
	}
	(q -> size) ++;
}

/*
returns
1 if the queue is empty
0 if it is not empty
*/
int is_empty(queue_t * q)
{
	return q -> size <= 0;
}

/*
pops the data at the front of the queue, removing it and updating the queue
accordingly
unconditionally returns 0 if the queue is empty. is_empty should be called 
to check for this
*/
int * pop(queue_t * q)
{
	if( q -> size > 0)
	{
		node_t * popped = q -> head;
		int * ret =  (int*) malloc(K * sizeof(int));
		memcpy(ret, popped -> data, K * sizeof(int));
		q -> head = popped -> next;
		free(popped);
		(q -> size) --;
		return ret;
	}
	else
	{
		return 0;
	}
}
