#include <stdio.h>
#include <stdlib.h>

#include <mpi.h>

static const int N = 8;
static int done = 0;
 
void solve(int col, int *hist)
{
	if(done) return; //break out of recursion, answer found

	if (col == N) {
		for(i=0; i< N; i++, printf("%d", hist[i]+1));
		printf("\n");
		done = 1;
		return;
	}
 
#	define attack(i, j) (hist[j] == i || abs(hist[j] - i) == col - j)
	int j;
	for (i = 0, j = 0; i < N; i++) {
		for (j = 0; j < col && !attack(i, j); j++);
		if (j < col) continue;
 
		hist[col] = i;
		solve(N, col + 1, hist);
	}
}
 
int main(int argc, char **argv)
{
	MPI_Init(&argc, &argv);
	
	int rank;
	
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	double start, end;
	
	MPI_Barrier(MPI_COMM_WORLD);
	start = MPI_Wtime();
	
	if(rank == 0)
	{
		int * hist = (int*) malloc(N * sizeof(int));
		solve(N, 0, hist);
	}
	
	MPI_Barrier(MPI_COMM_WORLD);
	end = MPI_Wtime();
	
	printf("RUNTIME %lf ms\n", (end - start) * 1000);
	
	MPI_Finalize();
	
	return 0;
}