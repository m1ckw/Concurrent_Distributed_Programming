////////////////////////////// ACTIVITY 2 - Q1 & 3 ////////////////////////////

// COMPILE: mpicxx mpi_vectorAdd.cpp -o vec.o
// RUN HEAD: mpirun -np 6 ./vec.o
// RUN CLUSTER: sudo mpirun -np 6 -hostfile ./cluster ./vec.o

#include <mpi.h>
#include <cstdlib>
#include <iostream>
#include <time.h>
#include <chrono>

using namespace std;
using namespace std::chrono;

#define SIZE 102000000  //102,000,000

// Populates vectors with random numbers 1-99.
void rand_vector(int vector[], int size)
{
    for (int i = 0; i < size; i++)
    {
        vector[i] = rand() % 100;
    }
}


int vector_add(int v1[], int v2[], int v3[], int partition)
{
    int sum = 0;
    // Sum the values of the vector.
    for (int i = 0; i<partition; i++)
    {
        v3[i] = v1[i] + v2[i];
        sum += v3[i];
    }
    return sum;
}


int main(int argc, char** argv)
{
    int num_tasks, rank, name_len, tag=1;
    char name[MPI_MAX_PROCESSOR_NAME];

    // Creating the pointes and allocating the memory.
    int *v1, *v2, *v3, *v1_sub, *v2_sub, *v3_sub, total_sum;

    v1 = (int*) malloc(SIZE * sizeof(int));
    v2 = (int*) malloc(SIZE * sizeof(int));
    v3 = (int*) malloc(SIZE * sizeof(int));

    // Setting up and starting the Parallel Processes
    MPI_Status status; // Required to receive routines
    MPI_Init(&argc,&argv); // Initialize the MPI environment
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks); // Get the number of tasks/process
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Get the rank i.e. process i.d.
    MPI_Get_processor_name(name, &name_len); // Find the processor name// Find the processor name

    int partition = SIZE/num_tasks;

    v1_sub = (int*) malloc(partition * sizeof(int));
    v2_sub = (int*) malloc(partition * sizeof(int));
    v3_sub = (int*) malloc(partition * sizeof(int));

    if (rank == 0)
    {
        rand_vector(v1, SIZE);
        rand_vector(v2, SIZE);

        total_sum = 0;
    }

    auto start = high_resolution_clock::now();

    if (rank == 0)
    {
        printf("%s Broadcast %d elements::Sending instructions\n"
        ,name, SIZE);
    }
    MPI_Scatter(v1, partition, MPI_INT, v1_sub, partition, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Scatter(v2, partition, MPI_INT, v2_sub, partition, MPI_INT, 0, MPI_COMM_WORLD);

    int local_sum = 0;
    local_sum = vector_add(v1_sub, v2_sub, v3_sub, partition);

    MPI_Gather(v3_sub, partition, MPI_INT, v3, partition, MPI_INT, 0, MPI_COMM_WORLD);
    printf(" P%d has completed adding %d elements.\"\n", rank, partition);

    MPI_Reduce(&local_sum, &total_sum, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
        auto stop = high_resolution_clock::now();
        auto duration = duration_cast<milliseconds>(stop - start);
        cout << "Total time taken: " << duration.count()
            << " milliseconds (" << duration.count()/1000.0
            << " seconds).\n Total sum: " << total_sum << endl;
    }

    // Freeing the allocated memory.
    free(v1); v1 = NULL;
    free(v2); v2 = NULL;
    free(v3); v3 = NULL;
    free(v1_sub); v1_sub = NULL;
    free(v2_sub); v2_sub = NULL;
    free(v3_sub); v3_sub = NULL;

    // Finalize the MPI environment
    MPI_Finalize();
}
