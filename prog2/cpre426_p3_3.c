#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc, char** argv)
{
        MPI_Init(&argc, &argv);
        MPI_Comm comm;
        int tosort[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        int m = sizeof(tosort) / sizeof(tosort[0]);
        //Get the size of the comm
        int q;
        MPI_Comm_size(comm, &q);
        int color_id = 0;

        if(q == 1){
                //Sort serially.
                //Print to file.
        }
        else {
                //1. If q>1 all processors in the communicator use the same random number
                //   generator to generate a random number between 0 and m-1, say k.

                srand(123);
                int k = rand() % m;

                if(k == 0)
                {
                        k = 1;
                }

                //See if this processor has the kth integer
                int rank;
                MPI_Comm_rank(comm, &rank);

                int offset = rank * m;
                int offset_top = offset + ((rank + 1) * m);
                int pivot = 0;

                //2. The processor that has the kth integer (pivot) broadcasts it to all
                //   processors in the communicator.

                //If kth integer broadcast its value to all processors in group
                if(k > offset && k < offset_top)
                {
                        pivot = tosort[k];
                        //kth integer is in this process, broadcast it to all other processors
                        MPI_Bcast(&pivot, 1, MPI_INT, 0, comm);
                }

                MPI_Barrier(comm);

				//3. Each processor goes through its integer array and partitions it into two sub-arrays
                //   one containing integers less than or equal to the pivot and another containing
                //   integers  greater than the pivot.

                //Find the number of elements greater than pivot
                int num_greater_than_pivot = 0;
                int num_less_than_pivot = 0;
                int i;
                for(i = offset; i < offset_top; i++)
                {
                        if(tosort[i] > pivot)
                        {
                                num_greater_than_pivot++;
                        }
                        else {
                                num_less_than_pivot++;
                        }
                }
                //Create the arrays to contained partitioned integers
                int m_double_prime[num_greater_than_pivot];
                int m_prime[num_less_than_pivot];


                //Partition the array.
                int j = 0;
                for(i = offset; i < offset_top; i++)
                {
                        if(tosort[i] > pivot)
                        {
                                m_double_prime[j] = tosort[i];
                        }
                        else {
                                m_prime[j] = tosort[i];
                        }

                        j++;
                }

                //4. Using an allgather operation, the number of integers in each of the two
                //   sub-arrays on all the processors is gathered on every processor.

                //Allgather the number of elements in each subarray
                int *m_prime_buf;
                int *m_double_prime_buf;

                m_prime_buf = (int *)malloc(q*sizeof(int));
                m_double_prime_buf = (int *)malloc(q*sizeof(int));

                MPI_Allgather(&num_greater_than_pivot, 1, MPI_INT, &m_double_prime_buf, 1, MPI_INT, comm);
                MPI_Allgather(&num_less_than_pivot, 1, MPI_INT, &m_prime_buf, 1, MPI_INT, comm);

                MPI_Barrier(comm);

                //5. Compute the total number of integers less than or equal to the pivot and the total
                //   number of integers greater than the pivot.

                int m_prime_total = 0;
                int m_double_prime_total = 0;

                for(i = q; i < q; i++)
                {
                        m_prime_total = m_prime_total + m_prime_buf[i];
                        m_double_prime_total = m_double_prime_total + m_double_prime_buf[i];
                }

				//6. Parition the p processors to the two sub-problems of sorting m' integers and sorting m''
                //   integers respectively, by allocating processors in proportion to the problem sizes(make
                //   sure you do not allocate zero processors to a problem).

                //Each subarray must have a processor
                int size_to_allocate = q - 2;

                //Find the number of processors each subarray needs
                int less_than_allocation;
                int greater_than_allocation;

                if(m_prime_total > m_double_prime_total)
                {
                        less_than_allocation =
                                ((m_double_prime_total * size_to_allocate) + m_prime_total - 1) / m_prime_total;
                        greater_than_allocation = size_to_allocate - less_than_allocation;
                }
                else {
                        greater_than_allocation =
                                ((m_prime_total * size_to_allocate) + m_double_prime_total - 1) / m_double_prime_total;
                }

                //Find greater of two to sift by
                int sift;

                if(less_than_allocation > greater_than_allocation)
                {
                        sift = less_than_allocation;
                }
                else {
                        sift = greater_than_allocation;
                }

                if(rank == 0)
                {
                        //Set to lower sift
                        MPI_Comm_split(comm, 1, 0, &new_comm);
                }
                else if(rank == (q-1))
                {
                        //Set to higher sift
                        MPI_Comm_split(comm, 2, 0, &new_comm);
                }
                else if(rank < sift)
                {
                        //Set to lower sift
                        MPI_Comm_split(comm, 1, rank, &new_comm);
                }
                else
                {
                        //Set to higher sift
                        MPI_Comm_split(comm, 2, (rank - sift), &new_comm);
                }

                //7. Using the gathered information on the sizes of the sub-arrays on all processors
                //   each processor can compute where to send its data and from where to recieve.

                //8. Use an Alltoall communication to perform the data transfer. Create two new
                //   communicators corresponding to the new paritions.

                //9. Recursively sort within each partition.
                //Set old comm to new comm
        }
}
