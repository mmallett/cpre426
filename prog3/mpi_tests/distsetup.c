#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(int argc, char ** argv){
	int p, m;
	
	sscanf(argv[1], "%d", &p);
	sscanf(argv[2], "%d", &m);
	
	// What dimensions are necessary?
	int x = sqrt(p);
	int y = sqrt(p);
	
	float result = sqrt(p);
	float check = fmod(result, 1);
	
	if(check != 0)
	{
		float a = log2l(p);
		x = pow(2, ((a + 1) / 2));
		y = p / x;
	}
	
	printf("%d x %d\n\n", x, y);
	
	// What coordinates do I need to obtain?
	// First find leftover.
	int size_x = m / p;
	int leftover_x = m - (p * size_x); 
	
	int my_rank;
	
	for(my_rank == 0; my_rank < p; my_rank++)
	{
		int start_adjust_x = 0;
		int end_adjust_x = 0;
		
		if(leftover_x != 0)
		{
			int p_adjust_x = (p - leftover_x) + 1;
			
			if((my_rank + 1) >= p_adjust_x)
			{
				printf("Adjusted ");
				start_adjust_x = leftover_x - (p - my_rank);
				end_adjust_x = leftover_x - (p - (my_rank + 1));
			}
		}
		
		int from_x = (my_rank * size_x) + start_adjust_x;
		int to_x = ((my_rank + 1) * size_x) + end_adjust_x;
		
		printf("%d : (%d, 0) --> (%d, y)\n", my_rank, from_x, to_x); 
	}
}
