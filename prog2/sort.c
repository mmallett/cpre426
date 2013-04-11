/*
Matt Mallett mmallett@iastate.edu
Deven Starn djstarn@gmail.com

CprE 426 Programming Assignment 2
sort.c
parallelized quicksort via MPI
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpi.h>

/*
>>>>>>>>>>>>>>>>>>>>>>>>>>>CHANGE THESE<<<<<<<<<<<<<<<<<<<<<<<<<<<
*/
static const char IN_PATH[] = "/home/mmallett/cpre426/prog2/in.txt"; //path to input file
static const char OUT_PATH[] = "/home/mmallett/cpre426/prog2/out/"; //path to directory where output will go
//note this ends in a /
/*
>>>>>>>>>>>>>>>>>>>>>>>>>>>CHANGE THESE<<<<<<<<<<<<<<<<<<<<<<<<<<<
*/

int main(int argc, char ** argv){
	MPI_Init(&argc, &argv);
	int world_rank, world_size;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	int * rec_data;
	int   rec_cnt;

	int * send_data;
	int * send_cnt;
	int * send_disp;

	int n;

	if(world_rank == 0){
		FILE * fp;
		fp = fopen(IN_PATH, "r");

		if(fp == NULL){
			perror("Could not open file");
			exit(EXIT_FAILURE);
		}

		//first line is problem size
		fscanf(fp, "%d\n", &n);

		int i;
		send_data = (int*) calloc(n, sizeof(int));

		//fill buffer with data from file
		for(i=0; i<n; i++){
			fscanf(fp, "%d\n", &send_data[i]);
		}

		fclose(fp);

		send_cnt = (int*) calloc(world_size, sizeof(int));
		send_disp = (int*) calloc(world_size, sizeof(int));

		//calculate vector sizes for each processor
		for(i=0; i<world_size; i++){
			send_cnt[i] = n/world_size;
		}
		send_cnt[0] += n%world_size;

		//calculate vector displacements for each processor
		send_disp[0] = 0;
		for(i=1; i<world_size; i++){
			send_disp[i] = send_disp[i-1]+send_cnt[i-1];
		}
	}

	//scatter incoming vector sizes to processors
	MPI_Scatter(
		send_cnt,
		1,
		MPI_INT,
		&rec_cnt,
		1,
		MPI_INT,
		0,
		MPI_COMM_WORLD
	);

	rec_data = (int*) malloc(rec_cnt * sizeof(int));

	//scatter data to processors
	MPI_Scatterv(
		send_data,
		send_cnt,
		send_disp,
		MPI_INT,
		rec_data,
		rec_cnt,
		MPI_INT,
		0,
		MPI_COMM_WORLD
	);

	////////////////////////////////////////////////////////
	// END DISTRIBUTION STAGE
	// SORTING STAGE BEGIN
	////////////////////////////////////////////////////////

	int * data = rec_data;
	int data_length = rec_cnt;

	int m;
	int q = world_size;

	MPI_Bcast(
		&n,
		1,
		MPI_INT,
		0,
		MPI_COMM_WORLD
	);

	m = n;
	
	MPI_Comm comm;
	MPI_Comm_dup(MPI_COMM_WORLD, &comm);

	printf("[%d] %d items: ", world_rank, data_length);
        int z;
        for(z=0; z<data_length; z++){
                printf("%d ", data[z]);
        }
        printf("\n");

	srand(18);

	while(q>1){
		int rank;
		MPI_Comm_rank(comm, &rank);
		
		//figure out who has the kth integer
		int k = rand()%(m-1);

		printf("[%d] k:%d m:%d\n", world_rank, k, m);
		
		int * sizes = (int*) malloc(q * sizeof(int));
		MPI_Allgather(
			&data_length,
			1,
			MPI_INT,
			sizes,
			1,
			MPI_INT,
			comm
		);

		printf("[%d] allgather sizes: ", world_rank);
		for(z=0; z<q; z++){
			printf("%d ", sizes[z]);
		}
		printf("\n");

		int i;
		int data_length_total = sizes[0];
		int has_k = 0;
		for(i=1; i<q; i++){
			data_length_total += sizes[i];
			sizes[i] += sizes[i-1];
			if(k >= sizes[i-1] && k < sizes[i]){
				has_k = i;
			}
		}

		printf("[%d] proc offsets: ", world_rank);
		for(z=0; z<q; z++){
			printf("%d ", sizes[z]);
		}
		printf("\n");
		printf("[%d] proc w/ k:%d\n", world_rank, has_k);

		int k_val;
		if(has_k == rank){
			if(rank == 0){
				k_val = data[k];
			}
			else{
				k_val = data[k-sizes[rank-1]];
			}
		}
		
		//broadcast k value to everyone
		MPI_Bcast(
			&k_val,
			1,
			MPI_INT,
			has_k,
			comm
		);

		printf("[%d] k value:%d\n", world_rank, k_val);
			
		//partition data around k
		int left = 0, right = data_length-1;
		while(left < right){
			while(left <= right && data[left] <= k_val) left++;
			while(left < right && data[right] > k_val) right--;
			if(left < right){//swap
				int tmp = data[left];
				data[left] = data[right];
				data[right] = tmp;
			}
		}

		printf("[%d] partitioned: ", world_rank);
		for(z=0; z<data_length; z++){
			printf("%d ", data[z]);
		}
		printf("\n");

		int left_size = right;
		int right_size = data_length - right;

		printf("[%d] sizes L:%d R:%d\n", world_rank, left_size, right_size);

		int * left_sizes = (int*) malloc(q * sizeof(int));
	
		MPI_Allgather(
			&left_size,
			1,
			MPI_INT,
			left_sizes,
			1,
			MPI_INT,
			comm
		);

		int left_total = 0;
		for(i=0; i<q; i++){
			left_total += left_sizes[i];
		}
		int right_total = data_length_total - left_total;

		printf("[%d] totals L:%d R:%d\n", world_rank, left_total, right_total);

		//allocate processors to the 2 partitions
		int available = q - 2;
		int left_group_size = (left_total/data_length)*available + 1;
		int right_group_size = q - left_group_size;

		printf("[%d] allocated L:%d R:%d\n", world_rank, left_group_size, right_group_size);

		//split up buffer for distribution
		free(send_cnt);
		free(send_disp);
		send_cnt = (int*) malloc(q * sizeof(int));
		send_disp = (int*) malloc(q * sizeof(int));

		int left_buff_size = left_size/left_group_size;
		for(i=0; i<left_group_size; i++){
			send_cnt[i] = left_buff_size;
		}
		send_cnt[left_group_size-1] += left_size%left_group_size;

		int right_buff_size = right_size/right_group_size;
		for(i=left_group_size; i<q; i++){
			send_cnt[i] = right_buff_size;
		}
		send_cnt[q-1] += right_size%right_group_size;

		printf("[%d] send counts: ", world_rank);
		for(z=0; z<q; z++){
			printf("%d ", send_cnt[z]);
		}
		printf("\n");

		send_disp[0] = 0;
		for(i=1; i<q; i++){
			send_disp[i] = send_disp[i-1] + send_cnt[i-1];
		}
		
		printf("[%d] displacements: ", world_rank);
		for(z=0; z<q; z++){
			printf("%d ", send_disp[z]);
		}
		printf("\n");

		//send the vector sizes
		int * recv_cnt = (int*) malloc(q * sizeof(int));
		MPI_Alltoall(
			send_cnt,
			1,
			MPI_INT,
			recv_cnt,
			1,
			MPI_INT,
			comm
		);

		printf("[%d] recv counts: ", world_rank);
		for(z=0; z<q; z++){
			printf("%d ", recv_cnt[z]);
		}
		printf("\n");
		
		//prepare arguments
		int total_inc_size = 0;
		int * recv_disp = (int*) malloc(q * sizeof(int));
		for(i=0; i<q; i++){
			total_inc_size += recv_cnt[i];
		}
		recv_disp[0] = 0;
		for(i=1; i<q; i++){
			recv_disp[i] = recv_disp[i-1] + recv_cnt[i-1];
		}

		printf("[%d] recv displacements: ", world_rank);
		for(z=0; z<q; z++){
			printf("%d ", recv_disp[z]);
		}
		printf("\n");

		rec_data = (int*) malloc(total_inc_size * sizeof(int));

		//send the data!
		MPI_Alltoallv(
			data,
			send_cnt,
			send_disp,
			MPI_INT,
			rec_data,
			recv_cnt,
			recv_disp,
			MPI_INT,
			comm
		);

		printf("[%d] received %d elements: ", world_rank, total_inc_size);
		for(z=0; z<total_inc_size; z++){
			printf("%d ", rec_data[z]);
		}
		printf("\n");

		//housekeeping
		free(data);
		data = rec_data;
		data_length = total_inc_size;

		printf("[%d] survived transfer: ", world_rank);
		for(z=0; z<data_length; z++){
			printf("%d ", rec_data[z]);
		}
		printf("\n");

		free(recv_cnt);
		free(recv_disp);
		
		//split communicator in 2
		int color = rank < left_group_size;
		int new_rank = (color) ? rank : rank - left_group_size;

		MPI_Comm new_comm;
		MPI_Comm_split(
			comm,
			color,
			new_rank,
			&new_comm
		);
		comm = new_comm;

		MPI_Comm_size(comm, &q);

		printf("[%d] color:%d rank:%d size:%d\n", world_rank, color, new_rank, q);
	}//end while

	///////////////////////////////////////////////////////
	// PARALLEL SORTING DONE
	// BEGIN SERIAL SORT
	///////////////////////////////////////////////////////

	//SELECTION SORT LAIKA BOSS
	int i,j,min;
	for(j=0; j<data_length-1; j++){
		min=j;
		for(i=j+1; i<data_length; i++){
			if(data[i] < data[min]){
				min=i;
			}
		}
	
		if(min != j){
			int tmp = data[j];
			data[j] = data[min];
			data[min] = tmp;
		}
	}
	
	///////////////////////////////////////////////////////
	// END SORTING
	// BEGIN OUTPUT SECTION
	///////////////////////////////////////////////////////
	
	char path_buffer[50];
	strcpy(path_buffer, OUT_PATH);
	sprintf(path_buffer, "%spart%d.txt", path_buffer, world_rank);
	
	FILE * out;
	out=fopen(path_buffer, "w");
	
	if(out == NULL){
			perror("Could not open file");
			exit(EXIT_FAILURE);
	}
	
	for(i=0; i<data_length; i++){
		fprintf(out, "%d\n", data[i]);
	}
	
	fclose(out);
	
	//printf("[%d] sum barry urz!\n", world_rank);
	MPI_Barrier(MPI_COMM_WORLD);
	//printf("[%d] de r dun wit\n", world_rank);
	
	if(world_rank == 0){
		strcpy(path_buffer, OUT_PATH);
		sprintf(path_buffer, "%ssorted.txt", path_buffer);
		
		out = fopen(path_buffer, "w");
		
		if(out == NULL){
			perror("Could not open file");
			exit(EXIT_FAILURE);
		}
		
		for(i=0; i<world_size; i++){
			strcpy(path_buffer, OUT_PATH);
			sprintf(path_buffer, "%spart%d.txt", path_buffer, i);
			
			FILE * readin;
			readin = fopen(path_buffer, "r");
			
			if(readin == NULL){
				fprintf(out, "part %d could not be read\n", i);
				continue;
			}
			
			//read in write to sorted.txt
			int c;
			while((c = fgetc(readin)) != EOF){
				fputc(c, out);
			}
			
			fclose(readin);
		}
		
		fclose(out);
	}
	
	MPI_Finalize();
	return 0;
}
			
	 


	
