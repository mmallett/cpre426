/********************************************************
Matt Mallett, Deven Starn
CprE 426 Programming Assignment 1

nqueens-single.c
Find a single solution to the n-queens problem.

Sources
http://rosettacode.org/wiki/N-queens_problem#C
	Serial n-queens solution
********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

//problem size
static const int N = 8;
//flag indicating a solution is found
static int done = 0;
 
void solve(int col, int *hist)
{
	if(done) return; //break out of recursion, answer found

	int i, j;
	
	if (col == N) {
		for(i=0; i< N; i++, printf("%d ", hist[i]+1));
		printf("\n"); //print once, and done
		done = 1;
		return;
	}
 
#	define attack(i, j) (hist[j] == i || abs(hist[j] - i) == col - j)
	for (i = 0, j = 0; i < N; i++) { //for each row
		for (j = 0; j < col && !attack(i, j); j++); //if this piece is safe, move to the next column
		if (j < col) continue;
 
		hist[col] = i; //safe combination, store it for the next recursive call to check against
		solve(col + 1, hist);
	}
}
 
int main(int argc, char **argv)
{
	//mpi book keeping
	MPI_Init(&argc, &argv);
	
	int rank;
	
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	double start, end;
	
	MPI_Barrier(MPI_COMM_WORLD);
	start = MPI_Wtime();
	
	//master runs the serial algorithm
	if(rank == 0)
	{
		int * hist = (int*) malloc(N * sizeof(int));
		solve(0, hist);
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	end = MPI_Wtime();
	
	//master output runtime
	if(rank == 0) printf("RUNTIME %lf ms\n", (end - start) * 1000);
	
	MPI_Finalize();
	
	return 0;
}