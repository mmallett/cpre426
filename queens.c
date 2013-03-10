
#include <stdio.h>
#include <stdlib.h>

#include <mpi.h>

#define WORKTAG 1
#define DIETAG 2

#define BUFFER_SIZE 20
#define ANSWER SIZE 100


//////// MASTER SLAVE PROBLEM DEFINITIONS

void master(void);
void slave(void);

int is_valid(int i);

static int k = 3;

//////// END MASTER SLAVE

//////// QUEUE DEFINITIONS

typedef struct node
{
	int data;
	struct node * next;
} node_t;

typedef struct queue
{
	int size;
	node_t * head;
	node_t * tail;
} queue_t;

queue_t * init_queue();

void enqueue(queue_t * q, int data);

int is_empty(queue_t * q);

int pop(queue_t *);

//////// END QUEUE

int
main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);

	int myrank;
	MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
	if (myrank == 0)
	{
		master();
	}
	else
	{
		slave();
	}

	MPI_Finalize();
	return 0;
}


void master()
{
	int ntasks, rank;
	int work;
	int result;
	MPI_Status status;

	//generate tasks

	int start = 1, end = 8, i;

	for(i=0; i<k-1; i++)
	{
		start = start * 10 + 1;
		end = end * 10 + 8;
	}

	queue_t * jobs = init_queue();

	for(i=start; i<=end; i++)
	{
		if(is_valid(i))
		{
			enqueue(jobs, i);
		}
	}

	/* Find out how many processes there are in the default
	communicator */

	MPI_Comm_size(MPI_COMM_WORLD, &ntasks);

	/* Seed the slaves; send one unit of work to each slave. */

	for (rank = 1; rank < ntasks; ++rank)
	{

		/* Find the next item of work to do */
		work = pop(jobs);

		/* Send it to each rank */

		MPI_Send(&work,             /* message buffer */
		1,                 /* one data item */
		MPI_INT,           /* data item is an integer */
		rank,              /* destination process rank */
		WORKTAG,           /* user chosen message tag */
		MPI_COMM_WORLD);   /* default communicator */
	}

	/* Loop over getting new work requests until there is no more work
	to be done */

	while (!is_empty(jobs))
	{

		/* Receive results from a slave */

		MPI_Recv(&result,           /* message buffer */
		1,                 /* one data item */
		MPI_DOUBLE,        /* of type double real */
		MPI_ANY_SOURCE,    /* receive from any sender */
		MPI_ANY_TAG,       /* any type of message */
		MPI_COMM_WORLD,    /* default communicator */
		&status);          /* info about the received message */

		/* Send the slave a new work unit */

		work = pop(jobs);

		MPI_Send(&work,             /* message buffer */
		1,                 /* one data item */
		MPI_INT,           /* data item is an integer */
		status.MPI_SOURCE, /* to who we just received from */
		WORKTAG,           /* user chosen message tag */
		MPI_COMM_WORLD);   /* default communicator */

	}

	/* There's no more work to be done, so receive all the outstanding
	results from the slaves. */

	for (rank = 1; rank < ntasks; ++rank)
	{
		MPI_Recv(&result, 1, MPI_DOUBLE, MPI_ANY_SOURCE,
		MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	}

	/* Tell all the slaves to exit by sending an empty message with the
	DIETAG. */

	for (rank = 1; rank < ntasks; ++rank)
	{
		MPI_Send(0, 0, MPI_INT, rank, DIETAG, MPI_COMM_WORLD);
	}
}


static void 
slave(void)
{
	unit_of_work_t work;
	unit_result_t results;
	MPI_Status status;

	while (1)
	{

		/* Receive a message from the master */

		MPI_Recv(&work, 1, MPI_INT, 0, MPI_ANY_TAG,
		MPI_COMM_WORLD, &status);

		/* Check the tag of the received message. */

		if (status.MPI_TAG == DIETAG)
		{
			return;
		}

		/* Do the work */

		// TODO: DEW WERK HEAR!

		/* Send the result back */

		MPI_Send(&result, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
	}
}

//////// QUEUE IMPLEMENTATION

queue_t * init_queue()
{
	queue_t * ret = (queue_t *) malloc(sizeof(queue_t));
	ret -> size = 0;
	ret -> head = NULL;
	ret -> tail = NULL;
}

void enqueue(queue_t * q, int data)
{
	node_t * new_node = (node_t *) malloc(sizeof(node_t));
	new_node -> data = data;
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

int is_empty(queue_t * q)
{
	return q -> size <= 0;
}

int pop(queue_t * q)
{
	if( q -> size > 0)
	{
		node_t * popped = q -> head;
		int ret = popped -> data;
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
